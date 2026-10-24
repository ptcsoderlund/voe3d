// The foliage's gathering (0442 point 5): voe_3d_draw_system_foliage, which
// 3d/draw_system.h states, walks every copy of every landscape row _run draws
// solid and writes each as a render instance into its group of (kind, level,
// part, rest, tier), draw_foliage.h's key. Internal to 3d; called once a frame
// per view, inside the frame and before its passes.
//
// A COPY'S TIER (0464 point 4) is found from the planes made once a call: the
// camera's six, then each casting sun's four cascades, fitted with
// voe_3d_shadow_cascades_fit exactly as draw_shadows.c fits them, which keeps
// no state, so the two agree. The first walk counts the copies into the
// frame's `counts`.
//
// TWO WALKS OVER THE SAME COPIES: the first counts each group's instances, the
// second fills them contiguously, group after group, into one array on the
// caller's arena, written by one voe_render_frame_instances and given back; then
// the groups' draws, one each, solid and cast (draw_foliage.h). Both walks
// take a row's own copies and then its grown copies (0476 point 5), each with
// its placed transform, so they find the same groups in the same order, the
// counts the first made are the room the second fills, and a group the first
// left out past VOE_3D_FOLIAGE_GROUPS the second leaves out too.
//
// A ROW'S KINDS ARE FOUND ONCE: each kind's loaded `.glb`, each of its parts as
// worn, a blended one left out (0442 point 5), and whether its look is
// shadowless (0476 point 6). A copy's (kind, level, part,
// rest) is looked up in a per-row table and only its first sight walks the
// groups' keys, so a row's copies cost no search.
//
// Constraints: every copy's level is chosen and its matrix and tier made each
// frame and view, twice, one per walk, each copy tested against up to 22
// planes' volumes; the walk's state, about 70 KB, is on the stack; the
// instances need 64 bytes each of the arena, up to VOE_3D_FOLIAGE_INSTANCES.
#include "draw_foliage.h"
#include "cull.h"
#include "draw_bounce.h"
#include "draw_group.h"
#include "draw_material.h"
#include "draw_models.h"

#include <3d/draw_system.h>
#include <3d/model_component.h>
#include <3d/model_levels.h>
#include <3d/models_foliage.h>
#include <3d/shadow_cascades.h>
#include <base/assert.h>
#include <math/float4x4.h>
#include <base/report.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <string.h>

// A per-row slot that has not been looked up, and one whose group was left out.
#define SLOT_UNSEEN 0u
#define SLOT_LEFT_OUT 0xffffu

static_assert(VOE_3D_FOLIAGE_GROUPS < SLOT_LEFT_OUT,
	      "a slot holds a group's index plus one");

// One walk's state: the groups found so far, by key, and the row's tables.
struct gathering {
	struct voe_3d_foliage_key keys[VOE_3D_FOLIAGE_GROUPS];
	voe_3d_foliage_group groups[VOE_3D_FOLIAGE_GROUPS];
	// The second walk's cursor in each group.
	uint32_t filled[VOE_3D_FOLIAGE_GROUPS];
	uint32_t count;
	// Every group's instances, counted by the first walk.
	uint32_t instances;
	// A group was left out past VOE_3D_FOLIAGE_GROUPS.
	bool left_out;
	// Where the second walk writes; NULL while the first counts.
	voe_render_instance *out;
	// The camera's planes, and each casting sun's cascades', sun side open.
	struct voe_3d_cull view;
	struct voe_3d_cull cascades[VOE_RENDER_DIRECTIONAL_LIGHTS *
				    VOE_RENDER_SHADOW_CASCADES];
	uint32_t cascade_count;
	// The first walk's copies not `rest`: seen as `drawn`, else `skipped`.
	voe_3d_view_counts counts;
	// This row's kinds: each one's loaded `.glb`, NULL for none, and each of
	// its parts as worn, NULL for a blended one.
	const voe_3d_model_entry *kinds[VOE_ASSETS_LANDSCAPE_KINDS];
	const voe_3d_model_part *worn[VOE_ASSETS_LANDSCAPE_KINDS]
				     [VOE_3D_MODEL_PARTS];
	// This row's look of each kind is shadowless.
	bool shadowless[VOE_ASSETS_LANDSCAPE_KINDS];
	// This row's group of each (kind, level, part, rest, tier): index plus
	// one, SLOT_UNSEEN or SLOT_LEFT_OUT.
	uint16_t slots[VOE_ASSETS_LANDSCAPE_KINDS][VOE_3D_MODEL_LEVELS]
		      [VOE_3D_MODEL_PARTS][2][3];
};

static bool same_key(struct voe_3d_foliage_key a, struct voe_3d_foliage_key b)
{
	return a.kind == b.kind && a.level == b.level && a.part == b.part &&
	       a.rest == b.rest && a.tier == b.tier && a.casts == b.casts;
}

// The camera's planes and, for each casting sun of `frame` (as draw_shadows.c
// chooses them: shaded, of some strength, its row casting), its four cascades'
// fitted at its direction, into `g`.
static void make_culls(const voe_ecs_world *world, const voe_3d_frame *frame,
		       struct gathering *g)
{
	VOE_BASE_ASSERT(frame->more_count < VOE_RENDER_DIRECTIONAL_LIGHTS &&
				(frame->more_count == 0 || frame->more_lights != NULL),
			"more lights than a pass holds, or none to hold them");
	g->view = voe_3d_cull_of(frame->view, false);
	g->cascade_count = 0;
	for (uint32_t light = 0; light <= frame->more_count; light++) {
		const voe_render_light *sun =
			light == 0 ? &frame->light :
				     &frame->more_lights[light - 1].light;
		voe_3d_shadow_cascades fitted;

		if (sun->unshaded || sun->intensity <= 0.0f ||
		    !voe_3d_draw_light_casts(world, light))
			continue;
		fitted = voe_3d_shadow_cascades_fit(frame->view, frame->eye,
						    sun->direction,
						    VOE_3D_SHADOW_TEXELS);
		for (uint32_t c = 0; c < VOE_RENDER_SHADOW_CASCADES; c++)
			g->cascades[g->cascade_count++] =
				voe_3d_cull_of(fitted.light[c], true);
	}
	VOE_BASE_ASSERT(g->cascade_count <= VOE_RENDER_DIRECTIONAL_LIGHTS *
						    VOE_RENDER_SHADOW_CASCADES,
			"more cascades than room");
}

// The tier of a copy of `kind` placed by `place`, a matrix about the eye: its
// entry's sphere seen by the camera, else by a cascade, else neither.
static voe_3d_foliage_tier tier_of(const struct gathering *g,
				   const voe_3d_model_entry *kind,
				   voe_math_float4x4 place)
{
	const voe_math_float3 centre =
		voe_math_float4x4_transform_point(place, kind->centre);
	const float radius = voe_3d_cull_radius(place, kind->radius);

	VOE_BASE_ASSERT(radius >= 0.0f, "a copy of negative size");
	if (voe_3d_cull_sees(&g->view, centre, radius))
		return VOE_3D_FOLIAGE_SEEN;
	for (uint32_t c = 0; c < g->cascade_count; c++)
		if (voe_3d_cull_sees(&g->cascades[c], centre, radius))
			return VOE_3D_FOLIAGE_CAST;
	return VOE_3D_FOLIAGE_UNSEEN;
}

// The top three rows of a matrix about the eye, the fourth being (0, 0, 0, 1).
static voe_render_instance instance_of(voe_math_float4x4 matrix)
{
	voe_render_instance instance = { 0 };

	for (uint32_t r = 0; r < 3; r++)
		instance.rows[r] = (voe_math_float4){ matrix.m[r][0], matrix.m[r][1],
						      matrix.m[r][2], matrix.m[r][3] };
	VOE_BASE_ASSERT(matrix.m[3][3] == 1.0f, "a placed matrix is affine");
	return instance;
}

// Each kind of `grid` found once in `models`: a loaded `.glb`, each of its
// parts as it wears it, its record's alpha mode the same at every level, and
// whether `grid`'s look of it is shadowless.
static void find_kinds(const voe_3d_models *models,
		       const voe_assets_landscape *grid, struct gathering *g)
{
	VOE_BASE_ASSERT(grid->kind_count <= VOE_ASSETS_LANDSCAPE_KINDS,
			"a landscape with more kinds than it holds");
	memset(g->kinds, 0, sizeof(g->kinds));
	memset(g->worn, 0, sizeof(g->worn));
	for (uint32_t k = 0; k < grid->kind_count; k++) {
		const voe_3d_model_entry *kind =
			voe_3d_models_find(models, grid->kinds[k]);

		g->shadowless[k] = grid->looks[k].shadowless;
		if (kind == NULL || !kind->loaded || kind->picture ||
		    kind->material || kind->landscape != NULL)
			continue;
		VOE_BASE_ASSERT(kind->part_count <= VOE_3D_MODEL_PARTS,
				"a kind with more parts than the store holds");
		g->kinds[k] = kind;
		for (uint32_t p = 0; p < kind->part_count; p++) {
			const voe_3d_model_part *named = voe_3d_draw_material_named(
				models, kind->parts[p].wears);
			const voe_3d_model_part *worn =
				named != NULL ? named : &kind->parts[p];

			if (worn->material.alpha_mode != VOE_RENDER_ALPHA_BLENDED)
				g->worn[k][p] = worn;
		}
	}
}

// The group of `kind`'s part `part` at `level` in `tier`, made when first
// seen; nought past VOE_3D_FOLIAGE_GROUPS, else its index plus one.
static uint32_t group_of(struct gathering *g, uint32_t kind, uint32_t level,
			 uint32_t part, bool rest, voe_3d_foliage_tier tier)
{
	uint16_t *slot = &g->slots[kind][level][part][rest ? 1 : 0][tier];
	const struct voe_3d_foliage_key key = { .kind = g->kinds[kind],
						.level = level,
						.part = part,
						.rest = rest,
						.tier = tier,
						.casts = !g->shadowless[kind] };
	uint32_t at = 0;

	if (*slot == SLOT_LEFT_OUT)
		return 0;
	if (*slot != SLOT_UNSEEN)
		return *slot;
	while (at < g->count && !same_key(g->keys[at], key))
		at++;
	if (at == VOE_3D_FOLIAGE_GROUPS) {
		g->left_out = true;
		*slot = SLOT_LEFT_OUT;
		return 0;
	}
	if (at == g->count) {
		VOE_BASE_ASSERT(g->out == NULL, "a group made while filling");
		g->keys[at] = key;
		g->groups[at] = (voe_3d_foliage_group){
			.geometry = key.kind->levels[level].geometries[part],
			.part = g->worn[kind][part],
			.rest = rest,
			.tier = tier,
			.casts = key.casts,
		};
		g->count++;
	}
	*slot = (uint16_t)(at + 1);
	return *slot;
}

// One copy of `kind` at `level` in `tier` as one instance per drawn part that
// level has: counted while counting, else written at its group's cursor with
// `dither`.
static void gather_copy(struct gathering *g, uint32_t kind, uint32_t level,
			bool rest, voe_3d_foliage_tier tier, float dither,
			voe_render_instance instance)
{
	const voe_3d_model_entry *model = g->kinds[kind];

	VOE_BASE_ASSERT(model != NULL && level < model->level_count,
			"gathering a level the kind has not got");
	instance.dither = dither;
	for (uint32_t part = 0; part < model->part_count; part++) {
		uint32_t group;

		if (g->worn[kind][part] == NULL ||
		    model->levels[level].geometries[part].generation == 0)
			continue;
		group = group_of(g, kind, level, part, rest, tier);
		if (group-- == 0)
			continue;
		if (g->out == NULL) {
			g->groups[group].count++;
			g->instances++;
			continue;
		}
		VOE_BASE_ASSERT(g->filled[group] < g->groups[group].count,
				"filling past what was counted");
		g->out[g->groups[group].first + g->filled[group]++] = instance;
	}
}

// The `count` copies at `copies` of a grid of `kind_count` kinds, each placed
// by `placed` under the row's `where`. A shadowless kind's copy not seen is
// counted skipped and left out.
static void gather_copies(const voe_3d_frame *frame, voe_scene_transform where,
			  const voe_assets_landscape_copy *copies,
			  const voe_scene_transform *placed, uint32_t count,
			  uint32_t kind_count, struct gathering *g)
{
	VOE_BASE_ASSERT(count == 0 || (copies != NULL && placed != NULL),
			"copies with no placed transforms");
	for (uint32_t i = 0; i < count; i++) {
		const uint32_t kind = copies[i].kind;
		voe_scene_transform at;
		voe_math_float4x4 place;
		voe_3d_model_level_choice choice;
		voe_3d_foliage_tier tier;
		voe_render_instance instance = { 0 };
		bool dissolving;

		VOE_BASE_ASSERT(kind < kind_count, "a copy of no kind");
		if (g->kinds[kind] == NULL)
			continue;
		at = voe_scene_transform_compose(where, placed[i]);
		choice = voe_3d_model_level_choose(g->kinds[kind], &at, frame->eye,
						   &frame->view);
		dissolving = choice.shown < 1.0f;
		VOE_BASE_ASSERT(!dissolving || choice.level >= 1,
				"level 0 dissolving into no level before it");
		place = voe_scene_transform_matrix(at, frame->eye);
		tier = tier_of(g, g->kinds[kind], place);
		if (g->out != NULL)
			instance = instance_of(place);
		else if (tier == VOE_3D_FOLIAGE_SEEN)
			g->counts.drawn++;
		else
			g->counts.skipped++;
		if (g->shadowless[kind] && tier != VOE_3D_FOLIAGE_SEEN)
			continue;
		gather_copy(g, kind, choice.level, false, tier,
			    dissolving ? choice.shown : 0.0f, instance);
		if (dissolving)
			gather_copy(g, kind, choice.level - 1, true, tier,
				    -choice.shown, instance);
	}
}

// Every copy of `land`, a loaded landscape whose row stands at `where`: its
// own, then its grown.
static void gather_row(const voe_3d_frame *frame,
		       const voe_3d_model_entry *land, voe_scene_transform where,
		       struct gathering *g)
{
	const voe_assets_landscape *grid = land->landscape;
	const voe_3d_models_grown grown =
		voe_3d_models_landscape_grown(frame->models, land);

	VOE_BASE_ASSERT(grid != NULL, "a landscape row with no grid");
	find_kinds(frame->models, grid, g);
	memset(g->slots, 0, sizeof(g->slots));
	gather_copies(frame, where, grid->copies,
		      voe_3d_models_landscape_placed(frame->models, land),
		      grid->copy_count, grid->kind_count, g);
	gather_copies(frame, where, grown.copies, grown.placed, grown.count,
		      grid->kind_count, g);
}

// The landscape rows draw_system.c's landscape walk draws solid: not off,
// hidden or fading, among its first VOE_3D_LANDSCAPES_DRAWN.
static void walk(const voe_ecs_world *world, const voe_3d_frame *frame,
		 struct gathering *g)
{
	const voe_ecs_entity *owners = voe_3d_model_entities(world);
	uint32_t landscapes = 0;

	VOE_BASE_ASSERT(frame->models != NULL, "gathering from no store");
	for (uint32_t row = 0; row < voe_3d_model_count(world); row++) {
		const voe_3d_model_entry *land;
		const float fade = voe_3d_model_rows(world)[row].fade;

		if (fade >= 1.0f ||
		    voe_3d_draw_group_left_out(world, owners[row], frame->hidden))
			continue;
		land = voe_3d_draw_models_entry(world, frame->models, row);
		if (land == NULL ||
		    !voe_3d_draw_models_is_landscape(frame->models, land))
			continue;
		if (landscapes++ == VOE_3D_LANDSCAPES_DRAWN)
			break;
		if (fade > 0.0f)
			continue;
		gather_row(frame, land,
			   voe_scene_transform_between(world, owners[row],
						       frame->lag),
			   g);
	}
}

// Counts, fills on `arena` and writes; the groups pushed only once render took
// the instances, so a refusal leaves the arena as it was.
bool voe_3d_draw_system_foliage(const voe_ecs_world *world,
				voe_render_device *device, voe_3d_frame *frame,
				voe_base_arena *arena)
{
	static bool told_left_out, told_refused;
	struct gathering g;
	struct voe_base_arena_mark mark;
	voe_3d_foliage_group *groups;
	uint32_t first = 0;
	bool written;

	VOE_BASE_ASSERT(world != NULL && frame != NULL,
			"gathering foliage of no world or frame");
	VOE_BASE_ASSERT(device != NULL && arena != NULL,
			"gathering foliage with no device or arena");
	frame->foliage = NULL;
	frame->foliage_count = 0;
	if (frame->models == NULL || frame->blind)
		return true;
	memset(&g, 0, sizeof(g));
	make_culls(world, frame, &g);
	walk(world, frame, &g);
	frame->counts.drawn += g.counts.drawn;
	frame->counts.skipped += g.counts.skipped;
	if (g.left_out && !told_left_out)
		VOE_BASE_ERROR("3d", "more than %u foliage groups in a view; the rest are left out",
			       VOE_3D_FOLIAGE_GROUPS);
	told_left_out = told_left_out || g.left_out;
	if (g.instances == 0)
		return true;
	written = g.instances <= VOE_3D_FOLIAGE_INSTANCES;
	mark = voe_base_arena_mark(arena);
	if (written) {
		for (uint32_t i = 0, at = 0; i < g.count; at += g.groups[i++].count)
			g.groups[i].first = at;
		g.out = voe_base_arena_push(arena, sizeof(*g.out) * g.instances);
		walk(world, frame, &g);
		written = voe_render_frame_instances(device, g.out, g.instances,
						     &first);
	}
	voe_base_arena_rewind(arena, mark);
	if (!written) {
		if (!told_refused)
			VOE_BASE_ERROR("3d", "no room for %u foliage instances; none are drawn",
				       g.instances);
		told_refused = true;
		return false;
	}
	groups = voe_base_arena_push(arena, sizeof(*groups) * g.count);
	for (uint32_t i = 0; i < g.count; i++) {
		groups[i] = g.groups[i];
		groups[i].first += first;
	}
	frame->foliage = groups;
	frame->foliage_count = g.count;
	return true;
}

// One group's instanced draw: its worn record, white as a model part is
// (draw_models.c), between two two-sided calls when that record is.
static bool draw_group(voe_render_device *device,
		       const voe_3d_foliage_group *group)
{
	const voe_3d_material *material = &group->part->material;
	const voe_render_object object = {
		.shading = material->shading.index,
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
	};
	bool drawn;

	VOE_BASE_ASSERT(group->count > 0, "a foliage group of no copies");
	if (material->double_sided)
		voe_render_frame_two_sided(device, true);
	drawn = voe_render_frame_draw_instanced(device, group->geometry, object,
						group->first, group->count);
	if (material->double_sided)
		voe_render_frame_two_sided(device, false);
	return drawn;
}

bool voe_3d_draw_foliage_solid(voe_render_device *device,
			       const voe_3d_frame *frame)
{
	bool drawn = true;

	VOE_BASE_ASSERT(device != NULL && frame != NULL,
			"drawing foliage with no device or frame");
	VOE_BASE_ASSERT(frame->foliage_count == 0 || frame->foliage != NULL,
			"foliage groups counted and none held");
	if (frame->foliage_count == 0)
		return true;
	voe_render_frame_span_begin(device, "foliage");
	for (uint32_t i = 0; i < frame->foliage_count && drawn; i++)
		if (frame->foliage[i].tier == VOE_3D_FOLIAGE_SEEN)
			drawn = draw_group(device, &frame->foliage[i]);
	voe_render_frame_span_end(device);
	return drawn;
}

bool voe_3d_draw_foliage_cast(voe_render_device *device,
			      const voe_3d_frame *frame, bool every_tier,
			      voe_3d_view_counts *counts)
{
	VOE_BASE_ASSERT(device != NULL && frame != NULL,
			"casting foliage with no device or frame");
	VOE_BASE_ASSERT(frame->foliage_count == 0 || frame->foliage != NULL,
			"foliage groups counted and none held");
	for (uint32_t i = 0; i < frame->foliage_count; i++) {
		const voe_3d_foliage_group *group = &frame->foliage[i];
		const bool drawn = every_tier ||
				   group->tier != VOE_3D_FOLIAGE_UNSEEN;

		if (group->rest || !group->casts ||
		    !voe_3d_draw_casts(&group->part->material))
			continue;
		if (counts != NULL && drawn)
			counts->cast += group->count;
		else if (counts != NULL)
			counts->cast_skipped += group->count;
		if (drawn && !draw_group(device, group))
			return false;
	}
	return true;
}
