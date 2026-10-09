// The pick ray and the entity it meets: the arithmetic, and the one claim only a
// drawn picture can make.
//
// THE ARITHMETIC HALF NEEDS NO GRAPHICS CARD. A ray is two matrices and a walk
// over triangles the CPU already holds, so the distance to a cube, the ray that
// meets nothing, the frontmost of two in either order, a camera's box, a sun's
// cube or a bare place's marker winning over a cube in front of it or behind
// it, a water met on its plane and missed beyond it, a ray through the frustum
// and not the box picking nothing, and the entities skipped are all checkable
// on a build box with no Vulkan, and so is a child hit at its world place as
// its parent moves. A model's and a landscape's cases need one to load it.
//
// THE DRAWN HALF CANNOT BE DONE WITHOUT ONE, AND IT IS THE CHECK THAT MATTERS
// MOST. The arithmetic half cannot catch a flipped Y: it works the pixel out the
// same way voe_3d_pick_ray does, so a ray that turns the picture upside down
// agrees with a test that turns it upside down too. Only a frame the card
// actually drew knows which way up it is, so that half draws a cube off the
// centre, finds a pixel the cube covers by reading the target back (ADR-0177)
// and asks who is under it. It skips without a card, exactly as
// 3d/tests/draw_system.c does — a box with no Vulkan is the box and not this
// engine.
#include <3d/draw_system.h>
#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/model_component.h>
#include <3d/models.h>
#include <3d/panel_component.h>
#include <3d/pick.h>
#include <3d/projection.h>
#include <3d/shape_component.h>
#include <3d/shape_geometry.h>
#include <3d/shape_system.h>
#include <3d/water_component.h>
#include <assets/landscape.h>
#include <base/arena.h>
#include <base/error.h>
#include <ecs/component.h>
#include <ecs/structure.h>
#include <ecs/world.h>
#include <math/float2.h>
#include <math/float4.h>
#include <math/float4x4.h>
#include <render/device.h>
#include <scene/camera_component.h>
#include <scene/camera_system.h>
#include <scene/light_component.h>
#include <scene/light_system.h>
#include <scene/parent_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <math.h>
#include <stdio.h>

#include "model_data.inc"

// The three shapes' triangles and edges are a couple of hundred kilobytes
// (3d/shape_geometry.h), and the drawn half reads a picture back into the same
// arena.
#define SCRATCH (4 * 1024 * 1024)

#define WIDTH 640
#define HEIGHT 480

// The drawn half's picture: small enough to scan pixel by pixel, big enough that
// a cube a metre across covers many pixels, and 4:3 like the one above.
#define DRAWN_WIDTH 160
#define DRAWN_HEIGHT 120

static voe_scene_transform at(float x, float y, float z)
{
	voe_scene_transform transform = {
		.position = { x, y, z },
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = { 1.0f, 1.0f, 1.0f },
	};

	return transform;
}

// Five metres back along +Z, looking down -Z, sixty degrees vertically. A cube a
// metre across at the origin (ADR-0191) therefore has its near face at z = 0.5,
// four and a half metres away.
static voe_scene_camera the_lens(void)
{
	return (voe_scene_camera){
		.fov_y = 1.0471976f,
		.near_plane = 0.1f,
		.far_plane = 100.0f,
	};
}

// Where that camera stands, which its view is about (ADR-0250).
static const voe_math_double3 EYE = { 0.0, 0.0, 5.0 };

// That camera's view of a picture `size` big, through voe_3d_view as a pass
// would be opened with it.
static voe_render_view the_view(voe_platform_size size)
{
	voe_render_view view = { 0 };

	VOE_TEST_CHECK(voe_3d_view(at(0.0f, 0.0f, 5.0f), the_lens(),
				   (float)size.width / (float)size.height,
				   &view));
	return view;
}

static voe_ecs_world *a_world(voe_base_arena *arena)
{
	voe_ecs_limits limits = {
		.entities = 16,
		.component_types = 12,
		.intent_types = 8,
		// The shape system queues its removals through it
		// (3d/shape_system.h).
		.structure_requests = 8,
		.structure_bytes = 256,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);

	voe_scene_transform_register(world, 16);
	voe_scene_parent_register(world, 16);
	voe_scene_camera_register(world, 2);
	voe_scene_light_register(world, 2);
	voe_3d_mesh_register(world, 16);
	voe_3d_material_register(world, 16);
	// The draw system walks this table too, and the drawn half below runs it.
	voe_3d_panel_register(world, 4);
	voe_3d_shape_register(world, 8);
	voe_3d_model_register(world, 4);
	voe_3d_water_register(world, 2);
	return world;
}

// A grey cube with a transform at (x, y, z).
static voe_ecs_entity add_a_cube(voe_ecs_world *world, float x, float y,
				 float z)
{
	voe_ecs_entity entity = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(world, entity, at(x, y, z)));
	VOE_TEST_CHECK(voe_3d_shape_add(world, entity,
					(voe_3d_shape){
						.kind = VOE_3D_SHAPE_CUBE,
						.colour = VOE_3D_SHAPE_GREY }));
	return entity;
}

// Where a world point lands in a picture `size` big seen from EYE, worked out
// here with the same two matrices voe_3d_pick_ray is handed — which is what makes the last
// case below a round trip rather than a repetition.
static voe_math_float2 pixel_of(voe_render_view view, voe_platform_size size,
				voe_math_float3 point)
{
	voe_math_float4x4 clip_from_world =
		voe_math_float4x4_mul(view.projection, view.view);
	voe_math_float4 clip = voe_math_float4x4_mul_float4(
		clip_from_world,
		(voe_math_float4){ point.x - (float)EYE.x,
				   point.y - (float)EYE.y,
				   point.z - (float)EYE.z, 1.0f });

	return (voe_math_float2){
		(clip.x / clip.w + 1.0f) * 0.5f * (float)size.width - 0.5f,
		(1.0f - clip.y / clip.w) * 0.5f * (float)size.height - 0.5f,
	};
}

// The centre of the picture, and a cube at the origin: the eye is five metres
// from the cube's centre and four and a half from its near face, and the ray
// starts a near plane's tenth of a metre in front of the eye, so the distance
// along it is 4.4.
static void the_centre_ray_hits_a_cube_at_the_origin(
	voe_base_arena *arena, const voe_3d_shape_geometries *geometries)
{
	voe_ecs_world *world = a_world(arena);
	voe_platform_size size = { WIDTH, HEIGHT };
	voe_ecs_entity cube = add_a_cube(world, 0.0f, 0.0f, 0.0f);
	voe_3d_ray ray = voe_3d_pick_ray(
		the_view(size), EYE, size,
		(voe_math_float2){ WIDTH / 2.0f, HEIGHT / 2.0f });
	float distance = -1.0f;
	voe_ecs_entity hit;

	// The ray starts on the near plane and points into the picture.
	VOE_TEST_CHECK_FLOAT(ray.origin.z, 4.9f, 1e-3f);
	VOE_TEST_CHECK_FLOAT(ray.direction.z, -1.0f, 1e-3f);

	hit = voe_3d_pick(world, geometries, NULL, ray, &distance);
	VOE_TEST_CHECK_INT(hit.index, cube.index);
	VOE_TEST_CHECK_INT(hit.generation, cube.generation);
	VOE_TEST_CHECK_FLOAT(distance, 4.5f - 0.1f, 1e-3f);

	// A ray through the top-left pixel points well past the cube's corner:
	// nothing is hit, the answer is a zeroed entity — which is what clears a
	// selection — and the distance is left exactly as it was.
	distance = -1.0f;
	ray = voe_3d_pick_ray(the_view(size), EYE, size,
			      (voe_math_float2){ 0.0f, 0.0f });
	hit = voe_3d_pick(world, geometries, NULL, ray, &distance);
	VOE_TEST_CHECK_INT(hit.index, 0);
	VOE_TEST_CHECK_INT(hit.generation, 0);
	VOE_TEST_CHECK_FLOAT(distance, -1.0f, 1e-6f);
}

// Two cubes on the same line of sight: the nearer one is the answer, whichever
// order the two rows are in. Table order breaks ties and nothing else.
static void the_nearer_of_two_wins_in_either_order(
	voe_base_arena *arena, const voe_3d_shape_geometries *geometries)
{
	voe_platform_size size = { WIDTH, HEIGHT };
	voe_3d_ray ray = voe_3d_pick_ray(
		the_view(size), EYE, size,
		(voe_math_float2){ WIDTH / 2.0f, HEIGHT / 2.0f });
	float distance = -1.0f;
	{
		voe_ecs_world *world = a_world(arena);
		voe_ecs_entity far_cube = add_a_cube(world, 0.0f, 0.0f, 0.0f);
		voe_ecs_entity near_cube = add_a_cube(world, 0.0f, 0.0f, 2.0f);
		voe_ecs_entity hit =
			voe_3d_pick(world, geometries, NULL, ray, &distance);

		(void)far_cube;
		VOE_TEST_CHECK_INT(hit.index, near_cube.index);
		// 2.5 from the eye to the near cube's front face, less the near
		// plane the ray starts on.
		VOE_TEST_CHECK_FLOAT(distance, 2.5f - 0.1f, 1e-3f);
	}
	{
		// The same two, added the other way round, so the answer cannot
		// be "the first row that hit".
		voe_ecs_world *world = a_world(arena);
		voe_ecs_entity near_cube = add_a_cube(world, 0.0f, 0.0f, 2.0f);
		voe_ecs_entity far_cube = add_a_cube(world, 0.0f, 0.0f, 0.0f);
		voe_ecs_entity hit =
			voe_3d_pick(world, geometries, NULL, ray, &distance);

		(void)far_cube;
		VOE_TEST_CHECK_INT(hit.index, near_cube.index);
		// 2.5 from the eye to the near cube's front face, less the near
		// plane the ray starts on.
		VOE_TEST_CHECK_FLOAT(distance, 2.5f - 0.1f, 1e-3f);
	}
}

// A camera with the test lens and a transform at (x, y, z), looking down -Z.
static voe_ecs_entity add_a_camera(voe_ecs_world *world, float x, float y,
				   float z)
{
	voe_ecs_entity entity = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(world, entity, at(x, y, z)));
	VOE_TEST_CHECK(voe_scene_camera_add(world, entity, the_lens()));
	return entity;
}

// A camera's marker wins over a shape, whichever is nearer (0223, 0354): in
// front of the cube its box, 0.15 m deep, is met at 4.9 - 2.15; behind the cube
// it is still the answer, at its own box's face, 4.9 + 1.85.
static void a_camera_marker_wins_over_the_cube(
	voe_base_arena *arena, const voe_3d_shape_geometries *geometries)
{
	voe_platform_size size = { WIDTH, HEIGHT };
	voe_3d_ray ray = voe_3d_pick_ray(
		the_view(size), EYE, size,
		(voe_math_float2){ WIDTH / 2.0f, HEIGHT / 2.0f });
	float distance = -1.0f;
	{
		voe_ecs_world *world = a_world(arena);
		voe_ecs_entity cube = add_a_cube(world, 0.0f, 0.0f, 0.0f);
		voe_ecs_entity camera = add_a_camera(world, 0.0f, 0.0f, 2.0f);
		voe_ecs_entity hit =
			voe_3d_pick(world, geometries, NULL, ray, &distance);

		(void)cube;
		VOE_TEST_CHECK_INT(hit.index, camera.index);
		VOE_TEST_CHECK_INT(hit.generation, camera.generation);
		VOE_TEST_CHECK_FLOAT(distance, 4.9f - 2.15f, 1e-3f);
	}
	{
		voe_ecs_world *world = a_world(arena);
		voe_ecs_entity camera = add_a_camera(world, 0.0f, 0.0f, -2.0f);
		voe_ecs_entity cube = add_a_cube(world, 0.0f, 0.0f, 0.0f);
		voe_ecs_entity hit =
			voe_3d_pick(world, geometries, NULL, ray, &distance);

		(void)cube;
		VOE_TEST_CHECK_INT(hit.index, camera.index);
		VOE_TEST_CHECK_INT(hit.generation, camera.generation);
		VOE_TEST_CHECK_FLOAT(distance, 4.9f + 1.85f, 1e-3f);
	}
}

// A light with a transform at (x, y, z), shining down -Z.
static voe_ecs_entity add_a_sun(voe_ecs_world *world, float x, float y,
				float z)
{
	voe_ecs_entity entity = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(world, entity, at(x, y, z)));
	VOE_TEST_CHECK(voe_scene_light_add(
		world, entity,
		(voe_scene_light){ .colour = { 1.0f, 1.0f, 1.0f },
				   .intensity = 1.0f }));
	return entity;
}

// A sun's marker wins over a shape, whichever is nearer (0274, 0354): in front
// of the cube its marker's cube, 0.25 m in half extent, is met at 4.9 - 2.25;
// behind the cube it is still the answer, at 4.9 + 1.75.
static void a_sun_marker_wins_over_the_cube(
	voe_base_arena *arena, const voe_3d_shape_geometries *geometries)
{
	voe_platform_size size = { WIDTH, HEIGHT };
	voe_3d_ray ray = voe_3d_pick_ray(
		the_view(size), EYE, size,
		(voe_math_float2){ WIDTH / 2.0f, HEIGHT / 2.0f });
	float distance = -1.0f;
	{
		voe_ecs_world *world = a_world(arena);
		voe_ecs_entity cube = add_a_cube(world, 0.0f, 0.0f, 0.0f);
		voe_ecs_entity sun = add_a_sun(world, 0.0f, 0.0f, 2.0f);
		voe_ecs_entity hit =
			voe_3d_pick(world, geometries, NULL, ray, &distance);

		(void)cube;
		VOE_TEST_CHECK_INT(hit.index, sun.index);
		VOE_TEST_CHECK_INT(hit.generation, sun.generation);
		VOE_TEST_CHECK_FLOAT(distance, 4.9f - 2.25f, 1e-3f);
	}
	{
		voe_ecs_world *world = a_world(arena);
		voe_ecs_entity sun = add_a_sun(world, 0.0f, 0.0f, -2.0f);
		voe_ecs_entity cube = add_a_cube(world, 0.0f, 0.0f, 0.0f);
		voe_ecs_entity hit =
			voe_3d_pick(world, geometries, NULL, ray, &distance);

		(void)cube;
		VOE_TEST_CHECK_INT(hit.index, sun.index);
		VOE_TEST_CHECK_INT(hit.generation, sun.generation);
		VOE_TEST_CHECK_FLOAT(distance, 4.9f + 1.75f, 1e-3f);
	}
}

// A water 4 m wide and 6 m long at the origin, seen straight down from 3 m up,
// is met on its plane at 3 m; 2.5 m aside along X, past its half width, the same
// ray meets nothing. Either face counts, so from 3 m below it is met too.
static void a_water_is_hit_on_its_plane_and_missed_beyond_it(
	voe_base_arena *arena, const voe_3d_shape_geometries *geometries)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity water = { 0 };
	voe_3d_water body = { .width = 4.0f, .length = 6.0f };
	voe_3d_ray ray = { .origin = { 0.0, 3.0, 0.0 },
			   .direction = { 0.0f, -1.0f, 0.0f } };
	float distance = -1.0f;
	voe_ecs_entity hit;

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &water));
	VOE_TEST_CHECK(voe_scene_transform_add(world, water,
					       at(0.0f, 0.0f, 0.0f)));
	VOE_TEST_CHECK(voe_3d_water_add(world, water, body));

	hit = voe_3d_pick(world, geometries, NULL, ray, &distance);
	VOE_TEST_CHECK_INT(hit.index, water.index);
	VOE_TEST_CHECK_INT(hit.generation, water.generation);
	VOE_TEST_CHECK_FLOAT(distance, 3.0f, 1e-4f);

	ray.origin.y = -3.0;
	ray.direction.y = 1.0f;
	hit = voe_3d_pick(world, geometries, NULL, ray, &distance);
	VOE_TEST_CHECK_INT(hit.index, water.index);
	VOE_TEST_CHECK_FLOAT(distance, 3.0f, 1e-4f);

	ray.origin.x = 2.5;
	distance = -1.0f;
	hit = voe_3d_pick(world, geometries, NULL, ray, &distance);
	VOE_TEST_CHECK_INT(hit.generation, 0);
	VOE_TEST_CHECK_FLOAT(distance, -1.0f, 1e-6f);
}

// A bare transform behind a cube is the answer on its place marker's cube, at
// 4.9 + 1.75 (0354, 0365); one a metre beside the ray is not, and the cube is.
static void a_place_marker_wins_over_the_cube(
	voe_base_arena *arena, const voe_3d_shape_geometries *geometries)
{
	voe_platform_size size = { WIDTH, HEIGHT };
	voe_3d_ray ray = voe_3d_pick_ray(
		the_view(size), EYE, size,
		(voe_math_float2){ WIDTH / 2.0f, HEIGHT / 2.0f });
	float distance = -1.0f;
	{
		voe_ecs_world *world = a_world(arena);
		voe_ecs_entity place = { 0 };
		voe_ecs_entity cube = add_a_cube(world, 0.0f, 0.0f, 0.0f);
		voe_ecs_entity hit;

		VOE_TEST_CHECK(voe_ecs_entity_create(world, &place));
		VOE_TEST_CHECK(voe_scene_transform_add(world, place,
						       at(0.0f, 0.0f, -2.0f)));
		hit = voe_3d_pick(world, geometries, NULL, ray, &distance);
		(void)cube;
		VOE_TEST_CHECK_INT(hit.index, place.index);
		VOE_TEST_CHECK_INT(hit.generation, place.generation);
		VOE_TEST_CHECK_FLOAT(distance, 4.9f + 1.75f, 1e-3f);
	}
	{
		voe_ecs_world *world = a_world(arena);
		voe_ecs_entity place = { 0 };
		voe_ecs_entity cube = add_a_cube(world, 0.0f, 0.0f, 0.0f);
		voe_ecs_entity hit;

		VOE_TEST_CHECK(voe_ecs_entity_create(world, &place));
		VOE_TEST_CHECK(voe_scene_transform_add(world, place,
						       at(1.0f, 0.0f, -2.0f)));
		hit = voe_3d_pick(world, geometries, NULL, ray, &distance);
		VOE_TEST_CHECK_INT(hit.index, cube.index);
		VOE_TEST_CHECK_INT(hit.generation, cube.generation);
		VOE_TEST_CHECK_FLOAT(distance, 4.5f - 0.1f, 1e-3f);
	}
}

// Only the box is hit (0223): a ray straight down -Z through the frustum's far
// top-right corner, 1 m ahead at 60 degrees and 16:9, passes the box by and
// picks nothing.
static void a_ray_through_the_frustum_s_corner_picks_nothing(
	voe_base_arena *arena, const voe_3d_shape_geometries *geometries)
{
	voe_ecs_world *world = a_world(arena);
	float up = tanf(the_lens().fov_y * 0.5f);
	voe_3d_ray ray = {
		.origin = { up * 16.0f / 9.0f, up, 5.0f },
		.direction = { 0.0f, 0.0f, -1.0f },
	};
	float distance = -1.0f;
	voe_ecs_entity hit;

	(void)add_a_camera(world, 0.0f, 0.0f, 0.0f);
	hit = voe_3d_pick(world, geometries, NULL, ray, &distance);
	VOE_TEST_CHECK_INT(hit.generation, 0);
	VOE_TEST_CHECK_FLOAT(distance, -1.0f, 1e-6f);
}

// A cube away from the centre is answered by a ray through the pixel its own
// centre projects to — and a shape with no transform is nowhere and is never
// answered, even by a ray that passes through where it would be.
static void a_cube_off_centre_is_found_at_its_own_pixel(
	voe_base_arena *arena, const voe_3d_shape_geometries *geometries)
{
	voe_ecs_world *world = a_world(arena);
	voe_platform_size size = { WIDTH, HEIGHT };
	voe_ecs_entity cube = add_a_cube(world, 2.0f, 1.0f, 0.0f);
	voe_math_float2 point = pixel_of(the_view(size), size,
					 (voe_math_float3){ 2.0f, 1.0f, 0.0f });
	voe_3d_ray ray = voe_3d_pick_ray(the_view(size), EYE, size, point);
	voe_ecs_entity homeless = { 0 };
	voe_ecs_entity hit;

	VOE_TEST_CHECK(point.x > 0.0f && point.x < (float)WIDTH);
	VOE_TEST_CHECK(point.y > 0.0f && point.y < (float)HEIGHT);

	hit = voe_3d_pick(world, geometries, NULL, ray, NULL);
	VOE_TEST_CHECK_INT(hit.index, cube.index);
	VOE_TEST_CHECK_INT(hit.generation, cube.generation);

	// A shape and no transform is nowhere: the draw system draws nothing for
	// it and this answers nothing for it. In a world of its own, so that the
	// answer cannot be somebody else's.
	{
		voe_ecs_world *empty = a_world(arena);
		voe_3d_ray centre = voe_3d_pick_ray(
			the_view(size), EYE, size,
			(voe_math_float2){ WIDTH / 2.0f, HEIGHT / 2.0f });

		VOE_TEST_CHECK(voe_ecs_entity_create(empty, &homeless));
		VOE_TEST_CHECK(voe_3d_shape_add(
			empty, homeless,
			(voe_3d_shape){ .kind = VOE_3D_SHAPE_CUBE,
					.colour = VOE_3D_SHAPE_GREY }));
		VOE_TEST_CHECK_INT(
			voe_3d_pick(empty, geometries, NULL, centre, NULL).generation,
			0);
	}
}

// THE RAY AND THE PICTURE AGREE ABOUT WHICH WAY IS UP. A cube off to one side
// and above the centre is drawn into a headless device's own picture; a pixel
// that differs from the background is a pixel the cube covers, and picking
// through it has to answer that cube. A flipped Y passes every case above and
// fails this one.
static void a_pixel_the_cube_covers_picks_the_cube(
	voe_base_arena *arena, const voe_3d_shape_geometries *geometries)
{
	voe_platform_size size = { DRAWN_WIDTH, DRAWN_HEIGHT };
	voe_base_error error = VOE_BASE_OK;
	voe_render_capacities capacities = {
		.vertices = VOE_3D_SHAPES_VERTICES,
		.indices = VOE_3D_SHAPES_INDICES,
		.geometries = VOE_3D_SHAPES_GEOMETRIES,
		.objects = 1,
		.shadings = VOE_3D_SHAPES_SHADINGS,
		.passes = 1,
	};
	voe_render_device *device =
		voe_render_device_new_headless(arena, size, capacities, &error);
	voe_3d_shapes shapes;
	voe_ecs_world *world;
	voe_ecs_entity eye = { 0 };
	voe_ecs_entity sun = { 0 };
	voe_ecs_entity cube;
	voe_3d_frame frame;
	voe_render_pass_camera pass_camera;
	voe_render_picture picture = { 0 };
	bool drawing = false;
	bool found = false;
	voe_math_float2 covered = { 0.0f, 0.0f };

	if (device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED) {
			printf("skip: %s\n", voe_base_error_string(error));
			return;
		}
		VOE_TEST_CHECK(device != NULL);
		return;
	}
	VOE_TEST_CHECK(voe_3d_shapes_upload(device, &shapes, &error));

	world = a_world(arena);
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &eye));
	VOE_TEST_CHECK(
		voe_scene_transform_add(world, eye, at(0.0f, 0.0f, 5.0f)));
	VOE_TEST_CHECK(voe_scene_camera_add(world, eye, the_lens()));
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &sun));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, sun,
		(voe_scene_transform){
			.rotation = voe_scene_light_facing(
				(voe_math_float3){ 0.0f, -0.6f, -0.8f }),
			.scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_light_add(
		world, sun,
		(voe_scene_light){ .colour = { 1.0f, 1.0f, 1.0f },
				   .intensity = 3.0f }));
	cube = add_a_cube(world, 1.5f, 1.0f, 0.0f);
	voe_3d_shape_system_run(world, &shapes);

	frame = voe_3d_draw_system_frame(world, size, 0.0f);
	pass_camera = (voe_render_pass_camera){ .view = frame.view,
						.light = frame.light };
	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (drawing) {
		VOE_TEST_CHECK(voe_render_pass_begin(
			device, VOE_RENDER_TARGET_WINDOW, &pass_camera));
		voe_3d_draw_system_run(world, device, arena, frame);
		voe_render_pass_end(device);
		VOE_TEST_CHECK(voe_render_frame_end(device));
	}

	VOE_TEST_CHECK(voe_render_target_read(device, VOE_RENDER_TARGET_WINDOW,
					      arena, &picture, &error));
	if (picture.pixels != NULL) {
		// The top-left pixel is the background — the cube is up and to
		// the right of the centre and covers nothing near that corner —
		// so any pixel unlike it is a pixel the cube was drawn into.
		const uint8_t *background = picture.pixels;

		for (uint32_t y = 0; y < picture.height && !found; y++) {
			for (uint32_t x = 0; x < picture.width; x++) {
				const uint8_t *pixel =
					picture.pixels +
					((size_t)y * picture.width + x) * 4;

				if (pixel[0] == background[0] &&
				    pixel[1] == background[1] &&
				    pixel[2] == background[2])
					continue;
				covered = (voe_math_float2){ (float)x,
							     (float)y };
				found = true;
				break;
			}
		}
	}

	// The eye's own marker box holds every pick ray's origin, so it would
	// answer every pixel (0223); an editor picks from its orbit, not from a
	// camera in the world, so the eye goes before the picks — all of it, or
	// its bare transform's place marker would answer instead (0365).
	voe_ecs_entity_destroy(world, eye);

	VOE_TEST_CHECK(found);
	if (found) {
		voe_ecs_entity hit = voe_3d_pick(
			world, geometries, NULL,
			voe_3d_pick_ray(frame.view, frame.eye, size, covered), NULL);

		VOE_TEST_CHECK_INT(hit.index, cube.index);
		VOE_TEST_CHECK_INT(hit.generation, cube.generation);
	}

	// And the corner the background was taken from picks nothing.
	VOE_TEST_CHECK_INT(
		voe_3d_pick(world, geometries, NULL,
			    voe_3d_pick_ray(frame.view, frame.eye, size,
					    (voe_math_float2){ 0.0f, 0.0f }),
			    NULL)
			.generation,
		0);

	voe_render_device_destroy(device);
}

// A THING WEARING A MODEL IS HIT ON THE MODEL'S OWN TRIANGLES. The file of
// model_data.inc is loaded (which uploads, so this skips with no card) and worn
// by a thing at (3,0,0), unturned. A ray five metres out along +X from the
// middle of the entry's first triangle, pointing back along -X, meets it at
// five metres, since that triangle lies in the model's x = 1 plane
// (3d/tests/models.c); the same ray ten metres higher meets nothing, and with
// no store the first ray meets nothing either.
static void a_model_is_hit_at_its_distance_and_missed_beside_it(
	voe_base_arena *arena, const voe_3d_shape_geometries *geometries)
{
	voe_platform_size size = { 4, 4 };
	voe_base_error error = VOE_BASE_OK;
	voe_render_capacities capacities = {
		.vertices = 64,
		.indices = 64,
		.geometries = 4,
		.objects = 1,
		.shadings = 4,
		.passes = 1,
	};
	voe_render_device *device =
		voe_render_device_new_headless(arena, size, capacities, &error);
	voe_ecs_world *world;
	voe_ecs_entity thing = { 0 };
	voe_3d_model model = { 0 };
	voe_3d_models *models;
	const voe_3d_model_entry *entry;
	const voe_3d_shape_geometry *shape;
	voe_math_float3 middle = { 0.0f, 0.0f, 0.0f };
	voe_3d_ray ray;
	voe_ecs_entity hit;
	float distance = -1.0f;

	if (device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED) {
			printf("skip: %s\n", voe_base_error_string(error));
			return;
		}
		VOE_TEST_CHECK(device != NULL);
		return;
	}
	models = voe_3d_models_new();
	VOE_TEST_CHECK(voe_3d_models_load(models, device, "Assets/two.glb", 1,
					  TWO_PRIMITIVES_GLB,
					  sizeof(TWO_PRIMITIVES_GLB), &error));
	entry = voe_3d_models_find(models, "Assets/two.glb");
	VOE_TEST_CHECK(entry != NULL && entry->loaded);
	if (entry == NULL || !entry->loaded || entry->shape.index_count < 3)
		goto destroy;
	shape = &entry->shape;
	for (uint32_t i = 0; i < 3; i++)
		middle = voe_math_float3_add(
			middle, voe_math_float3_scale(
					shape->vertices[shape->indices[i]].position,
					1.0f / 3.0f));
	VOE_TEST_CHECK_FLOAT(middle.x, 1.0f, 1e-5f);

	world = a_world(arena);
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &thing));
	VOE_TEST_CHECK(
		voe_scene_transform_add(world, thing, at(3.0f, 0.0f, 0.0f)));
	snprintf(model.path, sizeof(model.path), "%s", "Assets/two.glb");
	VOE_TEST_CHECK(voe_3d_model_add(world, thing, model));

	ray = (voe_3d_ray){ .origin = { 3.0 + middle.x + 5.0, middle.y,
					middle.z },
			    .direction = { -1.0f, 0.0f, 0.0f } };
	hit = voe_3d_pick(world, geometries, models, ray, &distance);
	VOE_TEST_CHECK_INT(hit.index, thing.index);
	VOE_TEST_CHECK_INT(hit.generation, thing.generation);
	VOE_TEST_CHECK_FLOAT(distance, 5.0f, 1e-4f);

	VOE_TEST_CHECK_INT(
		voe_3d_pick(world, geometries, NULL, ray, NULL).generation, 0);

	ray.origin.y += 10.0;
	distance = -1.0f;
	hit = voe_3d_pick(world, geometries, models, ray, &distance);
	VOE_TEST_CHECK_INT(hit.generation, 0);
	VOE_TEST_CHECK_FLOAT(distance, -1.0f, 1e-6f);

destroy:
	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
	voe_render_device_destroy(device);
}

// Room for one landscape: the shared grid, its heights texture, its ground and
// the ground's twin.
static const voe_render_capacities LANDSCAPE_CAPACITIES = {
	.vertices = 33 * 33,
	.indices = 32 * 32 * 6,
	.geometries = 1,
	.heights_texels = VOE_3D_LANDSCAPE_WRITE_TEXELS,
	.objects = 1,
	.shadings = 2,
	.passes = 1,
};

#define HILL "Assets/hill.landscape"

// A headless device, or NULL after saying why it skips.
static voe_render_device *a_landscape_device(voe_base_arena *arena)
{
	voe_base_error error = VOE_BASE_OK;
	voe_render_device *device = voe_render_device_new_headless(
		arena, (voe_platform_size){ 4, 4 }, LANDSCAPE_CAPACITIES, &error);

	if (device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED)
			printf("skip: %s\n", voe_base_error_string(error));
		else
			VOE_TEST_CHECK(device != NULL);
	}
	return device;
}

// A store holding HILL: 16 m of 8 cells, every height 1 m.
static voe_3d_models *a_hill(voe_base_arena *arena, voe_render_device *device)
{
	voe_assets_landscape land = voe_assets_landscape_flat(16.0f, 8, arena);
	voe_3d_models *models = voe_3d_models_new();
	voe_base_error error = VOE_BASE_OK;

	for (uint32_t i = 0; i < 9 * 9; i++)
		land.heights[i] = 1.0f;
	VOE_TEST_CHECK(voe_3d_models_load_landscape(models, device, HILL, 1,
						    &land, &error));
	return models;
}

// A thing wearing HILL placed by `transform`.
static voe_ecs_entity add_a_hill(voe_ecs_world *world,
				 voe_scene_transform transform)
{
	voe_ecs_entity entity = { 0 };
	voe_3d_model model = { 0 };

	snprintf(model.path, sizeof(model.path), "%s", HILL);
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(world, entity, transform));
	VOE_TEST_CHECK(voe_3d_model_add(world, entity, model));
	return entity;
}

// A LANDSCAPE IS MET BY ITS HEIGHTS, its shape being empty (0379 point 2). The
// hill at (10, 0, 0), its ground 1 m up, is met 10 m down a ray from 11 m; the
// same ray 20 m aside, past the grid's 8 m half side, meets nothing, and with
// no store the first meets nothing either.
static void pick_meets_a_landscape(voe_base_arena *arena,
				   const voe_3d_shape_geometries *geometries)
{
	voe_render_device *device = a_landscape_device(arena);
	voe_3d_models *models;
	voe_ecs_world *world;
	voe_ecs_entity thing;
	voe_3d_ray ray = { .origin = { 10.0, 11.0, 0.0 },
			   .direction = { 0.0f, -1.0f, 0.0f } };
	float distance = -1.0f;
	voe_ecs_entity hit;

	if (device == NULL)
		return;
	models = a_hill(arena, device);
	world = a_world(arena);
	thing = add_a_hill(world, at(10.0f, 0.0f, 0.0f));

	hit = voe_3d_pick(world, geometries, models, ray, &distance);
	VOE_TEST_CHECK_INT(hit.index, thing.index);
	VOE_TEST_CHECK_INT(hit.generation, thing.generation);
	VOE_TEST_CHECK_FLOAT(distance, 10.0f, 1e-3f);

	VOE_TEST_CHECK_INT(
		voe_3d_pick(world, geometries, NULL, ray, NULL).generation, 0);

	ray.origin.x += 20.0;
	distance = -1.0f;
	hit = voe_3d_pick(world, geometries, models, ray, &distance);
	VOE_TEST_CHECK_INT(hit.generation, 0);
	VOE_TEST_CHECK_FLOAT(distance, -1.0f, 1e-6f);

	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
	voe_render_device_destroy(device);
}

// THE HIT IS IN THE GRID'S OWN SPACE (0379 point 1). The hill scaled 2 at
// (100, 0, -50) is 2 m up; a ray down from world (104, 11, -44) meets it 9 m
// along at own (2, 3). A cube is no landscape, and a miss leaves `hit` alone.
static void pick_landscape_answers_its_own_space(voe_base_arena *arena)
{
	voe_render_device *device = a_landscape_device(arena);
	voe_scene_transform placed = at(100.0f, 0.0f, -50.0f);
	voe_3d_landscape_hit hit = { -1.0f, -1.0f, -1.0f };
	voe_3d_models *models;
	voe_ecs_world *world;
	voe_ecs_entity thing;
	voe_ecs_entity cube;
	voe_3d_ray ray = { .origin = { 104.0, 11.0, -44.0 },
			   .direction = { 0.0f, -1.0f, 0.0f } };

	if (device == NULL)
		return;
	models = a_hill(arena, device);
	world = a_world(arena);
	placed.scale = (voe_math_float3){ 2.0f, 2.0f, 2.0f };
	thing = add_a_hill(world, placed);
	cube = add_a_cube(world, 104.0f, 0.0f, -44.0f);

	VOE_TEST_CHECK(voe_3d_pick_landscape(world, models, thing, ray, &hit));
	VOE_TEST_CHECK_FLOAT(hit.x, 2.0f, 1e-3f);
	VOE_TEST_CHECK_FLOAT(hit.z, 3.0f, 1e-3f);
	VOE_TEST_CHECK_FLOAT(hit.distance, 9.0f, 1e-3f);

	hit = (voe_3d_landscape_hit){ -1.0f, -1.0f, -1.0f };
	VOE_TEST_CHECK(!voe_3d_pick_landscape(world, models, cube, ray, &hit));
	VOE_TEST_CHECK(!voe_3d_pick_landscape(world, NULL, thing, ray, &hit));
	ray.origin.x += 40.0;
	VOE_TEST_CHECK(!voe_3d_pick_landscape(world, models, thing, ray, &hit));
	VOE_TEST_CHECK_FLOAT(hit.distance, -1.0f, 0.0f);

	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
	voe_render_device_destroy(device);
}

// A cube 5 m along +X under a parent at (0, 0, -10) that has no shape: the ray
// finds it at its world place, and follows it when the parent moves 3 m up.
static void a_child_is_hit_at_its_world_place(
	voe_base_arena *arena, const voe_3d_shape_geometries *geometries)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity parent = { 0 };
	voe_ecs_entity child = add_a_cube(world, 5.0f, 0.0f, -10.0f);
	voe_3d_ray ray = { .origin = { 5.0, 0.0, 0.0 },
			   .direction = { 0.0f, 0.0f, -1.0f } };
	float distance = -1.0f;
	voe_ecs_entity hit;

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &parent));
	VOE_TEST_CHECK(voe_scene_transform_add(world, parent,
					       at(0.0f, 0.0f, -10.0f)));
	VOE_TEST_CHECK(voe_scene_parent_set(world, child, parent));
	voe_ecs_structure_apply(world);
	voe_scene_transform_system_run(world);
	VOE_TEST_CHECK_FLOAT(voe_scene_transform_get(world, child)->position.z,
			     0.0f, 1e-5f);

	hit = voe_3d_pick(world, geometries, NULL, ray, &distance);
	VOE_TEST_CHECK_INT(hit.index, child.index);
	VOE_TEST_CHECK_INT(hit.generation, child.generation);
	VOE_TEST_CHECK_FLOAT(distance, 9.5f, 1e-4f);

	VOE_TEST_CHECK(voe_scene_transform_submit(
		world, (voe_scene_transform_intent){
			       .entity = parent,
			       .transform = at(0.0f, 3.0f, -10.0f) }));
	voe_scene_transform_system_run(world);
	VOE_TEST_CHECK_INT(voe_3d_pick(world, geometries, NULL, ray, NULL)
				   .generation,
			   0);

	ray.origin.y = 3.0;
	hit = voe_3d_pick(world, geometries, NULL, ray, NULL);
	VOE_TEST_CHECK_INT(hit.index, child.index);
	VOE_TEST_CHECK_INT(hit.generation, child.generation);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_3d_shape_geometries geometries;

	voe_3d_shape_geometries_create(arena, &geometries);

	the_centre_ray_hits_a_cube_at_the_origin(arena, &geometries);
	the_nearer_of_two_wins_in_either_order(arena, &geometries);
	a_cube_off_centre_is_found_at_its_own_pixel(arena, &geometries);
	a_camera_marker_wins_over_the_cube(arena, &geometries);
	a_sun_marker_wins_over_the_cube(arena, &geometries);
	a_water_is_hit_on_its_plane_and_missed_beyond_it(arena, &geometries);
	a_place_marker_wins_over_the_cube(arena, &geometries);
	a_ray_through_the_frustum_s_corner_picks_nothing(arena, &geometries);
	a_pixel_the_cube_covers_picks_the_cube(arena, &geometries);
	a_model_is_hit_at_its_distance_and_missed_beside_it(arena, &geometries);
	a_child_is_hit_at_its_world_place(arena, &geometries);
	pick_meets_a_landscape(arena, &geometries);
	pick_landscape_answers_its_own_space(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
