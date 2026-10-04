// A frame carries the world's light blockers (0347 point 2), through
// voe_3d_light_blocker_shape and voe_3d_draw_system_light_blockers.
//
// THE BOX, NO GRAPHICS CARD: a blocker of size (2, 4, 6) at (1, 2, 3), turned a
// quarter about Y and scaled (-1, 0.5, 2), is a box centred there, turned the
// same, of half (1, 1, 6); one 1 m over a parent moved to (20, 0, 0) is centred
// at (20, 1, 0); one with no transform, and a dead one, give none and leave the
// answer untouched.
//
// THE PASS'S RECORDS: a 2 m box at (10, 0, 0) seen from an eye at (4, 0, 0) has
// its sphere at (6, 0, 0) of radius √3, and its rows hold (6, 0, 0) and the face
// at (7, 0, 0) and not (7.1, 0, 0); a turned one holds a point along its long
// side and not one off its short side; one with no transform and one of size 0
// on an axis are left out; a world with no blocker table fills none and is true.
//
// BLOCK AND THE SUN (0350 point 2): a Direct, a Fill and an All give their
// bits in `walls`, `indoors` and neither, by kept index, so one left out shifts
// the bits after it; a light inside a blocker sets its bit in `sun`, outside it
// none, and no light row or no transform is 0.
//
// THE EDITOR'S LINES (0347 point 5): the box through
// voe_3d_collider_marker_quads from an off-axis eye is 12 edges, 48 vertices.
//
// AND THREE PICTURES: a camera four metres over a grey ground cube under a sun,
// a 2 m All over the middle of it. Drawn through _frame, _light_blockers and
// _run with `.blockers` in the pass camera, the centre is black and a corner
// lit. A Direct floating over it under a low sun with a fill: the ground in its
// shadow reads the fill, not black and below sunlit ground clear of it. Then a
// blocker over an empty view with `light_blocker` naming it: a box edge's
// pixel is the outline colour, and with it zeroed it is not. All need a
// graphics card and skip with a reason without one, as
// 3d/tests/point_lights.c does.
#include <3d/collider_marker.h>
#include <3d/draw_system.h>
#include <3d/light_blocker.h>
#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/model_component.h>
#include <3d/panel_component.h>
#include <3d/projection.h>
#include <3d/shape_component.h>
#include <3d/shape_system.h>
#include <base/arena.h>
#include <base/error.h>
#include <ecs/structure.h>
#include <ecs/world.h>
#include <physics/collider_component.h>
#include <render/device.h>
#include <scene/camera_component.h>
#include <scene/camera_system.h>
#include <scene/light_blocker_component.h>
#include <scene/light_blocker_system.h>
#include <scene/light_component.h>
#include <scene/light_system.h>
#include <scene/parent_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <math.h>
#include <stdio.h>

#define SCRATCH (4 * 1024 * 1024)
#define SIDE 32
// A channel at most this is black: a driver's rounding into an sRGB target.
#define TOLERANCE 3

static const voe_scene_transform UNMOVED = { .rotation = { 0.0f, 0.0f, 0.0f,
							   1.0f },
					     .scale = { 1.0f, 1.0f, 1.0f } };

// A quarter turn about +Y: local +Z goes to world +X.
static const voe_math_quat QUARTER = { 0.0f, 0.70710678f, 0.0f, 0.70710678f };

// A world of room for `rows` blockers.
static voe_ecs_world *world_of(voe_base_arena *arena, uint32_t rows)
{
	voe_ecs_limits limits = {
		.entities = rows,
		.component_types = 8,
		.intent_types = 8,
		.structure_requests = 16,
		.structure_bytes = 1024,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);

	voe_scene_transform_register(world, rows);
	voe_scene_parent_register(world, rows);
	voe_scene_light_blocker_register(world, rows);
	return world;
}

// A blocker of `size` and `block`, at `where` unless `placed` is false.
static voe_ecs_entity block_of_blocker(voe_ecs_world *world,
				       voe_math_float3 size, uint32_t block,
				       bool placed, voe_scene_transform where)
{
	voe_ecs_entity blocker = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &blocker));
	if (placed)
		VOE_TEST_CHECK(voe_scene_transform_add(world, blocker, where));
	VOE_TEST_CHECK(voe_scene_light_blocker_add(
		world, blocker,
		(voe_scene_light_blocker){ .size = size, .block = block }));
	return blocker;
}

// An All of `size`, at `where` unless `placed` is false.
static voe_ecs_entity blocker_of(voe_ecs_world *world, voe_math_float3 size,
				 bool placed, voe_scene_transform where)
{
	return block_of_blocker(world, size, VOE_SCENE_LIGHT_BLOCKER_ALL, placed,
				where);
}

static voe_scene_transform placed_at(voe_math_double3 at)
{
	voe_scene_transform where = UNMOVED;

	where.position = at;
	return where;
}

// Whether `record` holds `p`, by its rows alone, the face included.
static bool holds(voe_render_light_blocker record, voe_math_float3 p)
{
	for (int i = 0; i < 3; i++) {
		voe_math_float4 row = record.rows[i];

		if (fabsf(row.x * p.x + row.y * p.y + row.z * p.z + row.w) >
		    1.0f + 1e-5f)
			return false;
	}
	return true;
}

// A scaled, turned blocker; a child of a moved parent; none without a
// transform or alive.
static void the_box_follows_the_transform(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena, 8);
	voe_scene_transform turned = { .position = { 1.0, 2.0, 3.0 },
				       .rotation = QUARTER,
				       .scale = { -1.0f, 0.5f, 2.0f } };
	voe_ecs_entity box = blocker_of(world, (voe_math_float3){ 2, 4, 6 },
					true, turned);
	voe_ecs_entity loose = blocker_of(world, (voe_math_float3){ 1, 1, 1 },
					  false, UNMOVED);
	voe_ecs_entity parent = { 0 };
	voe_ecs_entity child;
	voe_scene_transform moved = UNMOVED;
	voe_physics_shape shape = { 0 };

	VOE_TEST_CHECK(voe_3d_light_blocker_shape(world, box, 0.0f, &shape));
	VOE_TEST_CHECK_INT(shape.kind, VOE_PHYSICS_COLLIDER_BOX);
	VOE_TEST_CHECK(shape.centre.x == 1.0 && shape.centre.y == 2.0 &&
		       shape.centre.z == 3.0);
	VOE_TEST_CHECK_FLOAT(shape.rotation.y, QUARTER.y, 1e-6f);
	VOE_TEST_CHECK_FLOAT(shape.rotation.w, QUARTER.w, 1e-6f);
	VOE_TEST_CHECK_FLOAT(shape.half.x, 1.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(shape.half.y, 1.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(shape.half.z, 6.0f, 1e-6f);

	shape.half.x = 99.0f;
	VOE_TEST_CHECK(!voe_3d_light_blocker_shape(world, loose, 0.0f, &shape));
	VOE_TEST_CHECK_FLOAT(shape.half.x, 99.0f, 0.0f);

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &parent));
	VOE_TEST_CHECK(voe_scene_transform_add(world, parent, UNMOVED));
	child = blocker_of(world, (voe_math_float3){ 1, 1, 1 }, true,
			   placed_at((voe_math_double3){ 0.0, 1.0, 0.0 }));
	VOE_TEST_CHECK(voe_scene_parent_set(world, child, parent));
	voe_ecs_structure_apply(world);
	voe_scene_transform_system_run(world);
	moved.position = (voe_math_double3){ 20.0, 0.0, 0.0 };
	VOE_TEST_CHECK(voe_scene_transform_submit(
		world, (voe_scene_transform_intent){ .entity = parent,
						     .transform = moved }));
	voe_scene_transform_system_run(world);
	VOE_TEST_CHECK(voe_3d_light_blocker_shape(world, child, 0.0f, &shape));
	VOE_TEST_CHECK(shape.centre.x == 20.0 && shape.centre.y == 1.0 &&
		       shape.centre.z == 0.0);

	voe_ecs_entity_destroy(world, box);
	voe_ecs_structure_apply(world);
	shape.half.x = 99.0f;
	VOE_TEST_CHECK(!voe_3d_light_blocker_shape(world, box, 0.0f, &shape));
	VOE_TEST_CHECK_FLOAT(shape.half.x, 99.0f, 0.0f);
}

// A box about the eye, an unplaced one, a flat one and a turned one, in that
// table order.
static void the_table_becomes_the_passs_blockers(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena, 8);
	voe_scene_transform turned =
		placed_at((voe_math_double3){ 4.0, 0.0, 10.0 });
	voe_3d_frame frame = { .eye = { 4.0, 0.0, 0.0 } };

	turned.rotation = QUARTER;
	(void)blocker_of(world, (voe_math_float3){ 2, 2, 2 }, true,
			 placed_at((voe_math_double3){ 10.0, 0.0, 0.0 }));
	(void)blocker_of(world, (voe_math_float3){ 2, 2, 2 }, false, UNMOVED);
	(void)blocker_of(world, (voe_math_float3){ 2, 0, 2 }, true,
			 placed_at((voe_math_double3){ 0.0, 0.0, 0.0 }));
	(void)blocker_of(world, (voe_math_float3){ 2, 2, 4 }, true, turned);

	VOE_TEST_CHECK(voe_3d_draw_system_light_blockers(world, &frame, arena));
	VOE_TEST_CHECK_INT(frame.blockers.count, 2);
	if (frame.blockers.count != 2)
		return;

	voe_render_light_blocker box = frame.blockers.blockers[0];

	VOE_TEST_CHECK_FLOAT(box.sphere.x, 6.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(box.sphere.y, 0.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(box.sphere.z, 0.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(box.sphere.w, sqrtf(3.0f), 1e-5f);
	VOE_TEST_CHECK(holds(box, (voe_math_float3){ 6.0f, 0.0f, 0.0f }));
	VOE_TEST_CHECK(holds(box, (voe_math_float3){ 7.0f, 0.0f, 0.0f }));
	VOE_TEST_CHECK(!holds(box, (voe_math_float3){ 7.1f, 0.0f, 0.0f }));
	VOE_TEST_CHECK(!holds(box, (voe_math_float3){ 6.0f, 1.1f, 0.0f }));

	// Its long local Z lies along world X, about the eye at (0, 0, 10).
	voe_render_light_blocker long_box = frame.blockers.blockers[1];

	VOE_TEST_CHECK(holds(long_box, (voe_math_float3){ 1.9f, 0.0f, 10.0f }));
	VOE_TEST_CHECK(!holds(long_box, (voe_math_float3){ 0.0f, 0.0f, 11.5f }));
}

// A Direct, an unplaced Direct, a Fill and an All: the unplaced one is left
// out, so the Fill is bit 1 and the All, bit 2, is in neither. No light
// table: the sun's mask is 0.
static void the_blocks_are_bits_of_the_kept(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena, 8);
	voe_math_float3 two = { 2, 2, 2 };
	voe_3d_frame frame = { 0 };

	(void)block_of_blocker(world, two, VOE_SCENE_LIGHT_BLOCKER_DIRECT, true,
			       placed_at((voe_math_double3){ 0.0, 0.0, 0.0 }));
	(void)block_of_blocker(world, two, VOE_SCENE_LIGHT_BLOCKER_DIRECT, false,
			       UNMOVED);
	(void)block_of_blocker(world, two, VOE_SCENE_LIGHT_BLOCKER_FILL, true,
			       placed_at((voe_math_double3){ 5.0, 0.0, 0.0 }));
	(void)block_of_blocker(world, two, VOE_SCENE_LIGHT_BLOCKER_ALL, true,
			       placed_at((voe_math_double3){ 10.0, 0.0, 0.0 }));

	VOE_TEST_CHECK(voe_3d_draw_system_light_blockers(world, &frame, arena));
	VOE_TEST_CHECK_INT(frame.blockers.count, 3);
	VOE_TEST_CHECK_INT(frame.blockers.walls, 1u);
	VOE_TEST_CHECK_INT(frame.blockers.indoors, 2u);
	VOE_TEST_CHECK_INT(frame.blockers.sun, 0u);
}

// The sun's mask of a Direct at (10, 0, 0) and an All at (0, 0, 0), seen from an
// eye at (4, 0, 0), with a light row at `at` when `lit`, placed when `placed`.
static uint32_t sun_of(voe_base_arena *arena, bool lit, bool placed,
		       voe_math_double3 at)
{
	voe_ecs_world *world = world_of(arena, 8);
	voe_math_float3 two = { 2, 2, 2 };
	voe_3d_frame frame = { .eye = { 4.0, 0.0, 0.0 } };
	voe_ecs_entity light = { 0 };

	voe_scene_light_register(world, 1);
	(void)block_of_blocker(world, two, VOE_SCENE_LIGHT_BLOCKER_DIRECT, true,
			       placed_at((voe_math_double3){ 10.0, 0.0, 0.0 }));
	(void)block_of_blocker(world, two, VOE_SCENE_LIGHT_BLOCKER_ALL, true,
			       placed_at((voe_math_double3){ 0.0, 0.0, 0.0 }));
	if (lit) {
		VOE_TEST_CHECK(voe_ecs_entity_create(world, &light));
		if (placed)
			VOE_TEST_CHECK(voe_scene_transform_add(
				world, light, placed_at(at)));
		VOE_TEST_CHECK(voe_scene_light_add(
			world, light,
			(voe_scene_light){ .colour = { 1.0f, 1.0f, 1.0f },
					   .intensity = 1.0f }));
	}
	VOE_TEST_CHECK(voe_3d_draw_system_light_blockers(world, &frame, arena));
	return frame.blockers.sun;
}

// A light inside the Direct sets its bit, inside the All the All's, between
// them none; one with no transform, and no light row, are 0.
static void the_sun_is_masked_where_it_stands(voe_base_arena *arena)
{
	VOE_TEST_CHECK_INT(
		sun_of(arena, true, true, (voe_math_double3){ 10.0, 0.5, 0.0 }),
		1u);
	VOE_TEST_CHECK_INT(
		sun_of(arena, true, true, (voe_math_double3){ 0.5, 0.0, 0.0 }),
		2u);
	VOE_TEST_CHECK_INT(
		sun_of(arena, true, true, (voe_math_double3){ 5.0, 0.0, 0.0 }),
		0u);
	VOE_TEST_CHECK_INT(
		sun_of(arena, true, false, (voe_math_double3){ 10.0, 0.0, 0.0 }),
		0u);
	VOE_TEST_CHECK_INT(
		sun_of(arena, false, false, (voe_math_double3){ 0.0, 0.0, 0.0 }),
		0u);
}

// No blocker table, and a table with no rows: none, and true.
static void no_table_fills_none(voe_base_arena *arena)
{
	voe_ecs_limits limits = { .entities = 4,
				  .component_types = 4,
				  .intent_types = 4 };
	voe_ecs_world *bare = voe_ecs_world_new(arena, limits);
	voe_ecs_world *empty = world_of(arena, 4);
	voe_3d_frame frame = { 0 };

	voe_scene_transform_register(bare, 4);
	VOE_TEST_CHECK(voe_3d_draw_system_light_blockers(bare, &frame, arena));
	VOE_TEST_CHECK_INT(frame.blockers.count, 0);
	VOE_TEST_CHECK(frame.blockers.blockers == NULL);
	VOE_TEST_CHECK(voe_3d_draw_system_light_blockers(empty, &frame, arena));
	VOE_TEST_CHECK_INT(frame.blockers.count, 0);
}

// The lens every view here is seen through.
static const voe_scene_camera LENS = { .fov_y = 1.0471976f,
				       .near_plane = 0.1f,
				       .far_plane = 100.0f };

// A blocker's box seen from an eye off every axis is twelve edges' quads.
static void the_box_is_twelve_edges(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena, 2);
	voe_ecs_entity box = blocker_of(
		world, (voe_math_float3){ 2, 2, 2 }, true,
		placed_at((voe_math_double3){ 10.0, 0.0, 0.0 }));
	voe_scene_transform eye = placed_at((voe_math_double3){ 11.0, 2.0, 5.0 });
	voe_render_view view = { 0 };
	voe_physics_shape shape = { 0 };
	voe_3d_outline_mesh mesh = { 0 };

	VOE_TEST_CHECK(voe_3d_view(eye, LENS, 1.0f, &view));
	VOE_TEST_CHECK(voe_3d_light_blocker_shape(world, box, 0.0f, &shape));
	VOE_TEST_CHECK(voe_3d_collider_marker_quads(
		shape, view, eye.position, (voe_platform_size){ SIDE, SIDE },
		2.0f, arena, &mesh));
	VOE_TEST_CHECK_INT(mesh.vertex_count, 12 * 4);
	VOE_TEST_CHECK_INT(mesh.index_count, 12 * 6);
}

// The camera four metres up looking down, `sun` shining `toward`, the ground
// cube whose top is at y = -0.95, and a blocker of `block` and `size` at `at`.
// The light stands at y = 50, outside every blocker, so the sun's mask is 0.
static voe_ecs_world *a_blocked_world(voe_base_arena *arena,
				      const voe_3d_shapes *shapes,
				      voe_math_float3 toward,
				      voe_scene_light sun, uint32_t block,
				      voe_math_float3 size, voe_math_double3 at)
{
	voe_ecs_limits limits = {
		.entities = 8,
		.component_types = 16,
		.intent_types = 16,
		.structure_requests = 16,
		.structure_bytes = 1024,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);
	voe_ecs_entity entity = { 0 };
	voe_scene_transform ground = { .position = { 0.0, -1.0, 0.0 },
				       .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				       .scale = { 20.0f, 0.1f, 20.0f } };

	voe_scene_transform_register(world, 8);
	voe_scene_camera_register(world, 1);
	voe_scene_light_register(world, 1);
	voe_scene_light_blocker_register(world, 1);
	voe_3d_mesh_register(world, 4);
	voe_3d_material_register(world, 4);
	voe_3d_panel_register(world, 1);
	voe_3d_shape_register(world, 4);
	voe_3d_model_register(world, 1);

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){
			.position = { 0.0, 4.0, 0.0 },
			.rotation = { -0.70710678f, 0.0f, 0.0f, 0.70710678f },
			.scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_camera_add(
		world, entity,
		(voe_scene_camera){ .fov_y = 1.0471976f,
				    .near_plane = 0.1f,
				    .far_plane = 100.0f }));

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){
			.position = { 0.0, 50.0, 0.0 },
			.rotation = voe_scene_light_facing(toward),
			.scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_light_add(world, entity, sun));

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(world, entity, ground));
	VOE_TEST_CHECK(voe_3d_shape_add(
		world, entity,
		(voe_3d_shape){ .kind = VOE_3D_SHAPE_CUBE,
				.colour = { 0.5f, 0.5f, 0.5f } }));
	voe_3d_shape_system_run(world, shapes);

	(void)block_of_blocker(world, size, block, true, placed_at(at));
	return world;
}

static const uint8_t *pixel_at(voe_render_picture picture, uint32_t column,
			       uint32_t row)
{
	const uint8_t *pixel =
		picture.pixels + ((size_t)row * picture.width + column) * 4;

	printf("pixel %u %u: %u %u %u\n", column, row, pixel[0], pixel[1],
	       pixel[2]);
	return pixel;
}

// One frame of `world` through _frame, _light_blockers and _run, read back;
// no pixels when it could not be drawn.
static voe_render_picture drawn_blocked(voe_ecs_world *world,
					voe_render_device *device,
					voe_base_arena *arena)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_3d_frame frame = voe_3d_draw_system_frame(world, size, 0.0f);
	voe_render_pass_camera camera;
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;
	bool drawing = false;

	VOE_TEST_CHECK_INT(frame.blockers.count, 0);
	VOE_TEST_CHECK(voe_3d_draw_system_light_blockers(world, &frame, arena));
	VOE_TEST_CHECK_INT(frame.blockers.count, 1);
	VOE_TEST_CHECK_INT(frame.blockers.sun, 0u);
	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return picture;
	camera = (voe_render_pass_camera){ .view = frame.view,
					   .light = frame.light,
					   .blockers = frame.blockers };
	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     &camera));
	voe_3d_draw_system_run(world, device, arena, frame);
	voe_render_pass_end(device);
	VOE_TEST_CHECK(voe_render_frame_end(device));
	VOE_TEST_CHECK(voe_render_target_read(device, VOE_RENDER_TARGET_WINDOW,
					      arena, &picture, &error));
	VOE_TEST_CHECK(picture.pixels != NULL);
	return picture;
}

// A 2 m All about the ground's centre under a slanted sun.
static void a_blocker_keeps_the_sun_out(voe_base_arena *arena,
					voe_render_device *device,
					const voe_3d_shapes *shapes)
{
	voe_ecs_world *world = a_blocked_world(
		arena, shapes, (voe_math_float3){ 0.4f, -1.0f, 0.3f },
		(voe_scene_light){ .colour = { 1.0f, 1.0f, 1.0f },
				   .intensity = 3.0f },
		VOE_SCENE_LIGHT_BLOCKER_ALL, (voe_math_float3){ 2, 2, 2 },
		(voe_math_double3){ 0.0, -1.0, 0.0 });
	voe_render_picture picture = drawn_blocked(world, device, arena);

	if (picture.pixels == NULL)
		return;

	const uint8_t *inside = pixel_at(picture, SIDE / 2, SIDE / 2);
	const uint8_t *corner = pixel_at(picture, 0, 0);

	for (int channel = 0; channel < 3; channel++) {
		VOE_TEST_CHECK(inside[channel] <= TOLERANCE);
		VOE_TEST_CHECK(corner[channel] > 40);
	}
}

// A 2 × 0.5 × 2 Direct floating at y = 1 under a sun shining along (1, -1, 0)
// with a fill: its shadow on the ground runs x 0.7 to 3.2, z -1 to 1. Column
// 26 sees x ≈ 1.9 in it, column 4 x ≈ -2 clear of it, both on row 16.
static void a_direct_casts_a_filled_patch(voe_base_arena *arena,
					voe_render_device *device,
					const voe_3d_shapes *shapes)
{
	voe_ecs_world *world = a_blocked_world(
		arena, shapes, (voe_math_float3){ 1.0f, -1.0f, 0.0f },
		(voe_scene_light){ .colour = { 1.0f, 1.0f, 1.0f },
				   .intensity = 3.0f,
				   .fill_colour = { 1.0f, 1.0f, 1.0f },
				   .fill_intensity = 0.3f },
		VOE_SCENE_LIGHT_BLOCKER_DIRECT, (voe_math_float3){ 2, 0.5f, 2 },
		(voe_math_double3){ 0.0, 1.0, 0.0 });
	voe_render_picture picture = drawn_blocked(world, device, arena);

	if (picture.pixels == NULL)
		return;

	const uint8_t *shadowed = pixel_at(picture, 26, SIDE / 2);
	const uint8_t *sunlit = pixel_at(picture, 4, SIDE / 2);

	for (int channel = 0; channel < 3; channel++) {
		VOE_TEST_CHECK(shadowed[channel] > TOLERANCE);
		VOE_TEST_CHECK(shadowed[channel] + 40 < sunlit[channel]);
	}
}

// A camera at the origin looking down -Z, no light row, and a 2 m blocker 5 m
// ahead: its front face's right edge stands at x = 1, z = -4.
static voe_ecs_world *an_empty_view(voe_base_arena *arena,
				    voe_ecs_entity *blocker)
{
	voe_ecs_world *world = voe_ecs_world_new(
		arena, (voe_ecs_limits){ .entities = 4,
					 .component_types = 16,
					 .intent_types = 16,
					 .structure_requests = 16,
					 .structure_bytes = 1024 });
	voe_ecs_entity camera = { 0 };

	voe_scene_transform_register(world, 4);
	voe_scene_camera_register(world, 1);
	voe_scene_light_register(world, 1);
	voe_scene_light_blocker_register(world, 1);
	voe_3d_mesh_register(world, 1);
	voe_3d_material_register(world, 1);
	voe_3d_panel_register(world, 1);
	voe_3d_model_register(world, 1);
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &camera));
	VOE_TEST_CHECK(voe_scene_transform_add(world, camera, UNMOVED));
	VOE_TEST_CHECK(voe_scene_camera_add(world, camera, LENS));
	*blocker = blocker_of(world, (voe_math_float3){ 2, 2, 2 }, true,
			      placed_at((voe_math_double3){ 0.0, 0.0, -5.0 }));
	return world;
}

// The pixel on the box's front right edge after one frame of `world` with
// `light_blocker` naming `marked`.
static const uint8_t *edge_pixel(voe_ecs_world *world, voe_render_device *device,
				 voe_base_arena *arena,
				 const voe_3d_shapes *shapes,
				 voe_ecs_entity marked)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_3d_frame frame = voe_3d_draw_system_frame(world, size, 0.0f);
	voe_render_pass_camera camera;
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;
	bool drawing = false;

	VOE_TEST_CHECK_INT(frame.light_blocker.entity.generation, 0);
	frame.outlined.material = shapes->outline;
	frame.outlined.colour = (voe_math_float3){ 1.0f, 0.0f, 1.0f };
	frame.light_blocker = (voe_3d_collider_marked){ .entity = marked,
							 .pixels = 4.0f,
							 .size = size };
	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return NULL;
	camera = (voe_render_pass_camera){ .view = frame.view,
					   .light = frame.light };
	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     &camera));
	voe_3d_draw_system_run(world, device, arena, frame);
	voe_render_pass_end(device);
	VOE_TEST_CHECK(voe_render_frame_end(device));
	VOE_TEST_CHECK(voe_render_target_read(device, VOE_RENDER_TARGET_WINDOW,
					      arena, &picture, &error));
	VOE_TEST_CHECK(picture.pixels != NULL);
	if (picture.pixels == NULL)
		return NULL;
	// x = 1 at 4 m is 1 / (4 tan 30°) ≈ 0.43 across the half-width: column 22.
	return pixel_at(picture, 22, SIDE / 2);
}

static bool is_the_outline_colour(const uint8_t *pixel)
{
	return pixel != NULL && pixel[0] > 200 && pixel[1] <= TOLERANCE &&
	       pixel[2] > 200;
}

static void the_selected_blocker_is_outlined(voe_base_arena *arena,
					     voe_render_device *device,
					     const voe_3d_shapes *shapes)
{
	voe_ecs_entity blocker = { 0 };
	voe_ecs_world *world = an_empty_view(arena, &blocker);
	const uint8_t *marked;
	const uint8_t *unmarked;

	marked = edge_pixel(world, device, arena, shapes, blocker);
	VOE_TEST_CHECK(is_the_outline_colour(marked));
	unmarked = edge_pixel(world, device, arena, shapes,
			      (voe_ecs_entity){ 0 });
	VOE_TEST_CHECK(unmarked != NULL && !is_the_outline_colour(unmarked));
}

// The ground or the blocker's lines, one object, and one line set of this
// frame's geometry (3d/draw_system.h).
static const voe_render_capacities CAPACITIES = {
	.vertices = VOE_3D_SHAPES_VERTICES,
	.indices = VOE_3D_SHAPES_INDICES,
	.geometries = VOE_3D_SHAPES_GEOMETRIES,
	.objects = 1,
	.shadings = VOE_3D_SHAPES_SHADINGS,
	.transient_vertices = VOE_3D_COLLIDER_MARKER_VERTICES,
	.transient_indices = VOE_3D_COLLIDER_MARKER_INDICES,
	.transient_geometries = 1,
	.passes = 1,
};

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	voe_render_device *device;
	voe_3d_shapes shapes;

	the_box_follows_the_transform(arena);
	the_table_becomes_the_passs_blockers(arena);
	no_table_fills_none(arena);
	the_blocks_are_bits_of_the_kept(arena);
	the_sun_is_masked_where_it_stands(arena);
	the_box_is_twelve_edges(arena);

	device = voe_render_device_new_headless(arena, size, CAPACITIES, &error);
	if (device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED)
			printf("skip: %s\n", voe_base_error_string(error));
		else
			VOE_TEST_CHECK(device != NULL);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}
	VOE_TEST_CHECK(voe_3d_shapes_upload(device, &shapes, &error));
	a_blocker_keeps_the_sun_out(arena, device, &shapes);
	a_direct_casts_a_filled_patch(arena, device, &shapes);
	the_selected_blocker_is_outlined(arena, device, &shapes);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
