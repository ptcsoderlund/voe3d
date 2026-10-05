// The frame's probe bounce and this step's stale spheres (ADR-0326 points 2, 4
// and 8), and the relight's own sun map (0329 point 2). The contract is
// draw_bounce.h's; voe_3d_draw_system_shadows in 3d/draw_system.h states what
// the caller pays for it.
//
// A CASTER MOVED WHEN LAG 1 AND LAG 0 DIFFER: its position by more than a
// micrometre on an axis, or its rotation by more than 1e-6 off |q0 · q1| = 1,
// so q and −q are one rotation and a normalised copy of a still caster is still.
// A scale change marks nothing.
//
// THE CASTERS WEAR THEIR SHAPE'S COLOUR in the probes' pictures, as in the
// view: the walk is draw_shadows.c's, and it once drew every shape white (bug 01).
//
// THE GRID IS FITTED TO THE LEVEL, NOT THE EYE (0331, 0332): the still casters'
// box goes to voe_3d_bounce_grid_fit, and its spacing to the begin and the
// stale spheres, which are the larger of VOE_3D_BOUNCE_REACH and 3 cells.
//
// THE STILL CASTERS' BOX IS THE LEVEL (0332 point 1): the world box of every
// caster draw_shadows.c draws that did not move this step, each geometry's own
// box under its transform at lag 0. Still casters only, because a flying shell
// or a dragged box would stretch the grid fitted to it. In double, the corners
// scaled, turned and moved there and never through the eye-relative float
// matrix, so where the eye stands cannot move the box by a rounding.
//
// THE BOUNCE KEEPS TO THE FRAME'S BLOCKERS (0347 point 4): the begin carries
// `frame->blockers`, filled by voe_3d_draw_system_light_blockers before this.
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
#include <3d/shape_component.h>
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

// The two spheres of a moved `entity`, of radius `reach`, into `spheres` from
// `count`, as many as fit below `room`; the new count.
static uint32_t mark(const voe_ecs_world *world, const voe_3d_frame *frame,
		     voe_ecs_entity entity, float reach,
		     voe_math_float4 *spheres, uint32_t count, uint32_t room)
{
	const float lags[2] = { 1.0f, 0.0f };

	VOE_BASE_ASSERT(count <= room, "more stale spheres than room");
	for (uint32_t i = 0; i < 2 && count < room; i++) {
		voe_math_double3 at =
			voe_scene_transform_between(world, entity, lags[i]).position;

		spheres[count++] = (voe_math_float4){
			(float)(at.x - frame->eye.x), (float)(at.y - frame->eye.y),
			(float)(at.z - frame->eye.z), reach
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
			     const voe_3d_frame *frame, float reach,
			     voe_math_float4 *spheres, uint32_t count,
			     uint32_t room)
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
		count = mark(world, frame, owners[row], reach, spheres, count,
			     room);
	}
	return count;
}

uint32_t voe_3d_bounce_stale(const voe_ecs_world *world,
			     const voe_3d_frame *frame, float spacing,
			     voe_math_float4 *spheres, uint32_t room)
{
	const voe_3d_mesh *meshes = voe_3d_mesh_rows(world);
	const voe_ecs_entity *owners = voe_3d_mesh_entities(world);
	float reach = fmaxf(VOE_3D_BOUNCE_REACH, 3.0f * spacing);
	uint32_t count = 0;

	VOE_BASE_ASSERT(world != NULL && frame != NULL &&
				(spheres != NULL || room == 0),
			"marking stale spheres with no world, frame or room");
	VOE_BASE_ASSERT(spacing > 0.0f && isfinite(spacing),
			"stale spheres at no spacing");
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
		count = mark(world, frame, owners[row], reach, spheres, count,
			     room);
	}
	if (frame->models != NULL)
		count = stale_models(world, frame, reach, spheres, count, room);
	VOE_BASE_ASSERT(count <= room, "more stale spheres than room");
	return count;
}

// The still casters' box as it grows; `any` false until a caster is in it.
struct still_box {
	voe_math_double3 min;
	voe_math_double3 max;
	bool any;
};

// `geometry`'s own box under `placed` into `box`: its eight corners scaled,
// turned and moved in double. Nothing when the id names nothing.
static void grow(struct still_box *box, const voe_render_device *device,
		 voe_render_geometry geometry, const voe_scene_transform *placed)
{
	double x = placed->rotation.x, y = placed->rotation.y;
	double z = placed->rotation.z, w = placed->rotation.w;
	const double turn[3][3] = {
		{ 1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w) },
		{ 2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w) },
		{ 2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y) },
	};
	voe_math_float3 low;
	voe_math_float3 high;

	VOE_BASE_ASSERT(box != NULL && placed != NULL, "growing no box");
	if (!voe_render_geometry_box(device, geometry, &low, &high))
		return;
	for (uint32_t corner = 0; corner < 8; corner++) {
		const double own[3] = {
			(double)placed->scale.x * ((corner & 1) ? high.x : low.x),
			(double)placed->scale.y * ((corner & 2) ? high.y : low.y),
			(double)placed->scale.z * ((corner & 4) ? high.z : low.z),
		};
		double turned[3];
		voe_math_double3 at;

		for (uint32_t axis = 0; axis < 3; axis++)
			turned[axis] = turn[axis][0] * own[0] +
				       turn[axis][1] * own[1] +
				       turn[axis][2] * own[2];
		at = (voe_math_double3){ placed->position.x + turned[0],
					 placed->position.y + turned[1],
					 placed->position.z + turned[2] };
		if (!box->any)
			box->min = box->max = at;
		box->any = true;
		box->min = (voe_math_double3){ fmin(box->min.x, at.x),
					       fmin(box->min.y, at.y),
					       fmin(box->min.z, at.z) };
		box->max = (voe_math_double3){ fmax(box->max.x, at.x),
					       fmax(box->max.y, at.y),
					       fmax(box->max.z, at.z) };
	}
	VOE_BASE_ASSERT(box->any, "a corner that grew nothing");
}

// The still model casters' parts into `box`, as draw_shadows.c chooses them.
static void box_models(const voe_ecs_world *world,
		       const voe_render_device *device,
		       const voe_3d_frame *frame, struct still_box *box)
{
	const voe_3d_model *rows = voe_3d_model_rows(world);
	const voe_ecs_entity *owners = voe_3d_model_entities(world);

	VOE_BASE_ASSERT(frame->models != NULL, "casting models from no store");
	for (uint32_t row = 0; row < voe_3d_model_count(world); row++) {
		const voe_3d_model_entry *model;
		voe_scene_transform placed;

		if (!rows[row].cast_shadows ||
		    voe_3d_draw_group_is_the_same_entity(owners[row], frame->hidden) ||
		    voe_scene_transform_get(world, owners[row]) == NULL)
			continue;
		model = voe_3d_models_find(frame->models, rows[row].path);
		if (model == NULL || !model->loaded || moved(world, owners[row]))
			continue;
		placed = voe_scene_transform_between(world, owners[row], 0.0f);
		for (uint32_t part = 0; part < model->part_count; part++)
			if (voe_3d_draw_casts(&model->parts[part].material))
				grow(box, device, model->parts[part].geometry, &placed);
	}
	VOE_BASE_ASSERT(!box->any || box->min.x <= box->max.x, "a box turned out");
}

bool voe_3d_bounce_box(const voe_ecs_world *world,
		       const voe_render_device *device,
		       const voe_3d_frame *frame, voe_math_double3 *min,
		       voe_math_double3 *max)
{
	const voe_3d_mesh *meshes = voe_3d_mesh_rows(world);
	const voe_ecs_entity *owners = voe_3d_mesh_entities(world);
	struct still_box box = { .any = false };
	voe_ecs_type shapes;
	bool has_shapes = voe_3d_draw_group_shape_type(world, &shapes);

	VOE_BASE_ASSERT(world != NULL && device != NULL && frame != NULL &&
				min != NULL && max != NULL,
			"boxing with no world, device, frame or corners");
	for (uint32_t row = 0; row < voe_3d_mesh_count(world); row++) {
		const voe_3d_material *material;
		const voe_3d_shape *shape;
		voe_scene_transform placed;

		if (meshes[row].layer != VOE_3D_LAYER_WORLD ||
		    voe_3d_draw_group_is_the_same_entity(owners[row], frame->hidden))
			continue;
		material = voe_3d_material_get(world, owners[row]);
		shape = has_shapes ? voe_ecs_component_get(world, shapes,
							   owners[row]) :
				     NULL;
		if (material == NULL || !voe_3d_draw_casts(material) ||
		    (shape != NULL && !shape->cast_shadows) ||
		    voe_scene_transform_get(world, owners[row]) == NULL ||
		    moved(world, owners[row]))
			continue;
		placed = voe_scene_transform_between(world, owners[row], 0.0f);
		grow(&box, device, meshes[row].geometry, &placed);
	}
	if (frame->models != NULL)
		box_models(world, device, frame, &box);
	if (!box.any)
		return false;
	*min = box.min;
	*max = box.max;
	VOE_BASE_ASSERT(min->x <= max->x && min->y <= max->y && min->z <= max->z,
			"a box turned out");
	return true;
}

// The relight's own sun map (0329 point 2): the bounce shadow pass with the sun
// view of `grid`, the casters drawn into it when render opens it. False when
// the pass or a draw is refused.
static bool draw_sun_map(voe_ecs_world *world, voe_render_device *device,
			 const voe_3d_frame *frame, voe_3d_bounce_grid grid)
{
	voe_render_view light = voe_3d_bounce_grid_sun(grid, frame->eye,
						       frame->light.direction);
	bool opened = false;
	bool drawn;

	if (!voe_render_bounce_shadow_pass_begin(device, 0, &light, &opened))
		return false;
	if (!opened)
		return true;
	drawn = voe_3d_draw_casters(world, device, frame);
	voe_render_pass_end(device);
	return drawn;
}

bool voe_3d_draw_bounce(voe_ecs_world *world, voe_render_device *device,
			voe_3d_frame *frame)
{
	voe_math_float4 spheres[STALE_ROOM];
	voe_math_double3 min = { 1.0, 1.0, 1.0 };
	voe_math_double3 max = { 0.0, 0.0, 0.0 };
	voe_3d_bounce_grid grid;
	struct voe_render_bounce_frame bounce;
	bool opened = true;

	VOE_BASE_ASSERT(world != NULL && device != NULL && frame != NULL,
			"bouncing with no world, device or frame");
	VOE_BASE_ASSERT(!voe_render_pass_is_open(device),
			"the bounce goes between passes, none open");
	// No still caster leaves min above max, which the fit takes as no box.
	(void)voe_3d_bounce_box(world, device, frame, &min, &max);
	grid = voe_3d_bounce_grid_fit(min, max, frame->eye);
	bounce = (struct voe_render_bounce_frame){
		.spacing = grid.spacing,
		.cell = { grid.cell[0], grid.cell[1], grid.cell[2] },
		.corner = grid.corner,
		.stale = spheres,
		.stale_count = voe_3d_bounce_stale(world, frame, grid.spacing,
						   spheres, STALE_ROOM),
		.sun = frame->light,
		.points = frame->points,
		.blockers = frame->blockers,
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
	if (voe_3d_draw_light_casts(world) &&
	    !draw_sun_map(world, device, frame, grid))
		return false;
	voe_render_bounce_relight(device);
	return true;
}
