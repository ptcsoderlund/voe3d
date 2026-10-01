// voe_render_frame_copy_depth, the copy of a pass's depth into the sampled copy
// every target keeps (ADR-0305). Four claims.
//
// IT IS REFUSED OUTSIDE A CAMERA PASS. Inside a frame with no pass open, in a
// pass opened with no camera, and in a shadow pass, the call returns false and
// the frame goes on to end normally.
//
// DEPTH DRAWN BEFORE THE COPY STILL OCCLUDES AFTER IT. A red quad over the right
// half at the near depth, the copy, then a green quad over the whole picture at
// the far one: the right half stays red, so the block the copy reopened loaded
// the depth rather than clearing it.
//
// COLOUR DRAWN BEFORE THE COPY IS KEPT. A blue quad over the left half, near,
// before the copy: after the green quad behind it the left half is still blue.
// A reopened block that cleared colour would leave green or the clear there.
//
// THE SAME ON A CALLER'S TARGET. The same three draws into a target of the
// caller's own, read back through voe_render_target_read, as the window is.
//
// Both pictures come back through voe_render_target_read, so the bytes are RGBA.
// Identity camera and unlit records, as tests/passes.c has them; the depths are
// reversed, so the larger z is nearer.
//
// A MACHINE WITH NO USABLE VULKAN SKIPS AND SAYS SO.
#include <render/device.h>

#include <base/arena.h>
#include <base/error.h>
#include <math/float4x4.h>
#include <platform/window.h>

#include <testing/test.h>

#include <stdio.h>

#define SIDE 64
#define HALF (SIDE * SIDE / 2)
#define NEAR_DEPTH 0.75f
#define FAR_DEPTH 0.25f

static const voe_render_capacities CAPACITIES = {
	.vertices = 12,
	.indices = 18,
	.geometries = 3,
	.objects = 4,
	.shadings = 3,
	.elements = 1,
	.passes = 3,
	.targets = 1,
	.shadow_size = 16,
};

static voe_render_shading_values unlit(float red, float green, float blue)
{
	return (voe_render_shading_values){
		.base_colour = { red, green, blue, 1.0f },
		.roughness = 1.0f,
		.unlit = 1,
		.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
	};
}

struct scene {
	voe_render_device *device;
	voe_render_shading red;
	voe_render_shading green;
	voe_render_shading blue;
	voe_render_geometry right_near;
	voe_render_geometry left_near;
	voe_render_geometry whole_far;
	voe_render_target target;
};

// A quad from x0 to x1 across the whole height at depth z, the front face
// tests/passes.c uploads.
static bool upload(struct scene *scene, float x0, float x1, float z,
		   voe_render_geometry *out)
{
	const voe_render_vertex vertices[4] = {
		{ { x0, 1.0f, z }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
		{ { x1, 1.0f, z }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
		{ { x1, -1.0f, z }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
		{ { x0, -1.0f, z }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
	};
	const uint32_t indices[6] = { 3, 2, 1, 3, 1, 0 };
	voe_base_error error = VOE_BASE_OK;

	return voe_render_geometry_create(scene->device, vertices, 4, indices,
					  6, out, &error);
}

static voe_render_pass_camera identity_camera(void)
{
	return (voe_render_pass_camera){
		.view = {
			.view = voe_math_float4x4_identity(),
			.projection = voe_math_float4x4_identity(),
		},
		.light = {
			.direction = { 0.0f, -1.0f, 0.0f },
			.intensity = 1.0f,
			.colour = { 1.0f, 1.0f, 1.0f },
		},
	};
}

static voe_render_object wearing(voe_render_shading shading)
{
	return (voe_render_object){
		.world = voe_math_float4x4_identity(),
		.normal = voe_math_float4x4_identity(),
		.shading = shading.index,
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
	};
}

static bool open_frame(voe_render_device *device)
{
	voe_platform_size size = { SIDE, SIDE };
	bool drawing = false;

	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	return drawing;
}

static void the_copy_is_refused_outside_a_camera_pass(struct scene *scene)
{
	voe_render_device *device = scene->device;
	voe_render_view light = {
		.view = voe_math_float4x4_identity(),
		.projection = voe_math_float4x4_identity(),
	};

	if (!open_frame(device))
		return;
	VOE_TEST_CHECK(!voe_render_frame_copy_depth(device));

	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     NULL));
	VOE_TEST_CHECK(!voe_render_frame_copy_depth(device));
	voe_render_pass_end(device);

	VOE_TEST_CHECK(voe_render_shadow_pass_begin(device, 0, &light));
	VOE_TEST_CHECK(!voe_render_frame_copy_depth(device));
	voe_render_pass_end(device);

	VOE_TEST_CHECK(voe_render_frame_end(device));
}

// Blue left and red right, near, then the copy, then green over all of it, far.
static void draw_around_the_copy(struct scene *scene, voe_render_target target)
{
	voe_render_device *device = scene->device;
	voe_render_pass_camera camera = identity_camera();

	VOE_TEST_CHECK(voe_render_pass_begin(device, target, &camera));
	VOE_TEST_CHECK(voe_render_frame_draw(device, scene->left_near,
					     wearing(scene->blue)));
	VOE_TEST_CHECK(voe_render_frame_draw(device, scene->right_near,
					     wearing(scene->red)));
	VOE_TEST_CHECK(voe_render_frame_copy_depth(device));
	VOE_TEST_CHECK(voe_render_frame_draw(device, scene->whole_far,
					     wearing(scene->green)));
	voe_render_pass_end(device);
}

// How many pixels of the columns x0..x1 are mostly `channel` (0 red, 1 green,
// 2 blue) in an RGBA picture.
static int count_in_columns(const voe_render_picture *picture, int x0, int x1,
			    int channel)
{
	int count = 0;

	for (int y = 0; y < SIDE; y++) {
		for (int x = x0; x < x1; x++) {
			const uint8_t *pixel =
				picture->pixels + (y * SIDE + x) * 4;
			bool most = pixel[channel] > 128;

			for (int other = 0; other < 3; other++)
				if (other != channel &&
				    pixel[other] >= pixel[channel])
					most = false;
			if (most)
				count++;
		}
	}
	return count;
}

static void expect_kept(voe_render_device *device, voe_render_target target,
			voe_base_arena *arena, const char *what)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(arena);
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;

	VOE_TEST_CHECK(voe_render_target_read(device, target, arena, &picture,
					      &error));
	if (picture.pixels == NULL) {
		voe_base_arena_rewind(arena, mark);
		return;
	}
	printf("%s: left blue %d, right red %d, green %d\n", what,
	       count_in_columns(&picture, 0, SIDE / 2, 2),
	       count_in_columns(&picture, SIDE / 2, SIDE, 0),
	       count_in_columns(&picture, 0, SIDE, 1));
	VOE_TEST_CHECK_INT(count_in_columns(&picture, 0, SIDE / 2, 2), HALF);
	VOE_TEST_CHECK_INT(count_in_columns(&picture, SIDE / 2, SIDE, 0), HALF);
	VOE_TEST_CHECK_INT(count_in_columns(&picture, 0, SIDE, 1), 0);
	voe_base_arena_rewind(arena, mark);
}

static void the_window_keeps_what_came_before(struct scene *scene,
					      voe_base_arena *arena)
{
	if (!open_frame(scene->device))
		return;
	draw_around_the_copy(scene, VOE_RENDER_TARGET_WINDOW);
	VOE_TEST_CHECK(voe_render_frame_end(scene->device));
	expect_kept(scene->device, VOE_RENDER_TARGET_WINDOW, arena, "window");
}

static void a_target_keeps_what_came_before(struct scene *scene,
					    voe_base_arena *arena)
{
	if (!open_frame(scene->device))
		return;
	draw_around_the_copy(scene, scene->target);
	VOE_TEST_CHECK(voe_render_frame_end(scene->device));
	expect_kept(scene->device, scene->target, arena, "target");
}

static bool build_scene(struct scene *scene)
{
	voe_render_texture texture;
	voe_base_error error = VOE_BASE_OK;
	bool built = true;

	built &= voe_render_shading_create(scene->device, unlit(1, 0, 0),
					   &scene->red, &error);
	built &= voe_render_shading_create(scene->device, unlit(0, 1, 0),
					   &scene->green, &error);
	built &= voe_render_shading_create(scene->device, unlit(0, 0, 1),
					   &scene->blue, &error);
	built &= upload(scene, 0.0f, 1.0f, NEAR_DEPTH, &scene->right_near);
	built &= upload(scene, -1.0f, 0.0f, NEAR_DEPTH, &scene->left_near);
	built &= upload(scene, -1.0f, 1.0f, FAR_DEPTH, &scene->whole_far);
	built &= voe_render_target_create(scene->device, SIDE, SIDE,
					  &scene->target, &texture, &error);
	return built;
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(256 * 1024);
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	struct scene scene = { 0 };

	scene.device = voe_render_device_new_headless(arena, size, CAPACITIES,
						      &error);
	if (scene.device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED)
			printf("skip: %s\n", voe_base_error_string(error));
		else
			VOE_TEST_CHECK(scene.device != NULL);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}

	if (build_scene(&scene)) {
		the_copy_is_refused_outside_a_camera_pass(&scene);
		the_window_keeps_what_came_before(&scene, arena);
		a_target_keeps_what_came_before(&scene, arena);
	} else {
		VOE_TEST_CHECK(false);
	}

	voe_render_device_destroy(scene.device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
