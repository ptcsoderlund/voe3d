// The frame's probe bounce and this step's stale spheres (ADR-0326 points 2, 4
// and 8), and the relight's own sun maps (0329 point 2). The contract is
// draw_bounce.h's; voe_3d_draw_system_shadows in 3d/draw_system.h states what
// the caller pays for it.
//
// THE BOUNCE TAKES EVERY SUN (0357 points 1 and 4): the first with row 0's
// bounces and strength, the frame's further lights as the begin's `more`, and
// each sun whose row casts draws its own map, sun i at its own direction.
//
// A CASTER MOVED WHEN LAG 1 AND LAG 0 DIFFER: its position by more than a
// millimetre on an axis, its rotation by more than 1e-4 off |q0 · q1| = 1, so q
// and −q are one rotation, or its scale by more than 1e-4 on an axis.
//
// A STALE SPHERE IS THE CASTER'S OWN SIZE (0389 point 7): half its world box's
// diagonal, the box grown as the still casters' is; render adds the reach. A
// moved caster marks one at each lag, a new one (never remembered, in a world
// that remembers) and one the shape system changed one where it is.
//
// THE CASTERS WEAR THEIR SHAPE'S COLOUR in the probes' pictures, as in the
// view: the walk is draw_shadows.c's, and it once drew every shape white (bug 01).
// A capture draws every caster; sun i's map only those the blockers holding
// sun i hold, as its cascades do (0361 point 2).
//
// THE GRID IS FITTED TO THE LEVEL, NOT THE EYE (0331, 0332): the still casters'
// box goes to voe_3d_bounce_grid_fit, and its spacing to the begin.
//
// THEN THE NESTS, COARSE TO FINE (0389 points 1 to 3): each nest finer than
// the level grid, volume i + 1 for nest i, placed from where render last put
// it. Every volume takes the same lights, blockers and stale spheres, its own
// sun maps, and capture passes from one budget, each finer one keeping one.
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
// shadows call takes no arena. Sixty-four spheres, each out to a probe's reach,
// already queue more probes than VOE_RENDER_BOUNCE_CAPTURE_PASSES ×
// VOE_RENDER_BOUNCE_CAPTURE capture in a frame, so more would capture nothing
// sooner; an arena on the frame would lift it. The walk is linear over the mesh
// and model tables, as the cascades' is. Whether the world remembers is told
// by a remembered transform, scene saying no more; with no previous table that
// asks of every transform once a call, which a scene query would lift.
#include "draw_bounce.h"
#include "draw_group.h"

#include <3d/bounce_grid.h>
#include <3d/mesh_component.h>
#include <3d/model_component.h>
#include <3d/models.h>
#include <3d/shape_component.h>
#include <3d/shape_system.h>
#include <base/assert.h>
#include <scene/light_component.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <math.h>

#define STALE_ROOM 64

// Whether `entity` stands somewhere else, turned or scaled, than a step ago.
static bool moved(const voe_ecs_world *world, voe_ecs_entity entity)
{
	voe_scene_transform now = voe_scene_transform_between(world, entity, 0.0f);
	voe_scene_transform then = voe_scene_transform_between(world, entity, 1.0f);
	float dot = now.rotation.x * then.rotation.x +
		    now.rotation.y * then.rotation.y +
		    now.rotation.z * then.rotation.z +
		    now.rotation.w * then.rotation.w;

	VOE_BASE_ASSERT(isfinite(dot), "a rotation that is not a number");
	return fabs(now.position.x - then.position.x) > 1e-3 ||
	       fabs(now.position.y - then.position.y) > 1e-3 ||
	       fabs(now.position.z - then.position.z) > 1e-3 ||
	       1.0f - fabsf(dot) > 1e-4f ||
	       fabsf(now.scale.x - then.scale.x) > 1e-4f ||
	       fabsf(now.scale.y - then.scale.y) > 1e-4f ||
	       fabsf(now.scale.z - then.scale.z) > 1e-4f;
}

// A box as it grows; `any` false until a caster is in it.
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

// One call's marking: what it reads, whether the world remembers, and the
// spheres so far.
struct marking {
	const voe_ecs_world *world;
	const voe_render_device *device;
	const voe_3d_frame *frame;
	bool remembers;
	voe_math_float4 *spheres;
	uint32_t count;
	uint32_t room;
};

// A caster's world bounding radius under `placed`: half its world box's
// diagonal, the box of `geometry` when `model` is NULL, else of every casting
// part of `model`; nought when no id names anything.
static float radius(const voe_render_device *device, voe_render_geometry geometry,
		    const voe_3d_model_entry *model,
		    const voe_scene_transform *placed)
{
	struct still_box box = { .any = false };

	if (model == NULL)
		grow(&box, device, geometry, placed);
	else
		for (uint32_t part = 0; part < model->part_count; part++)
			if (voe_3d_draw_casts(&model->parts[part].material))
				grow(&box, device, model->parts[part].geometry,
				     placed);
	if (!box.any)
		return 0.0f;
	return (float)(0.5 * sqrt((box.max.x - box.min.x) * (box.max.x - box.min.x) +
				  (box.max.y - box.min.y) * (box.max.y - box.min.y) +
				  (box.max.z - box.min.z) * (box.max.z - box.min.z)));
}

// The spheres of caster `entity`, as many as fit: one at each lag when it
// moved; else one where it is when it is new or its shape changed; else none.
// A caster whose ids name nothing has no size and marks nothing.
static void mark(struct marking *marking, voe_ecs_entity entity,
		 voe_render_geometry geometry, const voe_3d_model_entry *model)
{
	const float lags[2] = { 1.0f, 0.0f };
	bool both = moved(marking->world, entity);
	bool one = (marking->remembers &&
		    !voe_scene_transform_remembered(marking->world, entity)) ||
		   voe_3d_shape_changed(marking->world, entity);

	VOE_BASE_ASSERT(marking->count <= marking->room,
			"more stale spheres than room");
	if (!both && !one)
		return;
	for (uint32_t i = both ? 0 : 1; i < 2 && marking->count < marking->room;
	     i++) {
		voe_scene_transform placed =
			voe_scene_transform_between(marking->world, entity, lags[i]);
		float w = radius(marking->device, geometry, model, &placed);

		if (w <= 0.0f)
			continue;
		marking->spheres[marking->count++] = (voe_math_float4){
			(float)(placed.position.x - marking->frame->eye.x),
			(float)(placed.position.y - marking->frame->eye.y),
			(float)(placed.position.z - marking->frame->eye.z), w
		};
	}
	VOE_BASE_ASSERT(marking->count <= marking->room,
			"more stale spheres than room");
}

// Whether the world has a previous table, told by a transform remembered in
// it: scene says whether an entity was, not whether the table is registered.
static bool remembers(const voe_ecs_world *world)
{
	const voe_ecs_entity *owners = voe_scene_transform_entities(world);

	for (uint32_t row = 0; row < voe_scene_transform_count(world); row++)
		if (voe_scene_transform_remembered(world, owners[row]))
			return true;
	return false;
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

// The model casters' spheres after the meshes'.
static void stale_models(struct marking *marking)
{
	const voe_3d_frame *frame = marking->frame;
	const voe_3d_model *rows = voe_3d_model_rows(marking->world);
	const voe_ecs_entity *owners = voe_3d_model_entities(marking->world);

	VOE_BASE_ASSERT(frame->models != NULL, "casting models from no store");
	for (uint32_t row = 0; row < voe_3d_model_count(marking->world) &&
			       marking->count < marking->room;
	     row++) {
		if (voe_3d_draw_group_is_the_same_entity(owners[row], frame->hidden) ||
		    voe_scene_transform_get(marking->world, owners[row]) == NULL ||
		    !model_casts(frame, &rows[row]))
			continue;
		mark(marking, owners[row], (voe_render_geometry){ 0 },
		     voe_3d_models_find(frame->models, rows[row].path));
	}
}

uint32_t voe_3d_bounce_stale(const voe_ecs_world *world,
			     const voe_render_device *device,
			     const voe_3d_frame *frame,
			     voe_math_float4 *spheres, uint32_t room)
{
	const voe_3d_mesh *meshes = voe_3d_mesh_rows(world);
	const voe_ecs_entity *owners = voe_3d_mesh_entities(world);
	struct marking marking = { world, device, frame, remembers(world),
				   spheres, 0, room };

	VOE_BASE_ASSERT(world != NULL && device != NULL && frame != NULL &&
				(spheres != NULL || room == 0),
			"marking stale spheres with no world, device, frame or room");
	for (uint32_t row = 0;
	     row < voe_3d_mesh_count(world) && marking.count < room; row++) {
		const voe_3d_material *material;

		if (meshes[row].layer != VOE_3D_LAYER_WORLD ||
		    voe_3d_draw_group_is_the_same_entity(owners[row], frame->hidden))
			continue;
		material = voe_3d_material_get(world, owners[row]);
		if (material == NULL || !voe_3d_draw_casts(material) ||
		    voe_scene_transform_get(world, owners[row]) == NULL)
			continue;
		mark(&marking, owners[row], meshes[row].geometry, NULL);
	}
	if (frame->models != NULL)
		stale_models(&marking);
	VOE_BASE_ASSERT(marking.count <= room, "more stale spheres than room");
	return marking.count;
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

// Sun `sun`'s relight map (0329 point 2, 0357 point 4): the bounce shadow pass
// with the view of `grid` along `direction`, the casters the blockers holding
// the sun hold drawn into it when render opens it (0361 point 2). False when
// the pass or a draw is refused.
static bool draw_sun_map(voe_ecs_world *world, voe_render_device *device,
			 const voe_3d_frame *frame, voe_3d_bounce_grid grid,
			 uint32_t sun, voe_math_float3 direction)
{
	voe_render_view light = voe_3d_bounce_grid_sun(grid, frame->eye,
						       direction);
	bool opened = false;
	bool drawn;

	VOE_BASE_ASSERT(sun <= frame->more_count, "a sun past the frame's");
	if (!voe_render_bounce_shadow_pass_begin(device, sun, &light, &opened))
		return false;
	if (!opened)
		return true;
	drawn = voe_3d_draw_casters(world, device, frame,
				    sun == 0 ? frame->blockers.sun :
					       frame->more_lights[sun - 1].blockers);
	voe_render_pass_end(device);
	return drawn;
}

// One call's bounce: what every volume it begins shares, the lights, blockers
// and stale spheres, and the capture passes it has opened so far.
struct bouncing {
	voe_ecs_world *world;
	voe_render_device *device;
	const voe_3d_frame *frame;
	struct voe_render_bounce_frame shared;
	uint32_t captured;
};

// Volume `volume` at `grid` (0389 points 1 and 3): begun, then capture passes
// while render opens one and this call's count is below `until`, one sun map
// per casting sun, and the relight. False when a pass or a draw is refused.
static bool bounce_volume(struct bouncing *call, voe_3d_bounce_grid grid,
			  uint32_t volume, uint32_t until)
{
	const voe_3d_frame *frame = call->frame;
	struct voe_render_bounce_frame bounce = call->shared;
	bool opened = true;

	VOE_BASE_ASSERT(volume < VOE_RENDER_BOUNCE_VOLUMES &&
				until <= VOE_RENDER_BOUNCE_CAPTURE_PASSES,
			"a volume or a capture budget past render's");
	bounce.spacing = grid.spacing;
	bounce.cell[0] = grid.cell[0];
	bounce.cell[1] = grid.cell[1];
	bounce.cell[2] = grid.cell[2];
	bounce.corner = grid.corner;
	bounce.volume = volume;
	voe_render_bounce_begin(call->device, frame->target, &bounce);
	while (opened && call->captured < until) {
		bool drawn;

		if (!voe_render_bounce_capture_pass_begin(call->device, &opened))
			return false;
		if (!opened)
			break;
		call->captured++;
		drawn = voe_3d_draw_casters(call->world, call->device, frame, 0);
		voe_render_pass_end(call->device);
		if (!drawn)
			return false;
	}
	if (voe_3d_draw_light_casts(call->world, 0) &&
	    !draw_sun_map(call->world, call->device, frame, grid, 0,
			  frame->light.direction))
		return false;
	for (uint32_t i = 0; i < frame->more_count; i++)
		if (voe_3d_draw_light_casts(call->world, i + 1) &&
		    !draw_sun_map(call->world, call->device, frame, grid, i + 1,
				  frame->more_lights[i].light.direction))
			return false;
	voe_render_bounce_relight(call->device);
	VOE_BASE_ASSERT(call->captured <= until, "more capture passes than budgeted");
	return true;
}

bool voe_3d_draw_bounce(voe_ecs_world *world, voe_render_device *device,
			voe_3d_frame *frame)
{
	voe_math_float4 spheres[STALE_ROOM];
	voe_math_double3 min = { 1.0, 1.0, 1.0 };
	voe_math_double3 max = { 0.0, 0.0, 0.0 };
	voe_3d_bounce_grid level;
	struct bouncing call = { world, device, frame, { 0 }, 0 };
	uint32_t first = 0;

	VOE_BASE_ASSERT(world != NULL && device != NULL && frame != NULL,
			"bouncing with no world, device or frame");
	VOE_BASE_ASSERT(!voe_render_pass_is_open(device),
			"the bounce goes between passes, none open");
	// No still caster leaves min above max, which the fit takes as no box.
	(void)voe_3d_bounce_box(world, device, frame, &min, &max);
	level = voe_3d_bounce_grid_fit(min, max, frame->eye);
	call.shared = (struct voe_render_bounce_frame){
		.stale = spheres,
		.stale_count = voe_3d_bounce_stale(world, device, frame, spheres,
						   STALE_ROOM),
		.sun = frame->light,
		.points = frame->points,
		.blockers = frame->blockers,
		.more = { frame->more_lights, frame->more_count },
	};
	if (voe_scene_light_count(world) >= 1) {
		call.shared.sun_bounces = voe_scene_light_rows(world)[0].bounces;
		call.shared.sun_strength =
			voe_scene_light_rows(world)[0].bounce_strength;
	}
	// The nests begun are those finer than the level grid, `first` on.
	while (first < VOE_3D_BOUNCE_NESTS &&
	       voe_3d_bounce_nest_spacing(first) >= level.spacing)
		first++;
	// Each finer volume begun keeps a capture pass: the j-th of n stops at
	// CAPTURE_PASSES − (n − 1 − j), nest i's n − 1 − j being NESTS − 1 − i.
	if (!bounce_volume(&call, level, 0,
			   VOE_RENDER_BOUNCE_CAPTURE_PASSES -
				   (VOE_3D_BOUNCE_NESTS - first)))
		return false;
	for (uint32_t i = first; i < VOE_3D_BOUNCE_NESTS; i++) {
		int32_t cell[3];
		bool placed = voe_render_bounce_placed(device, frame->target, i + 1,
						       cell);
		voe_3d_bounce_grid nest = voe_3d_bounce_grid_nest(
			voe_3d_bounce_nest_spacing(i), placed ? cell : NULL,
			frame->eye);

		if (!bounce_volume(&call, nest, i + 1,
				   VOE_RENDER_BOUNCE_CAPTURE_PASSES -
					   (VOE_3D_BOUNCE_NESTS - 1 - i)))
			return false;
	}
	return true;
}
