// Every function in include/platform/input.h, once, for both platforms. What
// makes that possible is src/input.h: a backend fills a struct
// voe_platform_input and hands it over, and nothing below knows which window
// system put the values there.
//
// SO THERE IS NO #ifdef IN THIS FILE AND THERE MUST NOT BE ONE. The moment
// something here needs to know which OS it is on, the split in src/input.h is in
// the wrong place — the fix is to move that thing into the two backends, not to
// reach for the preprocessor.
#include "input.h"

#include <base/assert.h>

#include <string.h>

bool voe_platform_input_key_down(voe_platform_window *window,
				 voe_platform_key key)
{
	struct voe_platform_input *input;

	VOE_BASE_DEBUG_ASSERT(window != NULL, "asking a NULL window for a key");
	VOE_BASE_DEBUG_ASSERT(key < VOE_PLATFORM_KEY_COUNT,
			      "VOE_PLATFORM_KEY_COUNT is a count, not a key");

	input = voe_platform_window_input(window);
	return input->keys[key];
}

voe_platform_motion voe_platform_input_motion(voe_platform_window *window)
{
	struct voe_platform_input *input;

	VOE_BASE_DEBUG_ASSERT(window != NULL, "asking a NULL window for motion");

	// Not zeroed here. Reading is not draining: two callers in one frame
	// must see the same number, and the poll is the one place that knows a
	// frame has gone by. See voe_platform_input_begin_poll.
	input = voe_platform_window_input(window);
	return (voe_platform_motion){ input->motion_x, input->motion_y };
}

voe_platform_wheel voe_platform_input_wheel(voe_platform_window *window)
{
	struct voe_platform_input *input;

	VOE_BASE_DEBUG_ASSERT(window != NULL, "asking a NULL window for the wheel");

	// Not zeroed here either, for the same reason motion is not.
	input = voe_platform_window_input(window);
	return (voe_platform_wheel){ input->wheel_x, input->wheel_y };
}

voe_platform_pointer voe_platform_input_pointer(voe_platform_window *window)
{
	struct voe_platform_input *input;

	VOE_BASE_DEBUG_ASSERT(window != NULL,
			      "asking a NULL window where the pointer is");

	// The lock is folded in here and not in the backends, so that both
	// platforms give the same answer for the same reason: a locked pointer
	// is one the camera has, and there is nothing at its position to point
	// at. pointer_over itself is left as the window system reported it, so
	// releasing the lock needs no event to bring the position back.
	input = voe_platform_window_input(window);
	return (voe_platform_pointer){
		.x = input->pointer_x,
		.y = input->pointer_y,
		.over = input->pointer_over && !input->pointer_locked,
	};
}

bool voe_platform_input_button_down(voe_platform_window *window,
				    voe_platform_button button)
{
	struct voe_platform_input *input;

	VOE_BASE_DEBUG_ASSERT(window != NULL, "asking a NULL window for a button");
	VOE_BASE_DEBUG_ASSERT(button < VOE_PLATFORM_BUTTON_COUNT,
			      "VOE_PLATFORM_BUTTON_COUNT is a count, not a button");

	input = voe_platform_window_input(window);
	return input->buttons[button];
}

void voe_platform_input_lock_pointer(voe_platform_window *window, bool lock)
{
	VOE_BASE_DEBUG_ASSERT(window != NULL, "locking a NULL window's pointer");

	// Straight through, every time it is asked, and deliberately not
	// filtered against the current state. On Wayland the compositor may have
	// taken a lock away for reasons of its own, so a caller that asks every
	// frame is a caller that gets it back — and the backends make asking for
	// what is already the case cost nothing.
	voe_platform_window_lock_pointer(window, lock);
}

bool voe_platform_input_pointer_locked(voe_platform_window *window)
{
	struct voe_platform_input *input;

	VOE_BASE_DEBUG_ASSERT(window != NULL, "asking a NULL window about a lock");

	input = voe_platform_window_input(window);
	return input->pointer_locked;
}

void voe_platform_input_begin_poll(struct voe_platform_input *input)
{
	VOE_BASE_DEBUG_ASSERT(input != NULL, "beginning a poll on nothing");

	// The position, the buttons and whether the pointer is over the window
	// are deliberately not touched: they are facts about now, not a sum
	// over the frame, and stay true until the window system says otherwise.
	input->motion_x = 0.0f;
	input->motion_y = 0.0f;
	input->wheel_x = 0.0f;
	input->wheel_y = 0.0f;
}

void voe_platform_input_focus_lost(struct voe_platform_input *input)
{
	VOE_BASE_DEBUG_ASSERT(input != NULL, "losing focus on nothing");

	// The keys and nothing else. The pointer lock is not ours to clear —
	// the window system says whether it is still held, and on Wayland it
	// may well hand it back when focus returns without being asked again.
	memset(input->keys, 0, sizeof(input->keys));

	// The motion of a frame that ended with focus going away is thrown out
	// with the keys. Alt-tab drags the pointer across the surface on some
	// compositors, and a camera that snapped round on the way out would look
	// exactly like a bug in the camera.
	input->motion_x = 0.0f;
	input->motion_y = 0.0f;
}

void voe_platform_input_pointer_lost(struct voe_platform_input *input)
{
	VOE_BASE_DEBUG_ASSERT(input != NULL, "losing the pointer on nothing");

	// The buttons, and the fact that the pointer was over the window. The
	// position stays: it is where the pointer was last seen, and the public
	// header promises that number to a GUI that wants to tell an edge from
	// an absence. The keys are the keyboard's and are not touched — a
	// pointer walking out of the window does not lift a finger off W.
	memset(input->buttons, 0, sizeof(input->buttons));
	input->pointer_over = false;
}
