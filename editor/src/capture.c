// The capture's frame count and its one write, as capture.h gives them.
//
// Constraints: the write's scratch arena is made and destroyed inside the
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

// How many frames a capture runs before it writes; capture.h says why two.
#define CAPTURE_FRAMES 2

bool voe_editor_capture_enough(const char *path, unsigned *frames)
{
	return path != NULL && ++*frames == CAPTURE_FRAMES;
}

// Nothing is printed on a failure: the readback, the encoder and the file write
// each say on stderr what refused.
bool voe_editor_capture_write(voe_app *app, const char *path)
{
	voe_base_error error;
	voe_base_arena *scratch;
	bool written;

	if (path == NULL)
		return true;
	scratch = voe_base_arena_new(CAPTURE_SCRATCH);
	written = voe_app_capture_png(app, VOE_RENDER_TARGET_WINDOW, scratch,
				      path, &error);
	voe_base_arena_destroy(scratch);
	return written;
}
