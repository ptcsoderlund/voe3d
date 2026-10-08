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
// EACH AT ITS OWN SPACING (0332 point 3). The next frame begins the window's at
// 2 m and the target's at 4, a new sun strength relighting both: that slot's
// region 0 holds spacing 2 and the target's region spacing 4, as its slot's
// begin keeps.
//
// TWO VOLUMES OF ONE TARGET APART (0389 point 1). One frame begins the window's
// volumes 0 and 3 at 2 m and 1 m, each captured and relit after its own begin,
// frame on frame until one captures and relights nothing: both are built, and
// voe_render_bounce_placed gives back each one's own lowest cell.
//
// AN UNPLACED VOLUME IS NOT PLACED: one never begun says false and leaves the
// cell alone, as does the window's volume 0 once freed.
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
// Frames two volumes may take to settle: about 108 each to capture, 16 to fade.
#define FRAMES_MAX 400

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
	.spacing = VOE_RENDER_BOUNCE_SPACING,
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
	const struct voe_render_bounce_volume *window = &device->window_volume[0];
	const struct voe_render_bounce_volume *own = &device->targets[0].volume[0];
	int32_t cell[3] = { 0 };

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
	VOE_TEST_CHECK(!voe_render_bounce_placed(device, VOE_RENDER_TARGET_WINDOW,
						 0, cell));
}

// Volume `index`'s region of the open slot's record buffer holds `cell`, wrapped,
// and `spacing`.
static void region_holds(const voe_render_device *device, uint32_t index,
			 const int32_t cell[3], float spacing)
{
	const uint32_t size[3] = { VOE_RENDER_BOUNCE_PROBES_XZ,
				   VOE_RENDER_BOUNCE_PROBES_Y,
				   VOE_RENDER_BOUNCE_PROBES_XZ };
	const struct voe_render_relight_record *record =
		(const void *)((const unsigned char *)
				       device->relight_records_mapped[device->slot] +
			       index * device->relight_record_stride);

	printf("region %u holds cell %u %u %u at spacing %g\n", index,
	       record->cell[0], record->cell[1], record->cell[2],
	       (double)record->spacing);
	for (uint32_t a = 0; a < 3; a++)
		VOE_TEST_CHECK(record->cell[a] ==
			       voe_render_bounce_probe_wrap(cell[a], size[a]));
	VOE_TEST_CHECK(record->spacing == spacing);
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
	VOE_TEST_CHECK(device->window_volume[0].built &&
		       device->targets[0].volume[0].built);
	voe_render_bounce_begin(device, VOE_RENDER_TARGET_WINDOW, &BOUNCE);
	voe_render_bounce_relight(device);
	voe_render_bounce_begin(device, id, &moved);
	voe_render_bounce_relight(device);
	region_holds(device, 0, BOUNCE.cell, VOE_RENDER_BOUNCE_SPACING);
	region_holds(device, voe_render_bounce_volume_index(id.index, 0),
		     moved.cell, VOE_RENDER_BOUNCE_SPACING);
	VOE_TEST_CHECK(voe_render_frame_end(device));
}

static void each_at_its_spacing(voe_render_device *device, voe_render_target id)
{
	struct voe_render_bounce_frame window = BOUNCE;
	struct voe_render_bounce_frame coarse = BOUNCE;
	bool drawing = false;

	// A new sun strength relights the window; a new spacing empties the target.
	window.sun_strength = 0.5f;
	coarse.sun_strength = 0.5f;
	coarse.spacing = 4.0f;
	coarse.corner = (voe_math_float3){ -48.0f, -24.0f, -48.0f };
	VOE_TEST_CHECK(voe_render_frame_begin(device, (voe_platform_size){ SIDE, SIDE },
					      &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return;
	voe_render_bounce_begin(device, VOE_RENDER_TARGET_WINDOW, &window);
	voe_render_bounce_relight(device);
	voe_render_bounce_begin(device, id, &coarse);
	voe_render_bounce_relight(device);
	region_holds(device, 0, window.cell, 2.0f);
	region_holds(device, voe_render_bounce_volume_index(id.index, 0),
		     coarse.cell, 4.0f);
	VOE_TEST_CHECK(device->targets[0].volume[0].begun[device->slot].spacing ==
		       4.0f);
	VOE_TEST_CHECK(voe_render_frame_end(device));
}

// One frame beginning each of `begins` onto the window, capturing it until a
// pass does not open, then relighting it. Whether it captured or relit at all.
static bool captured_or_relit(voe_render_device *device,
			      const struct voe_render_bounce_frame *begins,
			      uint32_t count)
{
	const uint32_t before = device->relight_dispatches;
	bool drawing = false;
	bool captured = false;

	VOE_TEST_CHECK(voe_render_frame_begin(device, (voe_platform_size){ SIDE, SIDE },
					      &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return false;
	for (uint32_t i = 0; i < count; i++) {
		bool opened = true;

		voe_render_bounce_begin(device, VOE_RENDER_TARGET_WINDOW, &begins[i]);
		for (uint32_t p = 0; p < VOE_RENDER_BOUNCE_CAPTURE_PASSES && opened;
		     p++) {
			VOE_TEST_CHECK(voe_render_bounce_capture_pass_begin(device,
									    &opened));
			if (opened) {
				voe_render_pass_end(device);
				captured = true;
			}
		}
		voe_render_bounce_relight(device);
	}
	VOE_TEST_CHECK(voe_render_frame_end(device));
	return captured || device->relight_dispatches != before;
}

// The window's volume `volume` was last placed at `expected`.
static void placed_at(const voe_render_device *device, uint32_t volume,
		      const int32_t expected[3])
{
	int32_t cell[3] = { 0 };

	VOE_TEST_CHECK(voe_render_bounce_placed(device, VOE_RENDER_TARGET_WINDOW,
						volume, cell));
	printf("volume %u placed at %d %d %d\n", volume, cell[0], cell[1],
	       cell[2]);
	for (uint32_t a = 0; a < 3; a++)
		VOE_TEST_CHECK_INT(cell[a], expected[a]);
}

static void two_volumes_of_one_target_build_and_relight_apart(
	voe_render_device *device)
{
	struct voe_render_bounce_frame begins[2] = { BOUNCE, BOUNCE };
	uint32_t frames = 0;

	begins[1].volume = 3;
	begins[1].spacing = 1.0f;
	begins[1].cell[1] = -9;
	begins[1].corner = (voe_math_float3){ -12.0f, -9.0f, -12.0f };
	// The first frame wants volume 3; the next builds it.
	captured_or_relit(device, begins, 2);
	while (frames < FRAMES_MAX && captured_or_relit(device, begins, 2))
		frames++;
	printf("two volumes settled in %u frames\n", frames);
	VOE_TEST_CHECK(frames > 0 && frames < FRAMES_MAX);
	VOE_TEST_CHECK(device->window_volume[0].built &&
		       device->window_volume[3].built);
	VOE_TEST_CHECK(device->window_volume[0].albedo.image !=
		       device->window_volume[3].albedo.image);
	placed_at(device, 0, begins[0].cell);
	placed_at(device, 3, begins[1].cell);
}

static void an_unplaced_volume_is_not_placed(const voe_render_device *device,
					     voe_render_target id)
{
	int32_t cell[3] = { 7, 7, 7 };

	VOE_TEST_CHECK(!voe_render_bounce_placed(device, VOE_RENDER_TARGET_WINDOW,
						 1, cell));
	VOE_TEST_CHECK(!voe_render_bounce_placed(device, id, 2, cell));
	VOE_TEST_CHECK(cell[0] == 7 && cell[1] == 7 && cell[2] == 7);
}

static void nothing_without_output_layer(voe_render_device *device,
					 voe_render_target id)
{
	printf("note: no shaderOutputLayer, so nothing bounces\n");
	one_frame(device, true, true, id);
	one_frame(device, false, false, id);
	VOE_TEST_CHECK(!device->window_volume[0].wanted &&
		       !device->window_volume[0].built);
	VOE_TEST_CHECK(!device->targets[0].volume[0].built);
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
		an_unplaced_volume_is_not_placed(device, id);
		if (device->output_layer) {
			built_on_first_use(device, id);
			each_relit_apart(device, id);
			each_at_its_spacing(device, id);
			two_volumes_of_one_target_build_and_relight_apart(device);
		} else {
			nothing_without_output_layer(device, id);
		}
		voe_render_device_destroy(device);
	}
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
