// The one read of the keyboard, once a frame. See the header for why every key
// edge in this program is found here, why an edge is tracked even on the frames
// nobody may act on it, and why what an edge means is the caller's.
#include "keys.h"

#include <base/assert.h>

#include <stddef.h>

void voe_editor_keys_read(voe_editor_keys *keys, voe_platform_window *window,
			  voe_editor_keys_frame *out)
{
	VOE_BASE_ASSERT(keys != NULL, "reading the keyboard into no keys");
	VOE_BASE_ASSERT(out != NULL, "reading the keyboard into no frame");

	for (int key = 0; key < VOE_PLATFORM_KEY_COUNT; key++) {
		bool down = window != NULL &&
			    voe_platform_input_key_down(
				    window, (voe_platform_key)key);

		out->down[key] = down;
		out->pressed[key] = down && !keys->was_down[key];
		keys->was_down[key] = down;
	}
}
