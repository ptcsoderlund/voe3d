// A frame's painted grounds (draw_ground.h; 0481 points 2 and 3): the landscape
// rows draw_system.c's landscape walk draws solid, each with its paint and
// layers asked of the store and the inverse of its world place about the eye;
// the first whose square holds a point; and a model part's record made a
// ground record of one.
//
// Used by draw_system.c's run, which builds the grounds on its scratch before
// the model walk, and by draw_models.c's walk, which finds a blending row's
// ground under its origin and fills each part's record with it.
//
// Constraints: the walk is linear in the model table, as every walk in
// draw_system.c is, and keeps at most VOE_3D_LANDSCAPES_DRAWN grounds, so the
// search under a point is a scan of four. The paint and layer asks repeat
// draw_terrain.c's look_of, which is that file's own; a shared helper there
// would lift the copy.
#include "draw_ground.h"

#include "draw_group.h"
#include "draw_material.h"
#include "draw_models.h"
#include "models_store.h"

#include <3d/model_component.h>
#include <3d/models.h>
#include <assets/landscape.h>
#include <base/assert.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <math.h>

static_assert(VOE_ASSETS_LANDSCAPE_LAYERS ==
		      sizeof(((struct voe_3d_ground *)0)->layers) / sizeof(uint32_t),
	      "a ground names every layer a landscape holds");

// The reach's bounds, metres (0481 point 4).
#define LEAST_REACH 0.05f
#define MOST_REACH 10.0f

// `entry`'s paint and each layer's shading record into `ground`, asked of the
// store now as draw_terrain.c asks them (0402 point 4): a layer whose material
// is not loaded takes the part's own record, layers past the count repeat
// layer 0's. False, nothing named, for a landscape with no layers.
static bool painted(const voe_3d_models *models, const voe_3d_model_entry *entry,
		    const voe_3d_models_terrain *terrain,
		    struct voe_3d_ground *ground)
{
	const voe_assets_landscape *land = entry->landscape;
	const uint32_t own = entry->parts[0].material.shading.index;

	VOE_BASE_ASSERT(land->layer_count <= VOE_ASSETS_LANDSCAPE_LAYERS,
			"more layers than a landscape holds");
	if (land->layer_count == 0)
		return false;
	ground->paint[0] = terrain->paint[0].index + 1;
	ground->paint[1] = terrain->paint[1].index + 1;
	for (uint32_t i = 0; i < land->layer_count; i++) {
		const voe_3d_model_part *worn =
			voe_3d_draw_material_named(models, land->layers[i]);

		ground->layers[i] = worn != NULL ? worn->material.shading.index :
						   own;
	}
	for (uint32_t i = land->layer_count; i < VOE_ASSETS_LANDSCAPE_LAYERS;
	     i++)
		ground->layers[i] = ground->layers[0];
	VOE_BASE_ASSERT(ground->paint[0] != 0, "a painted landscape with no paint");
	return true;
}

// Row `row`, wearing loaded landscape `entry`, as a ground into `out`: false
// when it is unpainted or its world place about the eye has no inverse, which
// draws no node either.
static bool ground_of(const voe_ecs_world *world, const voe_3d_frame *frame,
		      uint32_t row, const voe_3d_model_entry *entry,
		      struct voe_3d_ground *out)
{
	voe_3d_models_terrain terrain;
	voe_math_float4x4 place;
	voe_math_float4x4 inverse;

	VOE_BASE_ASSERT(entry != NULL && out != NULL,
			"a ground of no entry or into nothing");
	if (!voe_3d_models_terrain_of(frame->models, entry, &terrain))
		return false;
	place = voe_scene_transform_matrix(
		voe_scene_transform_between(
			world, voe_3d_model_entities(world)[row], frame->lag),
		frame->eye);
	if (voe_math_float4x4_determinant(place) == 0.0f ||
	    !painted(frame->models, entry, &terrain, out))
		return false;
	inverse = voe_math_float4x4_inverse(place);
	out->row = row;
	out->heights = terrain.heights.index + 1;
	out->side = entry->landscape->size;
	for (uint32_t r = 0; r < 3; r++)
		out->rows[r] = (voe_math_float4){ inverse.m[r][0], inverse.m[r][1],
						  inverse.m[r][2], inverse.m[r][3] };
	VOE_BASE_ASSERT(out->side > 0.0f, "a ground with no side");
	return true;
}

// The landscape rows counted as draw_system.c's landscape walk counts them:
// a fading row takes one of the VOE_3D_LANDSCAPES_DRAWN but is no ground.
struct voe_3d_grounds voe_3d_draw_grounds(const voe_ecs_world *world,
					  const voe_3d_frame *frame,
					  voe_base_arena *arena)
{
	struct voe_3d_grounds out = { 0 };
	struct voe_3d_ground *grounds;
	uint32_t landscapes = 0;

	VOE_BASE_ASSERT(world != NULL && frame != NULL && arena != NULL,
			"grounds of no world, frame or arena");
	if (frame->models == NULL)
		return out;
	grounds = voe_base_arena_push(arena, sizeof(*grounds) *
						     VOE_3D_LANDSCAPES_DRAWN);
	for (uint32_t row = 0; row < voe_3d_model_count(world) &&
			       landscapes < VOE_3D_LANDSCAPES_DRAWN;
	     row++) {
		const float fade = voe_3d_model_rows(world)[row].fade;
		const voe_3d_model_entry *entry;

		if (fade >= 1.0f ||
		    voe_3d_draw_group_left_out(world, voe_3d_model_entities(world)[row],
					       frame->hidden))
			continue;
		entry = voe_3d_draw_models_entry(world, frame->models, row);
		if (entry == NULL ||
		    !voe_3d_draw_models_is_landscape(frame->models, entry))
			continue;
		landscapes++;
		if (fade > 0.0f && fade < 1.0f)
			continue;
		if (ground_of(world, frame, row, entry, &grounds[out.count]))
			out.count++;
	}
	VOE_BASE_ASSERT(out.count <= VOE_3D_LANDSCAPES_DRAWN,
			"more grounds than landscapes drawn");
	out.grounds = out.count > 0 ? grounds : NULL;
	return out;
}

const struct voe_3d_ground *
voe_3d_draw_ground_under(const struct voe_3d_grounds *grounds,
			 voe_math_float3 at)
{
	VOE_BASE_ASSERT(grounds != NULL, "a ground under no grounds");
	VOE_BASE_ASSERT(grounds->count == 0 || grounds->grounds != NULL,
			"grounds counted with none");
	for (uint32_t i = 0; i < grounds->count; i++) {
		const struct voe_3d_ground *ground = &grounds->grounds[i];
		const voe_math_float4 *rows = ground->rows;
		const float half = 0.5f * ground->side;
		const float x = rows[0].x * at.x + rows[0].y * at.y +
				rows[0].z * at.z + rows[0].w;
		const float z = rows[2].x * at.x + rows[2].y * at.y +
				rows[2].z * at.z + rows[2].w;

		if (fabsf(x) <= half && fabsf(z) <= half)
			return ground;
	}
	return NULL;
}

void voe_3d_draw_ground_fill(const struct voe_3d_ground *ground, float reach,
			     voe_render_object *object)
{
	VOE_BASE_ASSERT(ground != NULL && object != NULL,
			"filling with no ground or no record");
	VOE_BASE_ASSERT(ground->heights != 0 && ground->paint[0] != 0,
			"a ground with no heights or paint");
	// False for a NaN as for nought.
	if (!(reach > 0.0f))
		return;
	object->ground = ground->heights;
	object->ground_reach = fminf(fmaxf(reach, LEAST_REACH), MOST_REACH);
	object->paint[0] = ground->paint[0];
	object->paint[1] = ground->paint[1];
	for (uint32_t i = 0; i < VOE_ASSETS_LANDSCAPE_LAYERS; i++)
		object->layers[i] = ground->layers[i];
	for (uint32_t r = 0; r < 3; r++)
		object->ground_rows[r] = ground->rows[r];
	object->terrain.w = ground->side;
	VOE_BASE_ASSERT(object->ground_reach >= LEAST_REACH &&
				object->ground_reach <= MOST_REACH,
			"a reach outside its bounds");
}
