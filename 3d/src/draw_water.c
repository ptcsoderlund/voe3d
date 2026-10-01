// The water's draws: each water row's entity matrix about the eye, the store's
// quad turned to face +Y and scaled to the row's width and length, and the
// row's colour, waves and sky in the object record, held in the world's blended
// group with the store's one water shading record.
//
// `waves` IS (height, length, seconds, deep), the clock the waves row's,
// narrowed to float here because it is kept below 60 s (3d/water_component.h);
// `sky` is the row's sky with w nought. render/device.h's voe_render_object
// says how the shader reads both.
//
// Constraints: every water row is walked every frame, two lookups each (its
// transform and its waves).
#include "draw_water.h"

#include <3d/normal_matrix.h>
#include <3d/water_component.h>
#include <base/assert.h>
#include <ecs/component.h>
#include <math/float4x4.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

// Whether the world registered the water table: a walk of the types, as
// draw_particles.c's, because asking for an unregistered key asserts.
static bool has_water(const voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "looking for water in no world");
	for (uint32_t i = 0; i < voe_ecs_component_type_count(world); i++) {
		voe_ecs_type type = voe_ecs_component_type_at(world, i);

		if (voe_ecs_component_key(world, type) == &voe_3d_water_key)
			return true;
	}
	return false;
}

uint32_t voe_3d_draw_water_count(const voe_ecs_world *world,
				 const voe_3d_models *models)
{
	VOE_BASE_ASSERT(world != NULL, "counting water in no world");
	if (models == NULL || voe_3d_models_water(models) == NULL ||
	    !has_water(world))
		return 0;
	return voe_3d_water_count(world);
}

// The quad's model space to the water's own: scaled to width by length in XY,
// then a quarter turn about -X, so +Z (the quad's face) goes to +Y and +Y to -Z.
static voe_math_float4x4 turned_and_scaled(const voe_3d_water *water)
{
	const voe_math_quat down = { -0.70710678f, 0.0f, 0.0f, 0.70710678f };
	voe_math_float4x4 local = voe_math_float4x4_mul(
		voe_math_float4x4_from_quat(down),
		voe_math_float4x4_from_scale((voe_math_float3){
			water->width, water->length, 1.0f }));

	VOE_BASE_ASSERT(local.m[3][3] == 1.0f, "the quad's turn is not affine");
	return local;
}

// One water's record: the world and normal matrices, the water record, and the
// row's colour, waves at `seconds` and sky.
static voe_render_object object_of(const voe_3d_frame *frame,
				   const voe_3d_model_part *part,
				   const voe_scene_transform *drawn,
				   const voe_3d_water *water, double seconds)
{
	voe_render_object object = { 0 };

	VOE_BASE_ASSERT(seconds >= 0.0 && seconds < 60.0,
			"a waves clock outside its minute");
	object.world = voe_math_float4x4_mul(
		voe_scene_transform_matrix(*drawn, frame->eye),
		turned_and_scaled(water));
	object.normal = voe_3d_normal_matrix(object.world);
	object.shading = part->material.shading.index;
	object.colour = (voe_math_float4){ water->colour.x, water->colour.y,
					   water->colour.z, 1.0f };
	object.waves = (voe_math_float4){ water->wave_height, water->wave_length,
					  (float)seconds, water->deep };
	object.sky = (voe_math_float4){ water->sky.x, water->sky.y, water->sky.z,
					0.0f };
	return object;
}

uint32_t voe_3d_draw_water_hold(const voe_ecs_world *world,
				const voe_3d_frame *frame,
				struct voe_3d_draw_group *world_blended)
{
	const voe_3d_model_part *part;
	const voe_3d_water *rows;
	const voe_ecs_entity *owners;
	uint32_t held = 0;

	VOE_BASE_ASSERT(world != NULL && frame != NULL && world_blended != NULL,
			"drawing water with no world, frame or group");
	if (voe_3d_draw_water_count(world, frame->models) == 0)
		return 0;
	part = voe_3d_models_water(frame->models);
	rows = voe_3d_water_rows(world);
	owners = voe_3d_water_entities(world);
	for (uint32_t row = 0; row < voe_3d_water_count(world); row++) {
		const voe_3d_waves *waves;
		voe_scene_transform drawn;
		struct voe_3d_deferred entry = { .panel = false };

		if (voe_3d_draw_group_is_the_same_entity(owners[row], frame->hidden))
			continue;
		waves = voe_3d_waves_get(world, owners[row]);
		if (waves == NULL ||
		    voe_scene_transform_get(world, owners[row]) == NULL)
			continue;
		drawn = voe_scene_transform_between(world, owners[row], frame->lag);
		entry.mesh.geometry = part->geometry;
		entry.mesh.object = object_of(frame, part, &drawn, &rows[row],
					      waves->seconds);
		voe_3d_draw_group_hold(world_blended, entry, entry.mesh.object.world,
				       frame->view.view);
		held++;
	}
	VOE_BASE_ASSERT(held <= voe_3d_water_count(world), "more water than rows");
	return held;
}
