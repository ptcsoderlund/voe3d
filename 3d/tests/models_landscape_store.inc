// The model store's landscape tests' harness, compiled into each program that
// includes it: models_landscape.c and models_landscape_foliage.c. It holds the
// device's capacities, the empty rect, one frame of landscape writes and the
// headless device each program opens; the cases stay in the programs. It
// brings its own includes, so a program needs only this line for them.
//
// THE GRID IS 16 METRES OF 8 CELLS, a cell 2 m, unless a case says otherwise.
//
// It reads the store's held dirt and pyramid through models_store.h.
//
// THE DEVICE SKIPS WHEN THERE IS NO GRAPHICS CARD, because a load uploads:
// a_device prints the reason and gives NULL, and the program returns its result.
#include "../src/models_store.h"

#include <3d/foliage.h>
#include <3d/landscape.h>
#include <3d/models.h>
#include <3d/models_foliage.h>
#include <assets/landscape.h>
#include <base/arena.h>
#include <base/error.h>
#include <render/device.h>

#include <testing/test.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define SCRATCH (1024 * 1024)
#define SIDE 16
#define GRASS "Ground/grass.material"
#define ROCK "Mats/rock.material"

static const voe_render_capacities CAPACITIES = {
	.vertices = 33 * 33,
	.indices = 32 * 32 * 6,
	.geometries = 1,
	.objects = 8,
	.shadings = 12,
	.passes = 1,
	.heights_texels = VOE_3D_LANDSCAPE_WRITE_TEXELS,
};

static bool is_empty(voe_3d_landscape_rect rect)
{
	return rect.x0 >= rect.x1 || rect.z0 >= rect.z1;
}

// One frame with nothing but the landscape writes in it.
static bool one_frame(voe_3d_models *models, voe_render_device *device)
{
	voe_platform_size size = { SIDE, SIDE };
	bool drawing = false;

	if (!voe_render_frame_begin(device, size, &drawing) || !drawing)
		return false;
	voe_3d_models_landscape_frame(models, device);
	return voe_render_frame_end(device);
}

// The headless device with CAPACITIES, or NULL: skipped with the reason
// printed when there is no card, a failed check otherwise.
static voe_render_device *a_device(voe_base_arena *arena)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	voe_render_device *device;

	device = voe_render_device_new_headless(arena, size, CAPACITIES, &error);
	if (device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED)
			printf("skip: %s\n", voe_base_error_string(error));
		else
			VOE_TEST_CHECK(device != NULL);
	}
	return device;
}
