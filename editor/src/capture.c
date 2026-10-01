// The capture's frame count and its two writers, as capture.h gives them.
//
// Constraints: each write's scratch arena is made and destroyed inside the
// call; nothing it holds outlives the file.
#include "capture.h"

#include <base/arena.h>
#include <base/error.h>

#include <render/device.h>

// The capture's working memory: the pixels off the card, the encoder's tables
// and the file's bytes all come out of it and none of them outlives the write
// (app.h). A block size, not a limit — a picture larger than it gets a block of
// its own.
#define CAPTURE_SCRATCH (4u * 1024u * 1024u)

bool voe_editor_capture_enough(const char *path, unsigned *frames,
			       unsigned reach)
{
	return path != NULL && ++*frames == reach;
}

// Nothing is printed on a failure: the readback, the encoder and the file write
// each say on stderr what refused.
bool voe_editor_capture_write_view(voe_app *app, voe_render_target target,
				   const char *path)
{
	voe_base_error error;
	voe_base_arena *scratch;
	bool written;

	if (path == NULL)
		return true;
	scratch = voe_base_arena_new(CAPTURE_SCRATCH);
	written = voe_app_capture_png(app, target, scratch, path, &error);
	voe_base_arena_destroy(scratch);
	return written;
}

bool voe_editor_capture_write(voe_app *app, const char *path)
{
	return voe_editor_capture_write_view(app, VOE_RENDER_TARGET_WINDOW,
					     path);
}
