// A device opens unprepared and voe_render_device_prepare builds the rest a step
// a call (ADR-0345). Three claims, each on a device of its own, so each starts
// unprepared: prepare answers PREPARING at least once and PREPARED within six
// calls, and PREPARED again after; a pass with no camera draws an element on a
// device nothing has prepared, and prepare still has steps left after it; a
// pass with a camera prepares the device itself, draws a mesh, and leaves
// nothing for prepare to do.
//
// The scene, readback and counts are element_scene.h's; the quad and its unlit
// record are tests/elements.c's, copied. Headless: a machine with no usable
// Vulkan skips and says so.
#include "element_scene.h"

// One quad and one object for the mesh case, one element for the element case.
static const voe_render_capacities CAPACITIES = {
	.vertices = 4,
	.indices = 6,
	.geometries = 1,
	.objects = 1,
	.shadings = 1,
	.elements = 1,
	.passes = 1,
};

// Unlit, so the pixel is the base colour whatever the sun.
static const voe_render_shading_values MESH_RED = {
	.base_colour = { 1.0f, 0.0f, 0.0f, 1.0f },
	.roughness = 1.0f,
	.unlit = 1,
	.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
};

// The prepare steps there can be: five pipelines and the relight's startup.
#define MOST_STEPS 6

// A quad over the top-left quadrant of clip space, wound as tests/offscreen.c
// proves a front face, at depth 0.5, above the clear of 0 under GREATER.
static void quad(voe_render_vertex vertices[4], uint32_t indices[6])
{
	static const uint32_t order[6] = { 3, 2, 1, 3, 1, 0 };

	vertices[0] = (voe_render_vertex){ { -1.0f, 1.0f, 0.5f },
					   { 0.0f, 0.0f, 1.0f },
					   { 0.0f, 0.0f } };
	vertices[1] = (voe_render_vertex){ { 0.0f, 1.0f, 0.5f },
					   { 0.0f, 0.0f, 1.0f },
					   { 1.0f, 0.0f } };
	vertices[2] = (voe_render_vertex){ { 0.0f, 0.0f, 0.5f },
					   { 0.0f, 0.0f, 1.0f },
					   { 1.0f, 1.0f } };
	vertices[3] = (voe_render_vertex){ { -1.0f, 0.0f, 0.5f },
					   { 0.0f, 0.0f, 1.0f },
					   { 0.0f, 1.0f } };
	for (int i = 0; i < 6; i++)
		indices[i] = order[i];
}

static void prepare_steps_then_prepared(voe_base_arena *arena)
{
	struct scene scene = { 0 };
	voe_render_prepare answer = VOE_RENDER_PREPARING;
	int preparing = 0;

	if (!open_scene(&scene, arena, CAPACITIES))
		return;
	for (int call = 0; call < MOST_STEPS && answer == VOE_RENDER_PREPARING;
	     call++) {
		answer = voe_render_device_prepare(scene.device);
		if (answer == VOE_RENDER_PREPARING)
			preparing++;
	}
	VOE_TEST_CHECK(preparing >= 1);
	VOE_TEST_CHECK_INT(answer, VOE_RENDER_PREPARED);
	VOE_TEST_CHECK_INT(voe_render_device_prepare(scene.device),
			   VOE_RENDER_PREPARED);
	close_scene(&scene);
}

static void element_pass_needs_no_prepare(voe_base_arena *arena)
{
	struct scene scene = { 0 };
	voe_platform_size size = { SIDE, SIDE };
	const struct voe_render_frame *frame;
	bool drawing = false;

	if (!open_scene(&scene, arena, CAPACITIES))
		return;
	VOE_TEST_CHECK(scene.device->pipeline == VK_NULL_HANDLE);
	frame = voe_render_frame_current(scene.device);
	VOE_TEST_CHECK(voe_render_frame_begin(scene.device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (drawing && scene.pixels != NULL) {
		VOE_TEST_CHECK(voe_render_pass_begin(scene.device,
						     VOE_RENDER_TARGET_WINDOW,
						     NULL));
		VOE_TEST_CHECK(voe_render_frame_submit_element(
			scene.device, solid(0, 0, SIDE, SIDE, RED)));
		VOE_TEST_CHECK(draw_everything(scene.device));
		VOE_TEST_CHECK(close_frame(scene.device));
		read_back(scene.device, frame, scene.readback.buffer);
		VOE_TEST_CHECK_INT(colour_of((const unsigned char *)scene.pixels +
					     (HALF * SIDE + HALF) * 4),
				   IS_RED);
	}
	VOE_TEST_CHECK_INT(voe_render_device_prepare(scene.device),
			   VOE_RENDER_PREPARING);
	close_scene(&scene);
}

static void camera_pass_prepares(voe_base_arena *arena)
{
	struct scene scene = { 0 };
	voe_base_error error = VOE_BASE_OK;
	voe_render_vertex vertices[4];
	uint32_t indices[6];
	const struct voe_render_frame *frame;
	voe_render_object object = {
		.world = voe_math_float4x4_identity(),
		.normal = voe_math_float4x4_identity(),
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
	};

	if (!open_scene(&scene, arena, CAPACITIES))
		return;
	quad(vertices, indices);
	VOE_TEST_CHECK(voe_render_geometry_create(scene.device, vertices, 4,
						  indices, 6, &scene.quad,
						  &error));
	VOE_TEST_CHECK(voe_render_shading_create(scene.device, MESH_RED,
						 &scene.red, &error));
	object.shading = scene.red.index;
	frame = voe_render_frame_current(scene.device);
	if (scene.pixels != NULL && open_frame(scene.device)) {
		VOE_TEST_CHECK(voe_render_frame_draw(scene.device, scene.quad,
						     object));
		VOE_TEST_CHECK(close_frame(scene.device));
		read_back(scene.device, frame, scene.readback.buffer);
		VOE_TEST_CHECK_INT(count_in(scene.pixels, 0, 0, HALF, HALF,
					    IS_RED),
				   QUADRANT);
	}
	VOE_TEST_CHECK_INT(voe_render_device_prepare(scene.device),
			   VOE_RENDER_PREPARED);
	close_scene(&scene);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);

	prepare_steps_then_prepared(arena);
	element_pass_needs_no_prepare(arena);
	camera_pass_prepares(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
