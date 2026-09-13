// The one thing about a material that is not a straight copy into its record:
// the UV rect, and what a material that never mentions one reads.
//
// THE CLAIM IS THAT EVERY MATERIAL WRITTEN BEFORE CARD 022 STILL READS ITS WHOLE
// PICTURE. Every material in this engine is built by naming the fields it cares
// about and letting C zero the rest, so the day a scale arrived, a scale of
// nothing had to mean one. Getting that wrong is not a build error and not a
// crash: it is every textured surface in the engine sampling one texel, which
// looks like a flat colour and reads as a lighting bug.
//
// IT NEEDS A GRAPHICS CARD TO CHECK A DECISION MADE ON THE CPU, and that is
// worth a sentence. The rule lives in voe_3d_material_upload, which cannot be
// called without a device because its whole job is to make a record; the check
// is then on what upload wrote back into the material, not on anything the GPU
// did with it. A headless device is the cheapest way to get one — no window, no
// compositor, nothing to close.
//
// IT SKIPS WHEN THERE IS NO GRAPHICS CARD, for the reason 3d/tests/import.c
// gives at length: a build box with no Vulkan is the box and not this engine.
#include <3d/material_component.h>
#include <base/arena.h>
#include <base/error.h>
#include <render/device.h>

#include <testing/test.h>

#include <stdio.h>

// Enough for the device's startup enumerations and nothing else; this file
// builds no world and reads no file.
#define SCRATCH (64 * 1024)

#define SIDE 16
#define TOLERANCE 1e-6f

// One record is all this needs, and the pools are asked for the least the device
// will take rather than for room this file never fills.
static const voe_render_capacities CAPACITIES = {
	.vertices = 4,
	.indices = 6,
	.geometries = 1,
	.objects = 1,
	.shadings = 4,
	.passes = 1,
};

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_platform_size size = { SIDE, SIDE };
	voe_render_device *device;
	voe_base_error error = VOE_BASE_OK;
	// A material of the shape every call site in this engine writes: the
	// fields it cares about, and nothing said about the rest.
	voe_3d_material silent = {
		.base_colour = { 1.0f, 1.0f, 1.0f, 1.0f },
		.roughness = 1.0f,
	};
	// And one that does say, as a sprite sheet's second cell of four across
	// and two down would.
	voe_3d_material framed = {
		.base_colour = { 1.0f, 1.0f, 1.0f, 1.0f },
		.roughness = 1.0f,
		.base_colour_uv_offset = { 0.25f, 0.0f },
		.base_colour_uv_scale = { 0.25f, 0.5f },
	};
	// Half a rect: x said, y forgotten. Component by component, so y is the
	// whole of y and x is what was asked for.
	voe_3d_material half = {
		.base_colour = { 1.0f, 1.0f, 1.0f, 1.0f },
		.roughness = 1.0f,
		.base_colour_uv_scale = { 0.5f, 0.0f },
	};

	device = voe_render_device_new_headless(arena, size, CAPACITIES, &error);
	if (device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED) {
			printf("skip: %s\n", voe_base_error_string(error));
			voe_base_arena_destroy(arena);
			return voe_test_result();
		}
		VOE_TEST_CHECK(device != NULL);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}

	VOE_TEST_CHECK(voe_3d_material_upload(device, &silent, &error));
	VOE_TEST_CHECK_FLOAT(silent.base_colour_uv_offset.x, 0.0f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(silent.base_colour_uv_offset.y, 0.0f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(silent.base_colour_uv_scale.x, 1.0f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(silent.base_colour_uv_scale.y, 1.0f, TOLERANCE);

	VOE_TEST_CHECK(voe_3d_material_upload(device, &framed, &error));
	VOE_TEST_CHECK_FLOAT(framed.base_colour_uv_offset.x, 0.25f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(framed.base_colour_uv_offset.y, 0.0f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(framed.base_colour_uv_scale.x, 0.25f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(framed.base_colour_uv_scale.y, 0.5f, TOLERANCE);

	VOE_TEST_CHECK(voe_3d_material_upload(device, &half, &error));
	VOE_TEST_CHECK_FLOAT(half.base_colour_uv_scale.x, 0.5f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(half.base_colour_uv_scale.y, 1.0f, TOLERANCE);

	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
