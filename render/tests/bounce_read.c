// Lit surfaces read the probe volumes (ADR-0326 point 7, 0389 point 6), read
// through ../src/device_internal.h. Headless.
//
// THE SCENE: a grey ground (a 40 m flat box, base 0.5) under a sun at N·L 0.8,
// no cascades, fill 0.1, seen from above; the window's volume 0 begun about the
// origin, lowest cell (−12, −6, −12), corner (−24, −12, −24).
//
// NOTHING CAPTURED IS NO BOUNCE. The first frame's begin wants the volume and
// the next frame's top builds it, cleared: every probe invalid. A frame begun
// onto that built volume names it in its camera pass's block (entry 0 of
// binding 6) and reads the same pixels as a frame with no begin, which names
// none: with no valid probe the read's weights sum to nought and E is nought,
// so the fill alone lifts the ground and the gain is gone.
//
// THE NESTS. A red wall (0.6, 0, 0) 0.5 × 3 × 6 m stands at x = −1 on the
// ground, its +x side sunlit, and the fill is 0 so the bounce shows. Volume 0 is
// settled: captured, relit and faded in, frames until one captures and relights
// nothing. Volume 3, the 1 m nest, is begun about the same place, lowest cell
// (−12, −6, −12), corner (−12, −6, −12).
// - An empty nest reads as the level grid alone: volume 3 built and named
//   (entry 3, grid 12), nothing captured, so its a is 0 and the picture is the
//   volume-0-alone one within 1 per channel.
// - A settled nest reads in its middle: volume 3 settled too, the pixel under
//   the origin, the nest's middle, differs from the volume-0-alone one; its
//   probes stand beside the wall's lit side, where the level grid's are inside
//   the wall or a metre off.
//
// A card without shaderOutputLayer builds no volume and names none; the first
// two pictures still match, the nests are not tried, and that is said. A machine
// with no usable Vulkan skips and says so.
#include "../src/device_internal.h"

#include <base/arena.h>
#include <base/error.h>
#include <math/float4x4.h>
#include <platform/window.h>

#include <testing/test.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIDE 64
#define FIELD_OF_VIEW 1.0471976f
#define NEAR_PLANE 0.1f
#define FAR_PLANE 100.0f
#define FILL 0.1f
#define FRAMES_MAX 600

static const voe_render_capacities CAPACITIES = {
	.vertices = 24,
	.indices = 36,
	.geometries = 1,
	.objects = 16,
	.shadings = 2,
	.passes = 6,
};

static const voe_render_shading_values GREY = {
	.base_colour = { 0.5f, 0.5f, 0.5f, 1.0f },
	.roughness = 1.0f,
	.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
};

static const voe_render_shading_values RED = {
	.base_colour = { 0.6f, 0.0f, 0.0f, 1.0f },
	.roughness = 1.0f,
	.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
};

static const voe_render_light SUN = {
	.direction = { -0.6f, -0.8f, 0.0f },
	.intensity = 1.0f,
	.colour = { 1.0f, 1.0f, 1.0f },
	.fill = { FILL, FILL, FILL },
};

// The level grid's begin, then the 1 m nest's.
static const struct voe_render_bounce_frame BEGINS[2] = {
	{
		.cell = { -12, -6, -12 },
		.corner = { -24.0f, -12.0f, -24.0f },
		.sun = SUN,
		.sun_bounces = 1,
		.sun_strength = 1.0f,
		.spacing = VOE_RENDER_BOUNCE_SPACING,
	},
	{
		.volume = 3,
		.cell = { -12, -6, -12 },
		.corner = { -12.0f, -6.0f, -12.0f },
		.sun = SUN,
		.sun_bounces = 1,
		.sun_strength = 1.0f,
		.spacing = 1.0f,
	},
};

// Counter-clockwise from outside, four vertices a face, a unit cube.
#define H 0.5f
static const voe_render_vertex CUBE_VERTICES[24] = {
	{ { -H, H, H }, { 0, 0, 1 }, { 0, 0 } },
	{ { H, H, H }, { 0, 0, 1 }, { 0, 0 } },
	{ { H, -H, H }, { 0, 0, 1 }, { 0, 0 } },
	{ { -H, -H, H }, { 0, 0, 1 }, { 0, 0 } },
	{ { H, H, -H }, { 0, 0, -1 }, { 0, 0 } },
	{ { -H, H, -H }, { 0, 0, -1 }, { 0, 0 } },
	{ { -H, -H, -H }, { 0, 0, -1 }, { 0, 0 } },
	{ { H, -H, -H }, { 0, 0, -1 }, { 0, 0 } },
	{ { H, H, H }, { 1, 0, 0 }, { 0, 0 } },
	{ { H, H, -H }, { 1, 0, 0 }, { 0, 0 } },
	{ { H, -H, -H }, { 1, 0, 0 }, { 0, 0 } },
	{ { H, -H, H }, { 1, 0, 0 }, { 0, 0 } },
	{ { -H, H, -H }, { -1, 0, 0 }, { 0, 0 } },
	{ { -H, H, H }, { -1, 0, 0 }, { 0, 0 } },
	{ { -H, -H, H }, { -1, 0, 0 }, { 0, 0 } },
	{ { -H, -H, -H }, { -1, 0, 0 }, { 0, 0 } },
	{ { -H, H, -H }, { 0, 1, 0 }, { 0, 0 } },
	{ { H, H, -H }, { 0, 1, 0 }, { 0, 0 } },
	{ { H, H, H }, { 0, 1, 0 }, { 0, 0 } },
	{ { -H, H, H }, { 0, 1, 0 }, { 0, 0 } },
	{ { -H, -H, H }, { 0, -1, 0 }, { 0, 0 } },
	{ { H, -H, H }, { 0, -1, 0 }, { 0, 0 } },
	{ { H, -H, -H }, { 0, -1, 0 }, { 0, 0 } },
	{ { -H, -H, -H }, { 0, -1, 0 }, { 0, 0 } },
};

static const uint32_t CUBE_INDICES[36] = {
	3, 2, 1, 3, 1, 0,	 7, 6, 5, 7, 5, 4,
	11, 10, 9, 11, 9, 8,	 15, 14, 13, 15, 13, 12,
	19, 18, 17, 19, 17, 16,	 23, 22, 21, 23, 21, 20,
};

struct scene {
	voe_render_device *device;
	voe_render_geometry cube;
	voe_render_shading grey;
	voe_render_shading red;
	voe_render_pass_camera camera;
	bool walled;
};

// The camera at (0, 6, 8) looking at the origin, reverse-Z perspective.
static voe_render_view scene_camera(void)
{
	const voe_math_float3 eye = { 0.0f, 6.0f, 8.0f };
	voe_math_float3 z = voe_math_float3_normalize(eye);
	voe_math_float3 up = { 0.0f, 1.0f, 0.0f };
	voe_math_float3 x =
		voe_math_float3_normalize(voe_math_float3_cross(up, z));
	voe_math_float3 rows[3] = { x, voe_math_float3_cross(z, x), z };
	voe_render_view view = { .eye = eye };
	float focal = 1.0f / tanf(FIELD_OF_VIEW * 0.5f);
	float span = FAR_PLANE - NEAR_PLANE;

	for (int r = 0; r < 3; r++) {
		view.view.m[r][0] = rows[r].x;
		view.view.m[r][1] = rows[r].y;
		view.view.m[r][2] = rows[r].z;
		view.view.m[r][3] = -voe_math_float3_dot(rows[r], eye);
	}
	view.view.m[3][3] = 1.0f;
	view.projection.m[0][0] = focal;
	view.projection.m[1][1] = focal;
	view.projection.m[2][2] = NEAR_PLANE / span;
	view.projection.m[2][3] = NEAR_PLANE * FAR_PLANE / span;
	view.projection.m[3][2] = -1.0f;
	return view;
}

// An unturned box of centre `at` and size `size` in `shading`.
static voe_render_object box(voe_math_float3 at, voe_math_float3 size,
			     voe_render_shading shading)
{
	return (voe_render_object){
		.world = voe_math_float4x4_mul(
			voe_math_float4x4_from_translation(at),
			voe_math_float4x4_from_scale(size)),
		.normal = voe_math_float4x4_from_scale((voe_math_float3){
			1.0f / size.x, 1.0f / size.y, 1.0f / size.z }),
		.shading = shading.index,
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
	};
}

// The ground, 40 m square with its top at y = 0, and the wall when `walled`.
static void draw_objects(struct scene *scene)
{
	VOE_TEST_CHECK(voe_render_frame_draw(
		scene->device, scene->cube,
		box((voe_math_float3){ 0.0f, -0.05f, 0.0f },
		    (voe_math_float3){ 40.0f, 0.1f, 40.0f }, scene->grey)));
	if (scene->walled)
		VOE_TEST_CHECK(voe_render_frame_draw(
			scene->device, scene->cube,
			box((voe_math_float3){ -1.0f, 1.5f, 0.0f },
			    (voe_math_float3){ 0.5f, 3.0f, 6.0f }, scene->red)));
}

// One frame onto the window: the first `begun` of BEGINS begun, the first
// `captured` of them each capturing until a capture pass does not open and
// relit, then the camera pass drawing the objects. `named` gets the pass's
// bounce grids, `picture` the picture when not NULL. Whether anything was
// captured or relit.
static bool draw_frame(struct scene *scene, uint32_t begun, uint32_t captured,
		       uint32_t named[VOE_RENDER_BOUNCE_VOLUMES],
		       voe_render_picture *picture, voe_base_arena *arena)
{
	const uint32_t dispatches = scene->device->relight_dispatches;
	voe_base_error error = VOE_BASE_OK;
	const struct voe_render_frame_block *block;
	bool drawing = false;
	bool took = false;
	uint32_t pass;

	VOE_TEST_CHECK(voe_render_frame_begin(scene->device,
					      (voe_platform_size){ SIDE, SIDE },
					      &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return false;
	for (uint32_t i = 0; i < begun; i++) {
		bool opened = i < captured;

		voe_render_bounce_begin(scene->device, VOE_RENDER_TARGET_WINDOW,
					&BEGINS[i]);
		for (uint32_t p = 0; p < VOE_RENDER_BOUNCE_CAPTURE_PASSES && opened;
		     p++) {
			VOE_TEST_CHECK(voe_render_bounce_capture_pass_begin(
				scene->device, &opened));
			if (opened) {
				draw_objects(scene);
				voe_render_pass_end(scene->device);
				took = true;
			}
		}
		if (i < captured)
			voe_render_bounce_relight(scene->device);
	}
	pass = scene->device->pass_count;
	VOE_TEST_CHECK(voe_render_pass_begin(
		scene->device, VOE_RENDER_TARGET_WINDOW, &scene->camera));
	draw_objects(scene);
	voe_render_pass_end(scene->device);
	block = (const struct voe_render_frame_block *)((const unsigned char *)
		voe_render_frame_current(scene->device)->uniforms_mapped +
		pass * scene->device->pass_stride);
	for (uint32_t v = 0; v < VOE_RENDER_BOUNCE_VOLUMES; v++)
		named[v] = block->bounce[v].grid;
	VOE_TEST_CHECK(voe_render_frame_end(scene->device));
	if (picture != NULL)
		VOE_TEST_CHECK(voe_render_target_read(scene->device,
						      VOE_RENDER_TARGET_WINDOW,
						      arena, picture, &error));
	return took || scene->device->relight_dispatches != dispatches;
}

// The first `begun` of BEGINS settled: frames until one captures and relights
// nothing, past the first, which may only want a volume.
static void settle(struct scene *scene, uint32_t begun, voe_base_arena *arena)
{
	uint32_t named[VOE_RENDER_BOUNCE_VOLUMES];
	uint32_t frames = 0;

	draw_frame(scene, begun, begun, named, NULL, arena);
	while (frames < FRAMES_MAX &&
	       draw_frame(scene, begun, begun, named, NULL, arena))
		frames++;
	printf("%u volumes settled in %u frames\n", begun, frames);
	VOE_TEST_CHECK(frames < FRAMES_MAX);
}

// The largest difference of any channel of any pixel between `a` and `b`.
static int largest_gap(const voe_render_picture *a, const voe_render_picture *b)
{
	int largest = 0;

	for (size_t i = 0; i < (size_t)SIDE * SIDE * 4; i++) {
		int gap = abs((int)a->pixels[i] - (int)b->pixels[i]);

		if (gap > largest)
			largest = gap;
	}
	return largest;
}

static void nothing_captured_is_no_bounce(struct scene *scene,
					  voe_base_arena *arena)
{
	const bool layered = scene->device->output_layer;
	const size_t bytes = (size_t)SIDE * SIDE * 4;
	uint32_t named[VOE_RENDER_BOUNCE_VOLUMES];
	voe_render_picture begun = { 0 };
	voe_render_picture plain = { 0 };

	if (!layered)
		printf("note: no shaderOutputLayer, so no volume is built\n");
	draw_frame(scene, 1, 0, named, NULL, arena);
	VOE_TEST_CHECK(named[0] == VOE_RENDER_NO_BOUNCE);
	draw_frame(scene, 1, 0, named, &begun, arena);
	VOE_TEST_CHECK(scene->device->window_volume[0].built == layered);
	VOE_TEST_CHECK(named[0] == (layered ? 0u : VOE_RENDER_NO_BOUNCE));
	draw_frame(scene, 0, 0, named, &plain, arena);
	VOE_TEST_CHECK(named[0] == VOE_RENDER_NO_BOUNCE);

	VOE_TEST_CHECK(begun.pixels != NULL && plain.pixels != NULL);
	if (begun.pixels == NULL || plain.pixels == NULL)
		return;
	{
		const uint8_t *middle =
			&begun.pixels[((SIDE / 2) * SIDE + SIDE / 2) * 4];

		printf("middle: %d %d %d\n", middle[0], middle[1], middle[2]);
		VOE_TEST_CHECK(middle[0] > 0 && middle[0] == middle[1]);
	}
	VOE_TEST_CHECK(memcmp(begun.pixels, plain.pixels, bytes) == 0);
}

static void an_empty_nest_reads_as_the_level_grid_alone(struct scene *scene,
							voe_base_arena *arena)
{
	uint32_t named[VOE_RENDER_BOUNCE_VOLUMES];
	voe_render_picture alone = { 0 };
	voe_render_picture nested = { 0 };

	settle(scene, 1, arena);
	draw_frame(scene, 1, 1, named, &alone, arena);
	draw_frame(scene, 2, 1, named, NULL, arena);
	draw_frame(scene, 2, 1, named, &nested, arena);
	VOE_TEST_CHECK(scene->device->window_volume[3].built);
	VOE_TEST_CHECK_INT(named[0], 0);
	VOE_TEST_CHECK_INT(named[3], 12);
	VOE_TEST_CHECK(alone.pixels != NULL && nested.pixels != NULL);
	if (alone.pixels == NULL || nested.pixels == NULL)
		return;
	printf("empty nest: largest gap %d\n", largest_gap(&alone, &nested));
	VOE_TEST_CHECK(largest_gap(&alone, &nested) <= 1);
}

static void a_settled_nest_reads_in_its_middle(struct scene *scene,
					       voe_base_arena *arena)
{
	const size_t middle = ((SIDE / 2) * SIDE + SIDE / 2) * 4;
	uint32_t named[VOE_RENDER_BOUNCE_VOLUMES];
	voe_render_picture alone = { 0 };
	voe_render_picture nested = { 0 };
	int gap = 0;

	settle(scene, 2, arena);
	draw_frame(scene, 2, 2, named, &nested, arena);
	VOE_TEST_CHECK_INT(named[3], 12);
	draw_frame(scene, 1, 1, named, &alone, arena);
	VOE_TEST_CHECK_INT(named[3], VOE_RENDER_NO_BOUNCE);
	VOE_TEST_CHECK(alone.pixels != NULL && nested.pixels != NULL);
	if (alone.pixels == NULL || nested.pixels == NULL)
		return;
	for (int c = 0; c < 3; c++)
		gap += abs((int)nested.pixels[middle + c] -
			   (int)alone.pixels[middle + c]);
	printf("middle alone %d %d %d, nested %d %d %d\n",
	       alone.pixels[middle], alone.pixels[middle + 1],
	       alone.pixels[middle + 2], nested.pixels[middle],
	       nested.pixels[middle + 1], nested.pixels[middle + 2]);
	VOE_TEST_CHECK(gap > 0);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(1024 * 1024);
	voe_base_error error = VOE_BASE_OK;
	struct scene scene = { 0 };

	scene.device = voe_render_device_new_headless(
		arena, (voe_platform_size){ SIDE, SIDE }, CAPACITIES, &error);
	if (scene.device == NULL && (error == VOE_BASE_ERROR_UNAVAILABLE ||
				     error == VOE_BASE_ERROR_UNSUPPORTED)) {
		printf("skip: %s\n", voe_base_error_string(error));
		voe_base_arena_destroy(arena);
		return 0;
	}
	VOE_TEST_CHECK(scene.device != NULL);
	if (scene.device != NULL) {
		VOE_TEST_CHECK(voe_render_shading_create(scene.device, GREY,
							 &scene.grey, &error));
		VOE_TEST_CHECK(voe_render_shading_create(scene.device, RED,
							 &scene.red, &error));
		VOE_TEST_CHECK(voe_render_geometry_create(
			scene.device, CUBE_VERTICES, 24, CUBE_INDICES, 36,
			&scene.cube, &error));
		scene.camera = (voe_render_pass_camera){
			.view = scene_camera(),
			.light = SUN,
		};
		nothing_captured_is_no_bounce(&scene, arena);
		if (scene.device->output_layer) {
			scene.walled = true;
			scene.camera.light.fill =
				(voe_math_float3){ 0.0f, 0.0f, 0.0f };
			an_empty_nest_reads_as_the_level_grid_alone(&scene, arena);
			a_settled_nest_reads_in_its_middle(&scene, arena);
		}
		voe_render_device_destroy(scene.device);
	}
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
