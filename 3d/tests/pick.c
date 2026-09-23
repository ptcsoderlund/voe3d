// The pick ray and the entity it meets: the arithmetic, and the one claim only a
// drawn picture can make.
//
// THE ARITHMETIC HALF NEEDS NO GRAPHICS CARD. A ray is two matrices and a walk
// over triangles the CPU already holds, so the distance to a cube, the ray that
// meets nothing, the frontmost of two in either order, a camera's box beating or
// losing to a cube, a ray through the frustum and not the box picking nothing,
// and the entities that are skipped are all checkable on a build box with no
// Vulkan.
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
#include <3d/panel_component.h>
#include <3d/pick.h>
#include <3d/projection.h>
#include <3d/shape_component.h>
#include <3d/shape_geometry.h>
#include <3d/shape_system.h>
#include <base/arena.h>
#include <base/error.h>
#include <ecs/component.h>
#include <ecs/world.h>
#include <math/float2.h>
#include <math/float4.h>
#include <math/float4x4.h>
#include <render/device.h>
#include <scene/camera_component.h>
#include <scene/camera_system.h>
#include <scene/light_component.h>
#include <scene/light_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <math.h>
#include <stdio.h>

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
		.component_types = 8,
		.intent_types = 8,
		// The shape system queues its removals through it
		// (3d/shape_system.h).
		.structure_requests = 8,
		.structure_bytes = 256,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);

	voe_scene_transform_register(world, 16);
	voe_scene_camera_register(world, 2);
	voe_scene_light_register(world, 2);
	voe_3d_mesh_register(world, 16);
	voe_3d_material_register(world, 16);
	// The draw system walks this table too, and the drawn half below runs it.
	voe_3d_panel_register(world, 4);
	voe_3d_shape_register(world, 8);
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

// Where a world point lands in a picture `size` big, worked out here with the
// same two matrices voe_3d_pick_ray is handed — which is what makes the last
// case below a round trip rather than a repetition.
static voe_math_float2 pixel_of(voe_render_view view, voe_platform_size size,
				voe_math_float3 point)
{
	voe_math_float4x4 clip_from_world =
		voe_math_float4x4_mul(view.projection, view.view);
	voe_math_float4 clip = voe_math_float4x4_mul_float4(
		clip_from_world,
		(voe_math_float4){ point.x, point.y, point.z, 1.0f });

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
		the_view(size), size,
		(voe_math_float2){ WIDTH / 2.0f, HEIGHT / 2.0f });
	float distance = -1.0f;
	voe_ecs_entity hit;

	// The ray starts on the near plane and points into the picture.
	VOE_TEST_CHECK_FLOAT(ray.origin.z, 4.9f, 1e-3f);
	VOE_TEST_CHECK_FLOAT(ray.direction.z, -1.0f, 1e-3f);

	hit = voe_3d_pick(world, geometries, ray, &distance);
	VOE_TEST_CHECK_INT(hit.index, cube.index);
	VOE_TEST_CHECK_INT(hit.generation, cube.generation);
	VOE_TEST_CHECK_FLOAT(distance, 4.5f - 0.1f, 1e-3f);

	// A ray through the top-left pixel points well past the cube's corner:
	// nothing is hit, the answer is a zeroed entity — which is what clears a
	// selection — and the distance is left exactly as it was.
	distance = -1.0f;
	ray = voe_3d_pick_ray(the_view(size), size,
			      (voe_math_float2){ 0.0f, 0.0f });
	hit = voe_3d_pick(world, geometries, ray, &distance);
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
		the_view(size), size,
		(voe_math_float2){ WIDTH / 2.0f, HEIGHT / 2.0f });
	float distance = -1.0f;
	{
		voe_ecs_world *world = a_world(arena);
		voe_ecs_entity far_cube = add_a_cube(world, 0.0f, 0.0f, 0.0f);
		voe_ecs_entity near_cube = add_a_cube(world, 0.0f, 0.0f, 2.0f);
		voe_ecs_entity hit =
			voe_3d_pick(world, geometries, ray, &distance);

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
			voe_3d_pick(world, geometries, ray, &distance);

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

// A camera competes with the shapes on distance (0223): in front of the cube
// its box, 0.15 m deep, is met at 4.9 - 2.15; behind the cube it loses.
static void a_camera_competes_with_the_cube_on_distance(
	voe_base_arena *arena, const voe_3d_shape_geometries *geometries)
{
	voe_platform_size size = { WIDTH, HEIGHT };
	voe_3d_ray ray = voe_3d_pick_ray(
		the_view(size), size,
		(voe_math_float2){ WIDTH / 2.0f, HEIGHT / 2.0f });
	float distance = -1.0f;
	{
		voe_ecs_world *world = a_world(arena);
		voe_ecs_entity cube = add_a_cube(world, 0.0f, 0.0f, 0.0f);
		voe_ecs_entity camera = add_a_camera(world, 0.0f, 0.0f, 2.0f);
		voe_ecs_entity hit =
			voe_3d_pick(world, geometries, ray, &distance);

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
			voe_3d_pick(world, geometries, ray, &distance);

		(void)camera;
		VOE_TEST_CHECK_INT(hit.index, cube.index);
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
	hit = voe_3d_pick(world, geometries, ray, &distance);
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
	voe_3d_ray ray = voe_3d_pick_ray(the_view(size), size, point);
	voe_ecs_entity homeless = { 0 };
	voe_ecs_entity hit;

	VOE_TEST_CHECK(point.x > 0.0f && point.x < (float)WIDTH);
	VOE_TEST_CHECK(point.y > 0.0f && point.y < (float)HEIGHT);

	hit = voe_3d_pick(world, geometries, ray, NULL);
	VOE_TEST_CHECK_INT(hit.index, cube.index);
	VOE_TEST_CHECK_INT(hit.generation, cube.generation);

	// A shape and no transform is nowhere: the draw system draws nothing for
	// it and this answers nothing for it. In a world of its own, so that the
	// answer cannot be somebody else's.
	{
		voe_ecs_world *empty = a_world(arena);
		voe_3d_ray centre = voe_3d_pick_ray(
			the_view(size), size,
			(voe_math_float2){ WIDTH / 2.0f, HEIGHT / 2.0f });

		VOE_TEST_CHECK(voe_ecs_entity_create(empty, &homeless));
		VOE_TEST_CHECK(voe_3d_shape_add(
			empty, homeless,
			(voe_3d_shape){ .kind = VOE_3D_SHAPE_CUBE,
					.colour = VOE_3D_SHAPE_GREY }));
		VOE_TEST_CHECK_INT(
			voe_3d_pick(empty, geometries, centre, NULL).generation,
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
	VOE_TEST_CHECK(voe_scene_light_add(
		world, sun,
		(voe_scene_light){ .direction = { 0.0f, -0.6f, -0.8f },
				   .colour = { 1.0f, 1.0f, 1.0f },
				   .intensity = 3.0f }));
	cube = add_a_cube(world, 1.5f, 1.0f, 0.0f);
	voe_3d_shape_system_run(world, &shapes);

	frame = voe_3d_draw_system_frame(world, size);
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
	// camera in the world, so the eye's camera goes before the picks.
	VOE_TEST_CHECK(voe_ecs_component_remove(
		world, voe_ecs_component_type(world, &voe_scene_camera_key),
		eye));

	VOE_TEST_CHECK(found);
	if (found) {
		voe_ecs_entity hit = voe_3d_pick(
			world, geometries,
			voe_3d_pick_ray(frame.view, size, covered), NULL);

		VOE_TEST_CHECK_INT(hit.index, cube.index);
		VOE_TEST_CHECK_INT(hit.generation, cube.generation);
	}

	// And the corner the background was taken from picks nothing.
	VOE_TEST_CHECK_INT(
		voe_3d_pick(world, geometries,
			    voe_3d_pick_ray(frame.view, size,
					    (voe_math_float2){ 0.0f, 0.0f }),
			    NULL)
			.generation,
		0);

	voe_render_device_destroy(device);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_3d_shape_geometries geometries;

	voe_3d_shape_geometries_create(arena, &geometries);

	the_centre_ray_hits_a_cube_at_the_origin(arena, &geometries);
	the_nearer_of_two_wins_in_either_order(arena, &geometries);
	a_cube_off_centre_is_found_at_its_own_pixel(arena, &geometries);
	a_camera_competes_with_the_cube_on_distance(arena, &geometries);
	a_ray_through_the_frustum_s_corner_picks_nothing(arena, &geometries);
	a_pixel_the_cube_covers_picks_the_cube(arena, &geometries);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
