// The model rows' walk: each row's parts from the frame's store, drawn into the
// open frame as the mesh walk draws a world-layer mesh. Internal to 3d;
// draw_system.c's run calls voe_3d_draw_models once a frame and sizes the
// world's blended group with voe_3d_draw_models_part_count (draw_models.h).
//
// A model row at fade 1 or more is not drawn, and one between 0 and 1 is held
// blended (0336 point 3); each at the level the camera chooses for it (0425
// point 5). A part whose worn material is blended is held too; the rest are
// drawn solid now, two-sided when that material is double-sided (0421 point
// 2). A landscape row is skipped here: draw_system.c draws it by
// draw_terrain.c's nodes. A row whose sphere the camera cannot see is skipped
// and counted (0464 points 2 and 5).
//
// A ROW IN A LEVEL'S BAND DISSOLVES (0432 point 3): a part drawn now is drawn
// twice, the chosen level on `shown` of its pixels through the object's
// `dither` and the level before on the rest through −`shown`, each skipped
// where its level lacks the part. Only these at-once draws dissolve (0432
// point 4): a held part is drawn whole at the chosen level, so the held
// groups, sized for one entry a part, keep their size, and two blended
// levels would need a sort between them that dither does without.
//
// A ROW BLENDING WITH THE GROUND (0481 points 2, 3 and 5): one with
// `blend_with_ground` and a ground (draw_ground.h) under its world origin about
// the eye has every part's record filled with that ground at its
// `blend_height`. Its held parts are held as ever, filled; its solid ones are
// drawn after every other row's, inside one `ground blend` span, opened only
// when there is one, so their cost is read apart.
//
// It holds nothing between frames: every row's entry is looked up in the
// store each frame, and a held part is the group's, given back by _run's
// rewind. The walk is linear in the model table, as every walk in
// draw_system.c is; with a blending row it walks the table twice, the second
// time looking again only at the blending rows.
#include "draw_models.h"
#include "models_store.h"

#include <3d/model_component.h>
#include <3d/model_levels.h>
#include <base/assert.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

const voe_3d_model_entry *voe_3d_draw_models_entry(const voe_ecs_world *world,
						   const voe_3d_models *models,
						   uint32_t row)
{
	const voe_3d_model_entry *entry;

	VOE_BASE_ASSERT(row < voe_3d_model_count(world), "a model row past the table");
	if (models == NULL ||
	    voe_scene_transform_get(world, voe_3d_model_entities(world)[row]) == NULL)
		return NULL;
	entry = voe_3d_models_find(models, voe_3d_model_rows(world)[row].path);
	if (entry == NULL || !entry->loaded)
		return NULL;
	VOE_BASE_ASSERT(entry->part_count <= VOE_3D_MODEL_PARTS,
			"a model with more parts than the store holds");
	return entry;
}

bool voe_3d_draw_models_is_landscape(const voe_3d_models *models,
				     const voe_3d_model_entry *entry)
{
	voe_3d_models_terrain terrain;

	VOE_BASE_ASSERT(models != NULL && entry != NULL,
			"asking a landscape of no store or entry");
	VOE_BASE_ASSERT(entry->loaded, "asking a landscape of an unloaded entry");
	return voe_3d_models_terrain_of(models, entry, &terrain);
}

// Every loaded part of every model row, which the world's blended group is
// sized for as it is for every mesh and panel, a fading row's held parts
// among them, and VOE_3D_LANDSCAPE_NODES for a landscape row. Nought with no
// store.
uint32_t voe_3d_draw_models_part_count(const voe_ecs_world *world,
				       const voe_3d_models *models)
{
	uint32_t parts = 0;

	VOE_BASE_ASSERT(world != NULL, "counting parts in no world");
	if (models == NULL)
		return 0;
	for (uint32_t row = 0; row < voe_3d_model_count(world); row++) {
		const voe_3d_model_entry *entry =
			voe_3d_draw_models_entry(world, models, row);

		if (entry != NULL)
			parts += voe_3d_draw_models_is_landscape(models, entry) ?
					 VOE_3D_LANDSCAPE_NODES :
					 entry->part_count;
	}
	VOE_BASE_ASSERT(parts <= voe_3d_model_count(world) * VOE_3D_LANDSCAPE_NODES,
			"more parts than the rows can wear");
	return parts;
}

// One part's held or drawn entry: white, since a model has no shape
// (ADR-0191), or with `fading` its `faded` record at alpha 1 − fade (0336).
static struct voe_3d_deferred model_part_entry(const voe_3d_model_part *piece,
					       const voe_scene_transform *drawn,
					       bool fading, float fade,
					       voe_math_double3 eye)
{
	voe_3d_material material = piece->material;
	struct voe_3d_deferred entry = { .panel = false };

	VOE_BASE_ASSERT(!fading || (fade > 0.0f && fade < 1.0f),
			"a fading part outside 0 and 1");
	if (fading)
		material.shading = piece->faded;
	entry.mesh.geometry = piece->geometry;
	entry.mesh.object = voe_3d_draw_group_object_of(drawn, &material, NULL, NULL,
							eye);
	entry.mesh.two_sided = material.double_sided;
	if (fading)
		entry.mesh.object.colour.w = 1.0f - fade;
	VOE_BASE_ASSERT(entry.mesh.object.colour.w > 0.0f,
			"a drawn part with no alpha");
	return entry;
}

// What one row draws this frame: its entry, place, level and fade, and the
// ground it blends with at `reach`, NULL when it blends with none.
struct row_draw {
	const voe_3d_model_entry *model;
	voe_scene_transform drawn;
	voe_3d_model_level_choice choice;
	const struct voe_3d_ground *ground;
	float reach;
	float fade;
	bool fading;
};

// Part `part` of `row`'s model at `level` drawn now, solid, keeping `dither`'s
// share of its pixels (render/device.h) and filled with its ground; true and
// nothing drawn where the level lacks the part.
static bool draw_part_now(voe_render_device *device, const voe_3d_frame *frame,
			  const struct row_draw *row, uint32_t part,
			  uint32_t level, float dither)
{
	voe_3d_model_part piece = voe_3d_draw_group_part_worn(
		frame->models, row->model, part, level);
	struct voe_3d_deferred entry;

	VOE_BASE_ASSERT(!row->fading, "a fading part drawn now");
	if (piece.geometry.generation == 0)
		return true;
	entry = model_part_entry(&piece, &row->drawn, false, 0.0f, frame->eye);
	entry.mesh.object.dither = dither;
	if (row->ground != NULL)
		voe_3d_draw_ground_fill(row->ground, row->reach,
					&entry.mesh.object);
	return voe_3d_draw_group_mesh_draw(device, &entry, false);
}

// Whether `cull` sees `model`'s sphere under `drawn` about `eye`, as its parts
// are drawn; NULL sees everything.
static bool row_seen(const struct voe_3d_cull *cull,
		     const voe_3d_model_entry *model,
		     const voe_scene_transform *drawn, voe_math_double3 eye)
{
	voe_math_float4x4 place;

	VOE_BASE_ASSERT(model != NULL && drawn != NULL,
			"culling no model or no place");
	if (cull == NULL)
		return true;
	place = voe_scene_transform_matrix(*drawn, eye);
	return voe_3d_cull_sees(
		cull, voe_math_float4x4_transform_point(place, model->centre),
		voe_3d_cull_radius(place, model->radius));
}

// Row `row`'s draw into `out`; false when it draws nothing: gone, left out, no
// entry, a landscape, or a sphere `cull` cannot see, the last counted skipped
// in `counts` and a drawn row counted drawn. NULL `counts` counts nothing, for
// the ground walk's second look. A fade is read once: at or above 1 nothing
// is drawn, between 0 and 1 every part is held blended, at or below 0 or not a
// number as a solid row (0336 point 3).
static bool row_of(const voe_ecs_world *world, const voe_3d_frame *frame,
		   const struct voe_3d_cull *cull,
		   const struct voe_3d_grounds *grounds, uint32_t row,
		   voe_3d_view_counts *counts, struct row_draw *out)
{
	const voe_3d_model *placed = &voe_3d_model_rows(world)[row];
	const voe_ecs_entity owner = voe_3d_model_entities(world)[row];

	VOE_BASE_ASSERT(grounds != NULL && out != NULL,
			"a row drawn with no grounds or into nothing");
	out->fade = placed->fade;
	// False for a NaN as for nought, so either draws as ever.
	out->fading = out->fade > 0.0f && out->fade < 1.0f;
	if (out->fade >= 1.0f ||
	    voe_3d_draw_group_left_out(world, owner, frame->hidden))
		return false;
	out->model = voe_3d_draw_models_entry(world, frame->models, row);
	if (out->model == NULL ||
	    voe_3d_draw_models_is_landscape(frame->models, out->model))
		return false;
	out->drawn = voe_scene_transform_between(world, owner, frame->lag);
	if (!row_seen(cull, out->model, &out->drawn, frame->eye)) {
		if (counts != NULL)
			counts->skipped++;
		return false;
	}
	if (counts != NULL)
		counts->drawn++;
	out->choice = voe_3d_model_level_choose(out->model, &out->drawn,
						frame->eye, &frame->view);
	// The ground under its world origin about the eye (0481 point 3).
	out->ground = placed->blend_with_ground ?
			      voe_3d_draw_ground_under(
				      grounds,
				      voe_math_float4x4_transform_point(
					      voe_scene_transform_matrix(
						      out->drawn, frame->eye),
					      (voe_math_float3){ 0 })) :
			      NULL;
	out->reach = placed->blend_height;
	VOE_BASE_ASSERT(out->fading || out->choice.shown >= 1.0f ||
				out->choice.level >= 1,
			"level 0 dissolving into no level before it");
	return true;
}

// `row`'s parts: with `ground_walk` false, its blended or fading ones held in
// `world_blended`, filled, and its solid ones drawn now unless it has a ground,
// `waiting` then set; with `ground_walk` true only those solid ones. False
// when a draw is refused.
static bool draw_row(voe_render_device *device, const voe_3d_frame *frame,
		     struct voe_3d_draw_group *world_blended,
		     const struct row_draw *row, bool ground_walk, bool *waiting)
{
	const bool dissolving = !row->fading && row->choice.shown < 1.0f;
	const float shown = dissolving ? row->choice.shown : 0.0f;

	VOE_BASE_ASSERT(row->model != NULL && waiting != NULL,
			"drawing no row or waiting on nothing");
	VOE_BASE_ASSERT(!ground_walk || row->ground != NULL,
			"a row with no ground in the ground walk");
	for (uint32_t part = 0; part < row->model->part_count; part++) {
		// Its own record, or the one its parts file names; the material
		// is the same at every level.
		voe_3d_model_part piece = voe_3d_draw_group_part_worn(
			frame->models, row->model, part, row->choice.level);

		if (row->fading ||
		    piece.material.alpha_mode == VOE_RENDER_ALPHA_BLENDED) {
			struct voe_3d_deferred entry;

			if (ground_walk || piece.geometry.generation == 0)
				continue;
			entry = model_part_entry(&piece, &row->drawn,
						 row->fading, row->fade,
						 frame->eye);
			if (row->ground != NULL)
				voe_3d_draw_ground_fill(row->ground, row->reach,
							&entry.mesh.object);
			voe_3d_draw_group_hold(world_blended, entry,
					       entry.mesh.object.world,
					       frame->view.view);
			continue;
		}
		if (!ground_walk && row->ground != NULL) {
			*waiting = true;
			continue;
		}
		if (!draw_part_now(device, frame, row, part, row->choice.level,
				   shown))
			return false;
		if (dissolving &&
		    !draw_part_now(device, frame, row, part,
				   row->choice.level - 1, -shown))
			return false;
	}
	return true;
}

// Two walks: every row as ever, a blending row's solid parts left waiting,
// then those inside the one `ground blend` span when any waits (0481 point 5).
bool voe_3d_draw_models(voe_ecs_world *world, voe_render_device *device,
			const voe_3d_frame *frame,
			struct voe_3d_draw_group *world_blended,
			const struct voe_3d_cull *cull,
			const struct voe_3d_grounds *grounds,
			voe_3d_view_counts *counts)
{
	const voe_3d_model *rows;
	bool waiting = false;
	bool drawn = true;

	VOE_BASE_ASSERT(world != NULL && frame != NULL && world_blended != NULL,
			"drawing models with no world, frame or group");
	VOE_BASE_ASSERT(counts != NULL && grounds != NULL,
			"drawing models with nothing to count in or no grounds");
	if (frame->models == NULL)
		return true;
	rows = voe_3d_model_rows(world);
	for (uint32_t row = 0; row < voe_3d_model_count(world); row++) {
		struct row_draw draw;

		if (row_of(world, frame, cull, grounds, row, counts, &draw) &&
		    !draw_row(device, frame, world_blended, &draw, false,
			      &waiting))
			return false;
	}
	if (!waiting)
		return true;
	voe_render_frame_span_begin(device, "ground blend");
	for (uint32_t row = 0; drawn && row < voe_3d_model_count(world); row++) {
		struct row_draw draw;

		if (rows[row].blend_with_ground &&
		    row_of(world, frame, cull, grounds, row, NULL, &draw) &&
		    draw.ground != NULL && !draw.fading)
			drawn = draw_row(device, frame, world_blended, &draw,
					 true, &waiting);
	}
	voe_render_frame_span_end(device);
	return drawn;
}
