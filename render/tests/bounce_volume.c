// A target's probe volume, built on first use and freed when unused (ADR-0326
// point 2, ADR-0316), read through ../src/device_internal.h. Headless.
//
// THE FIRST BEGIN WANTS, THE NEXT FRAME BUILDS. A begin onto the window leaves its
// volume wanted and not built; the next frame's top builds it, and a begin then
// places its probes. A begin onto a target that frame leaves the target's wanted
// and not built while the window's is built: the two are apart, and once both
// are built their images differ.
//
// UNUSED, FREED. After VOE_RENDER_BOUNCE_IDLE (300) frames with no begin both are
// still built; the top of the frame after frees them, and wants nothing more.
//
// A card without shaderOutputLayer bounces nothing and builds nothing: that is
// checked instead, and said. A machine with no usable Vulkan skips and says so.
#include "../src/device_internal.h"

#include <base/arena.h>
#include <base/error.h>
#include <platform/window.h>

#include <testing/test.h>

#include <stdio.h>

#define SIDE 16

static const voe_render_capacities CAPACITIES = {
	.vertices = 8,
	.indices = 36,
	.geometries = 1,
	.objects = 4,
	.shadings = 1,
	.passes = 4,
	.targets = 1,
};

static const struct voe_render_bounce_frame BOUNCE = {
	.cell = { -12, -6, -12 },
	.corner = { -24.0f, -12.0f, -24.0f },
	.sun = { .direction = { 0.0f, -1.0f, 0.0f }, .intensity = 1.0f,
		 .colour = { 1.0f, 1.0f, 1.0f } },
	.sun_bounces = 1,
	.sun_strength = 1.0f,
};

// One frame: begun onto the window and `target` as asked, then ended.
static void one_frame(voe_render_device *device, bool window, bool target,
		      voe_render_target id)
{
	bool drawing = false;

	VOE_TEST_CHECK(voe_render_frame_begin(device, (voe_platform_size){ SIDE, SIDE },
					      &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return;
	if (window)
		voe_render_bounce_begin(device, VOE_RENDER_TARGET_WINDOW, &BOUNCE);
	if (target)
		voe_render_bounce_begin(device, id, &BOUNCE);
	VOE_TEST_CHECK(voe_render_frame_end(device));
}

static void built_on_first_use(voe_render_device *device, voe_render_target id)
{
	const struct voe_render_bounce_volume *window = &device->window_volume;
	const struct voe_render_bounce_volume *own = &device->targets[0].volume;

	one_frame(device, true, false, id);
	VOE_TEST_CHECK(window->wanted && !window->built);
	VOE_TEST_CHECK(!own->wanted && !own->built);

	one_frame(device, true, true, id);
	VOE_TEST_CHECK(window->built && window->albedo.image != VK_NULL_HANDLE);
	VOE_TEST_CHECK(window->sh[6][2].image != VK_NULL_HANDLE);
	VOE_TEST_CHECK(window->probes.placed);
	VOE_TEST_CHECK(own->wanted && !own->built && !own->probes.placed);

	// Both now last begun a frame ago; this frame top builds the target's.
	for (int i = 0; i < VOE_RENDER_BOUNCE_IDLE; i++)
		one_frame(device, false, false, id);
	VOE_TEST_CHECK(own->built && own->albedo.image != VK_NULL_HANDLE);
	VOE_TEST_CHECK(own->albedo.image != window->albedo.image);
	VOE_TEST_CHECK(window->built);

	one_frame(device, false, false, id);
	VOE_TEST_CHECK(!window->built && !window->wanted);
	VOE_TEST_CHECK(window->albedo.image == VK_NULL_HANDLE);
	VOE_TEST_CHECK(!own->built && !own->wanted);
}

static void nothing_without_output_layer(voe_render_device *device,
					 voe_render_target id)
{
	printf("note: no shaderOutputLayer, so nothing bounces\n");
	one_frame(device, true, true, id);
	one_frame(device, false, false, id);
	VOE_TEST_CHECK(!device->window_volume.wanted &&
		       !device->window_volume.built);
	VOE_TEST_CHECK(!device->targets[0].volume.built);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(256 * 1024);
	voe_base_error error = VOE_BASE_OK;
	voe_render_target id;
	voe_render_texture texture;
	voe_render_device *device = voe_render_device_new_headless(
		arena, (voe_platform_size){ SIDE, SIDE }, CAPACITIES, &error);

	if (device == NULL && (error == VOE_BASE_ERROR_UNAVAILABLE ||
			       error == VOE_BASE_ERROR_UNSUPPORTED)) {
		printf("skip: %s\n", voe_base_error_string(error));
		voe_base_arena_destroy(arena);
		return 0;
	}
	VOE_TEST_CHECK(device != NULL);
	if (device != NULL) {
		VOE_TEST_CHECK(voe_render_target_create(device, SIDE, SIDE, &id,
							&texture, &error));
		if (device->output_layer)
			built_on_first_use(device, id);
		else
			nothing_without_output_layer(device, id);
		voe_render_device_destroy(device);
	}
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
