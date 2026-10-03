// A target's probe volume, built on first use and freed when unused (ADR-0326
// point 2, ADR-0316), read through ../src/device_internal.h. Headless.
//
// THE FIRST BEGIN WANTS, THE NEXT FRAME BUILDS. A begin onto the window leaves its
// volume wanted and not built; the next frame's top builds it, and a begin then
// places its probes. A begin onto a target that frame leaves the target's wanted
// and not built while the window's is built: the two are apart, and once both
// are built their images differ. A sum image is 48 wide: two texels a probe.
//
// UNUSED, FREED. After VOE_RENDER_BOUNCE_IDLE (300) frames with no begin both are
// still built; the top of the frame after frees them, and wants nothing more.
//
// EACH RELIT INTO ITS OWN RECORD (ADR-0330 point 2). Both wanted again and built,
// one frame begins the window's and the target's at different cells, a sun of
// bounces 1, relighting each after its begin: that slot's record region 0 holds
// the window's cell and the target's region its own, not the last one's.
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

// The sum's x image holds two RGBA16F texels a probe along x (ADR-0327): its
// memory at least 48 × 12 × 24 × 8 bytes, which a 24-wide image never needs.
static void sum_is_wide(voe_render_device *device,
			const struct voe_render_bounce_volume *volume)
{
	VkMemoryRequirements requirements = { 0 };

	voe_render_vk.get_image_memory_requirements(
		device->device, volume->irradiance[6][0].image, &requirements);
	printf("a sum image takes %llu bytes\n",
	       (unsigned long long)requirements.size);
	VOE_TEST_CHECK(requirements.size >=
		       (VkDeviceSize)2 * VOE_RENDER_BOUNCE_PROBES_XZ *
			       VOE_RENDER_BOUNCE_PROBES_Y *
			       VOE_RENDER_BOUNCE_PROBES_XZ * 8);
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
	VOE_TEST_CHECK(window->irradiance[6][2].image != VK_NULL_HANDLE);
	sum_is_wide(device, window);
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

// Volume `index`'s region of the open slot's record buffer holds `cell`, wrapped.
static void region_holds(const voe_render_device *device, uint32_t index,
			 const int32_t cell[3])
{
	const uint32_t size[3] = { VOE_RENDER_BOUNCE_PROBES_XZ,
				   VOE_RENDER_BOUNCE_PROBES_Y,
				   VOE_RENDER_BOUNCE_PROBES_XZ };
	const struct voe_render_relight_record *record =
		(const void *)((const unsigned char *)
				       device->relight_records_mapped[device->slot] +
			       index * device->relight_record_stride);

	printf("region %u holds cell %u %u %u\n", index, record->cell[0],
	       record->cell[1], record->cell[2]);
	for (uint32_t a = 0; a < 3; a++)
		VOE_TEST_CHECK(record->cell[a] ==
			       voe_render_bounce_probe_wrap(cell[a], size[a]));
}

static void each_relit_apart(voe_render_device *device, voe_render_target id)
{
	struct voe_render_bounce_frame moved = BOUNCE;
	bool drawing = false;

	moved.cell[0] += 5;
	moved.corner.x += 5 * VOE_RENDER_BOUNCE_SPACING;
	one_frame(device, true, true, id);
	VOE_TEST_CHECK(voe_render_frame_begin(device, (voe_platform_size){ SIDE, SIDE },
					      &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return;
	VOE_TEST_CHECK(device->window_volume.built &&
		       device->targets[0].volume.built);
	voe_render_bounce_begin(device, VOE_RENDER_TARGET_WINDOW, &BOUNCE);
	voe_render_bounce_relight(device);
	voe_render_bounce_begin(device, id, &moved);
	voe_render_bounce_relight(device);
	region_holds(device, 0, BOUNCE.cell);
	region_holds(device, id.index, moved.cell);
	VOE_TEST_CHECK(voe_render_frame_end(device));
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
		if (device->output_layer) {
			built_on_first_use(device, id);
			each_relit_apart(device, id);
		} else {
			nothing_without_output_layer(device, id);
		}
		voe_render_device_destroy(device);
	}
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
