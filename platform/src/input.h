// The input state both backends fill and neither reads. See
// include/platform/input.h for what a caller sees; this is the shape underneath
// it, and it is the same shape on both platforms because an evdev scancode and a
// Windows virtual key are two ways of saying the same thing.
//
// THE POINT OF THIS FILE IS THAT THE PUBLIC FUNCTIONS ARE WRITTEN ONCE. A held
// key, a motion accumulator and a lock flag do not differ between Wayland and
// Win32; what differs is where the events come from. So each backend translates
// its own events into this struct and defines voe_platform_window_input below,
// and input.c holds every function in the public header — one copy, on both
// platforms, with no #ifdef in it.
//
// Two copies of eight tiny accessors is the alternative and it is worse than it
// sounds: the interesting behaviour here is not reading a bool, it is clearing
// the whole struct when focus goes away and zeroing the accumulator on every
// poll. Those are the two places an input layer leaks state, and they are worth
// having in one place where a reader can see both.
#pragma once

#include <platform/input.h>

// What the OS has told us, folded flat. Nothing in here is a queue.
//
// THE MOTION ACCUMULATOR IS THE ONE FIELD THAT IS NOT PLAIN STATE, AND IT IS
// DRAINED RATHER THAN OVERWRITTEN. A window system reports mouse movement as a
// series of small deltas and there is no "where the mouse is" to ask for once
// the pointer is locked, so the deltas are summed here and the sum is zeroed at
// the top of every poll. A poll that arrives with three motion events therefore
// leaves their total, which is what a frame wants, and a poll with none leaves
// zero rather than the frame before last's number.
struct voe_platform_input {
	bool keys[VOE_PLATFORM_KEY_COUNT];
	float motion_x;
	float motion_y;

	// What the window system says, not what was asked for. Windows sets it
	// when it clips the cursor because nothing there can refuse; Wayland
	// sets it from the compositor's locked and unlocked events, which is
	// why it is not simply the argument to _lock_pointer.
	bool pointer_locked;
};

// Each backend defines this over its own struct voe_platform_window, and it is
// the whole of what input.c needs to know about either of them. Never NULL for a
// window that opened.
struct voe_platform_input *voe_platform_window_input(voe_platform_window *window);

// Each backend defines this too: ask the window system for the pointer, or give
// it back. Called by voe_platform_input_lock_pointer once the argument has been
// checked, so a backend never sees a NULL window here.
void voe_platform_window_lock_pointer(voe_platform_window *window, bool lock);

// Called by a backend at the top of its poll, before any event is dispatched. It
// zeroes the motion accumulated for the previous frame and leaves held keys
// alone — a key does not stop being held because a frame went by.
void voe_platform_input_begin_poll(struct voe_platform_input *input);

// Called by a backend when the window loses keyboard focus, and this is the
// function the card was pointing at.
//
// FOCUS LOST WITH A KEY STILL DOWN IS WHERE AN INPUT LAYER LEAKS STATE, AND THE
// LEAK IS NOT A BUG IN THE WINDOW SYSTEM. Alt-tab away while holding W and the
// release event is delivered to whoever has focus now, which is not us — so
// without this, W is held for ever and a camera flies off on its own with
// nobody touching the keyboard. Both window systems say when focus goes, both
// backends call this, and every key goes up at once. That is also why a key that
// was genuinely still held when focus came back reads as up until it is pressed
// again: there is no event that would say otherwise, and up is the safe answer.
void voe_platform_input_focus_lost(struct voe_platform_input *input);
