// The engine's splash read from its source folder — see splash.h.
//
// The path is joined in `scratch` from toolchain.h's engine folder, as
// game_tree.c reads the same macro; app/picture.h does the read.
#include "splash.h"

#include "toolchain.h"

#include <base/assert.h>
#include <base/error.h>
#include <base/report.h>

#include <platform/path.h>

bool voe_editor_splash_read(voe_render_device *device, voe_base_arena *scratch,
			    voe_app_picture *out)
{
	voe_base_error error;
	const char *path;

	VOE_BASE_ASSERT(device != NULL && scratch != NULL && out != NULL,
			"reading the splash with no device, scratch or picture");
	path = voe_platform_path_join(
		scratch,
		voe_platform_path_join(
			scratch,
			voe_platform_path_join(scratch, VOE_TOOLCHAIN_ENGINE,
					       "game"),
			"src"),
		"splashscreen.png");
	VOE_BASE_ASSERT(path != NULL, "the splash's path was not joined");

	if (!voe_app_picture_read(device, path, scratch, out, &error)) {
		VOE_BASE_ERROR("editor", "no splash at %s (%s); the plain screen shows",
			       path, voe_base_error_string(error));
		return false;
	}
	VOE_BASE_ASSERT(out->width > 0 && out->height > 0,
			"a splash read with no area");
	return true;
}
