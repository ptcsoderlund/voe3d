// The pipeline cache as bytes out and in (ADR-0370 point 6). Device A prepares
// and hands its cache out; headless device B seeds those bytes, which it takes
// (true), and prepares; device C is handed them with one payload byte flipped,
// refuses them (false) and still prepares. On C too, before its first step: a
// 10-byte buffer and NULL with 0 are refused and change nothing.
//
// Headless, every device of its own so each starts unprepared. A machine with
// no usable Vulkan skips and says so.
#include "../src/device_internal.h"

#include <base/arena.h>
#include <base/error.h>
#include <platform/window.h>

#include <testing/test.h>

#include <stdio.h>
#include <string.h>

// Enough for a device to open; nothing is drawn.
static const voe_render_capacities CAPACITIES = {
	.vertices = 4,
	.indices = 6,
	.geometries = 1,
	.objects = 1,
	.shadings = 1,
	.elements = 1,
	.passes = 1,
};

// Render's own header, before the driver's payload; pipeline_cache.c's.
#define HEADER_BYTES 56

// A headless device, or NULL; `skipped` says whether that was no Vulkan here.
static voe_render_device *open_device(voe_base_arena *arena, bool *skipped)
{
	voe_platform_size size = { 16, 16 };
	voe_base_error error = VOE_BASE_OK;
	voe_render_device *device =
		voe_render_device_new_headless(arena, size, CAPACITIES, &error);

	*skipped = device == NULL && (error == VOE_BASE_ERROR_UNAVAILABLE ||
				      error == VOE_BASE_ERROR_UNSUPPORTED);
	if (*skipped)
		printf("skip: %s\n", voe_base_error_string(error));
	else
		VOE_TEST_CHECK(device != NULL);
	return device;
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_render_device *device;
	const void *handed;
	unsigned char *bytes;
	size_t size = 0;
	bool skipped = false;

	// A: prepare, then hand the cache out.
	device = open_device(arena, &skipped);
	if (device == NULL)
		goto out;
	VOE_TEST_CHECK(voe_render_device_ready(device));
	handed = voe_render_device_cache_bytes(device, arena, &size);
	voe_render_device_destroy(device);
	VOE_TEST_CHECK(handed != NULL);
	VOE_TEST_CHECK(size > HEADER_BYTES);
	if (handed == NULL || size <= HEADER_BYTES)
		goto out;
	bytes = voe_base_arena_push(arena, size);
	memcpy(bytes, handed, size);

	// B: the same bytes are taken, and it prepares on them.
	device = open_device(arena, &skipped);
	if (device == NULL)
		goto out;
	VOE_TEST_CHECK(voe_render_device_cache_seed(device, bytes, size));
	VOE_TEST_CHECK(voe_render_device_ready(device));
	voe_render_device_destroy(device);

	// C: a flipped payload byte, a short buffer and none are all refused, and
	// it still prepares.
	device = open_device(arena, &skipped);
	if (device == NULL)
		goto out;
	bytes[HEADER_BYTES + (size - HEADER_BYTES) / 2] ^= 0x01u;
	VOE_TEST_CHECK(!voe_render_device_cache_seed(device, bytes, size));
	VOE_TEST_CHECK(!voe_render_device_cache_seed(device, bytes, 10));
	VOE_TEST_CHECK(!voe_render_device_cache_seed(device, NULL, 0));
	VOE_TEST_CHECK(voe_render_device_ready(device));
	voe_render_device_destroy(device);

out:
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
