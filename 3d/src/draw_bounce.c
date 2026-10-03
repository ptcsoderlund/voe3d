// The frame's probe bounce and this step's stale spheres (ADR-0326 points 2, 4
// and 8). The contract is draw_bounce.h's; voe_3d_draw_system_shadows in
// 3d/draw_system.h states what the caller pays for it.
//
// A CASTER MOVED WHEN LAG 1 AND LAG 0 DIFFER: its position by more than a
// micrometre on an axis, or its rotation by more than 1e-6 off |q0 · q1| = 1,
// so q and −q are one rotation and a normalised copy of a still caster is still.
// A scale change marks nothing.
//
// THE CASTERS WEAR THEIR SHAPE'S COLOUR in the probes' pictures, as in the
// view: the walk is draw_shadows.c's, and it once drew every shape white (bug 01).
//
// Constraints: at most STALE_ROOM spheres a frame, on the stack, because the
// shadows call takes no arena. Sixty-four spheres of 6 m already queue more
// probes than VOE_RENDER_BOUNCE_CAPTURE_PASSES × VOE_RENDER_BOUNCE_CAPTURE
// capture in a frame, so more would capture nothing sooner; an arena on the
// frame would lift it. The walk is linear over the mesh and model tables, as
// the cascades' is.
#include "draw_bounce.h"
#include "draw_group.h"

#include <3d/bounce_grid.h>
#include <3d/mesh_component.h>
#include <3d/model_component.h>
#include <3d/models.h>
#include <base/assert.h>
#include <scene/light_component.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <math.h>

#define STALE_ROOM 64

// Whether `entity` stands somewhere else, or turned, than a step ago.
static bool moved(const voe_ecs_world *world, voe_ecs_entity entity)
{
	voe_scene_transform now = voe_scene_transform_between(world, entity, 0.0f);
	voe_scene_transform then = voe_scene_transform_between(world, entity, 1.0f);
	float dot = now.rotation.x * then.rotation.x +
		    now.rotation.y * then.rotation.y +
		    now.rotation.z * then.rotation.z +
		    now.rotation.w * then.rotation.w;

	VOE_BASE_ASSERT(isfinite(dot), "a rotation that is not a number");
	return fabs(now.position.x - then.position.x) > 1e-6 ||
	       fabs(now.position.y - then.position.y) > 1e-6 ||
	       fabs(now.position.z - then.position.z) > 1e-6 ||
	       1.0f - fabsf(dot) > 1e-6f;
}

// The two spheres of a moved `entity` into `spheres` from `count`, as many as
// fit below `room`; the new count.
static uint32_t mark(const voe_ecs_world *world, const voe_3d_frame *frame,
		     voe_ecs_entity entity, voe_math_float4 *spheres,
		     uint32_t count, uint32_t room)
{
	const float lags[2] = { 1.0f, 0.0f };

	VOE_BASE_ASSERT(count <= room, "more stale spheres than room");
	for (uint32_t i = 0; i < 2 && count < room; i++) {
		voe_math_double3 at =
			voe_scene_transform_between(world, entity, lags[i]).position;

		spheres[count++] = (voe_math_float4){
			(float)(at.x - frame->eye.x), (float)(at.y - frame->eye.y),
			(float)(at.z - frame->eye.z), VOE_3D_BOUNCE_REACH
		};
	}
	VOE_BASE_ASSERT(count <= room, "more stale spheres than room");
	return count;
}

// Whether the model row's entry in the frame's store has a part that casts.
static bool model_casts(const voe_3d_frame *frame, const voe_3d_model *row)
{
	const voe_3d_model_entry *model = voe_3d_models_find(frame->models, row->path);

	VOE_BASE_ASSERT(frame->models != NULL, "casting models from no store");
	if (model == NULL || !model->loaded)
		return false;
	for (uint32_t part = 0; part < model->part_count; part++)
		if (voe_3d_draw_casts(&model->parts[part].material))
			return true;
	return false;
}

// The model casters' spheres after the meshes', from `count`; the new count.
static uint32_t stale_models(const voe_ecs_world *world,
			     const voe_3d_frame *frame, voe_math_float4 *spheres,
			     uint32_t count, uint32_t room)
{
	const voe_3d_model *rows = voe_3d_model_rows(world);
	const voe_ecs_entity *owners = voe_3d_model_entities(world);

	VOE_BASE_ASSERT(frame->models != NULL, "casting models from no store");
	for (uint32_t row = 0; row < voe_3d_model_count(world) && count < room;
	     row++) {
		if (voe_3d_draw_group_is_the_same_entity(owners[row], frame->hidden) ||
		    voe_scene_transform_get(world, owners[row]) == NULL ||
		    !model_casts(frame, &rows[row]) || !moved(world, owners[row]))
			continue;
		count = mark(world, frame, owners[row], spheres, count, room);
	}
	return count;
}

uint32_t voe_3d_bounce_stale(const voe_ecs_world *world,
			     const voe_3d_frame *frame, voe_math_float4 *spheres,
			     uint32_t room)
{
	const voe_3d_mesh *meshes = voe_3d_mesh_rows(world);
	const voe_ecs_entity *owners = voe_3d_mesh_entities(world);
	uint32_t count = 0;

	VOE_BASE_ASSERT(world != NULL && frame != NULL &&
				(spheres != NULL || room == 0),
			"marking stale spheres with no world, frame or room");
	for (uint32_t row = 0; row < voe_3d_mesh_count(world) && count < room;
	     row++) {
		const voe_3d_material *material;

		if (meshes[row].layer != VOE_3D_LAYER_WORLD ||
		    voe_3d_draw_group_is_the_same_entity(owners[row], frame->hidden))
			continue;
		material = voe_3d_material_get(world, owners[row]);
		if (material == NULL || !voe_3d_draw_casts(material) ||
		    voe_scene_transform_get(world, owners[row]) == NULL ||
		    !moved(world, owners[row]))
			continue;
		count = mark(world, frame, owners[row], spheres, count, room);
	}
	if (frame->models != NULL)
		count = stale_models(world, frame, spheres, count, room);
	VOE_BASE_ASSERT(count <= room, "more stale spheres than room");
	return count;
}

bool voe_3d_draw_bounce(voe_ecs_world *world, voe_render_device *device,
			voe_3d_frame *frame)
{
	voe_math_float4 spheres[STALE_ROOM];
	voe_3d_bounce_grid grid;
	struct voe_render_bounce_frame bounce;
	bool opened = true;

	VOE_BASE_ASSERT(world != NULL && device != NULL && frame != NULL,
			"bouncing with no world, device or frame");
	VOE_BASE_ASSERT(!voe_render_pass_is_open(device),
			"the bounce goes between passes, none open");
	grid = voe_3d_bounce_grid_fit(frame->eye);
	bounce = (struct voe_render_bounce_frame){
		.cell = { grid.cell[0], grid.cell[1], grid.cell[2] },
		.corner = grid.corner,
		.stale = spheres,
		.stale_count = voe_3d_bounce_stale(world, frame, spheres, STALE_ROOM),
		.sun = frame->light,
		.shadow = frame->shadow,
		.points = frame->points,
	};
	if (voe_scene_light_count(world) == 1) {
		bounce.sun_bounces = voe_scene_light_rows(world)[0].bounces;
		bounce.sun_strength = voe_scene_light_rows(world)[0].bounce_strength;
	}
	voe_render_bounce_begin(device, frame->target, &bounce);
	while (opened) {
		bool drawn;

		if (!voe_render_bounce_capture_pass_begin(device, &opened))
			return false;
		if (!opened)
			break;
		drawn = voe_3d_draw_casters(world, device, frame);
		voe_render_pass_end(device);
		if (!drawn)
			return false;
	}
	voe_render_bounce_relight(device);
	return true;
}
