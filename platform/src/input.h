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
// Two copies of a dozen tiny accessors is the alternative and it is worse than
// it sounds: the interesting behaviour here is not reading a bool, it is
// clearing the keys when focus goes away, clearing the buttons when the pointer
// goes away, and zeroing the accumulator on every poll. Those are the three
// places an input layer leaks state, and they are worth having in one place
// where a reader can see all of them.
#pragma once

#include <platform/input.h>

// What the OS has told us, folded flat. Nothing in here is a queue.
//
// THE MOTION AND WHEEL ACCUMULATORS ARE THE FIELDS THAT ARE NOT PLAIN STATE, AND
// THEY ARE DRAINED RATHER THAN OVERWRITTEN. A window system reports mouse movement as a
// series of small deltas and there is no "where the mouse is" to ask for once
// the pointer is locked, so the deltas are summed here and the sum is zeroed at
// the top of every poll. A poll that arrives with three motion events therefore
// leaves their total, which is what a frame wants, and a poll with none leaves
// zero rather than the frame before last's number.
//
// THE POINTER POSITION IS PLAIN STATE AND IS OVERWRITTEN, NEVER DRAINED. Where
// the pointer is does not stop being true because a frame went by, so a poll
// leaves it alone; each backend writes it from its own position event, already
// in the window's pixels, and pointer_over says whether the number is live — see
// include/platform/input.h for the three things that make it stale. The lock is
// folded in by input.c when a caller asks, not here: pointer_over is what the
// window system said about the surface, and the two are kept apart so that
// releasing the lock leaves pointer_over as it was.
struct voe_platform_input {
	bool keys[VOE_PLATFORM_KEY_COUNT];
	float motion_x;
	float motion_y;

	// The wheel, in notches, in the public header's sign. Drained by the
	// poll exactly as motion is, and for the same reason.
	float wheel_x;
	float wheel_y;

	float pointer_x;
	float pointer_y;
	bool pointer_over;
	bool buttons[VOE_PLATFORM_BUTTON_COUNT];

	// What the window system says, not what was asked for. Windows sets it
	// when it clips the cursor because nothing there can refuse; Wayland
	// sets it from the compositor's locked and unlocked events, which is
	// why it is not simply the argument to _lock_pointer.
	bool pointer_locked;

	// The typed text, in the public header's UTF-8 bytes and order.
	// Appended to by voe_platform_input_append_text below, drained by
	// voe_platform_input_begin_poll exactly as the wheel is, and emptied by
	// voe_platform_input_focus_lost too — see include/platform/input.h for
	// why. text_size is the count of bytes filled, never the buffer's
	// capacity.
	char text[256];
	uint32_t text_size;

	// The pointer's shape last asked for. Plain state: no poll, focus loss
	// or pointer loss clears it, and a backend reapplies it on enter.
	voe_platform_cursor cursor;
};

// Each backend defines this over its own struct voe_platform_window, and it is
// the whole of what input.c needs to know about either of them. Never NULL for a
// window that opened.
struct voe_platform_input *voe_platform_window_input(voe_platform_window *window);

// Each backend defines this too: ask the window system for the pointer, or give
// it back. Called by voe_platform_input_lock_pointer once the argument has been
// checked, so a backend never sees a NULL window here.
void voe_platform_window_lock_pointer(voe_platform_window *window, bool lock);

// Each backend defines this as well: show the shape now, if the pointer is over
// the window. Called by voe_platform_input_cursor only when the shape changed,
// after it is stored, so a backend never sees a NULL window or an invalid shape.
void voe_platform_window_cursor(voe_platform_window *window,
				voe_platform_cursor cursor);

// Called by a backend at the top of its poll, before any event is dispatched. It
// zeroes the motion and the wheel accumulated for the previous frame and leaves
// held keys alone — a key does not stop being held because a frame went by.
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

// Called by a backend when the pointer leaves the window — Wayland's
// wl_pointer.leave, Windows' WM_MOUSELEAVE or a capture taken away — and when
// the pointer itself goes, a mouse unplugged from the seat.
//
// THE BUTTONS GO UP FOR THE SAME REASON THE KEYS DO ON FOCUS LOST. A button held
// when the pointer leaves has its release delivered to whoever has the pointer
// now, which is not us, and a button that never comes up is a drag that never
// ends. Ordinarily neither window system lets this happen — both keep the
// pointer on the surface that took the press until the release — so this is the
// path for a compositor or a capture that broke the rule, and up is the safe
// answer.
//
// THE POSITION IS KEPT AND ONLY pointer_over IS CLEARED. The last place the
// pointer was seen is what a GUI uses to tell an edge from an absence, and the
// public header promises it.
void voe_platform_input_pointer_lost(struct voe_platform_input *input);

// Encodes one Unicode code point as UTF-8 and appends it to input->text,
// which is what makes this the one function both backends call to type a
// character: window_wayland.c has already turned a scancode and a shift
// level into a code point through src/keymap.h, and window_win32.c has
// already joined a WM_CHAR surrogate pair into one — neither backend touches
// UTF-8 itself past this point.
//
// A CODE POINT THIS CANNOT FORM IS DROPPED WHOLE, AND SO IS ONE THAT WOULD
// NOT FIT. Below 0x20, 0x7f, a surrogate (0xd800..0xdfff) and anything past
// 0x10ffff are never valid text and are dropped before any encoding is
// attempted; a code point that would encode but not fit in what capacity is
// left in the buffer is dropped too; there is no such thing as writing half
// of one.
void voe_platform_input_append_text(struct voe_platform_input *input,
				    uint32_t code_point);
