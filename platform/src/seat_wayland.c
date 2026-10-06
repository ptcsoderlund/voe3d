// The seat half of the Wayland window: the keyboard, the pointer, relative
// motion and the lock, and what they fill in src/input.h. The window it serves,
// its registry and its surface are src/window_wayland.c; the struct both halves
// write is src/window_wayland.h.
//
// A SEPARATE FILE OF THE SAME BACKEND, NOT A SEPARATE LAYER. A keyboard and a
// pointer are wl_seat's, wl_seat comes off the same registry as the compositor,
// and every event reaches here through the same wl_display_dispatch_pending that
// a resize does — so this file takes the whole window struct rather than being
// handed the display, the registry and the surface one by one, which would be
// that struct under a second name. It is its own file because the window file
// was past a thousand lines and the seat is the half of it that grows. Only
// two things cross back: voe_platform_seat_listener, which _new attaches to the
// seat the registry bound, and voe_platform_seat_release, which close_down calls.
//
// THE POINTER'S SHAPE IS SET BY NAME through cursor-shape-v1, on enter and on a
// change, and the compositor draws it; without the protocol it is left alone.
//
// A LOCKED POINTER IS HIDDEN ONLY WHEN CURSOR-SHAPE-V1 CAN BRING IT BACK BY
// NAME. Once the lock is reported locked, wl_pointer_set_cursor is given no
// surface; on unlock or release the shape is named again. Without the
// protocol, restoring the arrow would mean drawing one, an image this engine
// does not have, so the pointer stays frozen and visible. The lock freezes it
// in place, so when it shows again it is where it was.
//
// Every listener slot left NULL here is unreachable because window_wayland.c
// binds the seat and both pointer protocols at version 1; its header says why.
//
// _GNU_SOURCE is what makes <sys/mman.h> and <unistd.h> declare mmap and close
// under -std=c23. It is a feature-test macro and not a use of any GNU extension.
#define _GNU_SOURCE

#include "window_wayland.h"
#include "scale.h"

#include <base/assert.h>
#include <base/report.h>

#include "cursor-shape-v1-client-protocol.h"
#include "pointer-constraints-unstable-v1-client-protocol.h"
#include "relative-pointer-unstable-v1-client-protocol.h"

#include <linux/input-event-codes.h>

#include <sys/mman.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>

// An evdev scancode to one of the twelve keys this engine reads, or
// VOE_PLATFORM_KEY_COUNT for the hundred-odd it does not.
//
// A SWITCH AND NOT A TABLE INDEXED BY SCANCODE, BECAUSE THE SCANCODES ARE SPARSE
// AND THE ENGINE'S SET IS TINY. Twelve cases against an array with two hundred
// and fifty entries in it, of which twelve are used; the switch is also the form
// the
// Windows side takes, which makes the two tables readable side by side. Both
// ends of Shift and Control land on one key each, which is
// include/platform/input.h's decision and not this file's.
static voe_platform_key key_of(uint32_t scancode)
{
	switch (scancode) {
	case KEY_W:
		return VOE_PLATFORM_KEY_W;
	case KEY_A:
		return VOE_PLATFORM_KEY_A;
	case KEY_S:
		return VOE_PLATFORM_KEY_S;
	case KEY_D:
		return VOE_PLATFORM_KEY_D;
	case KEY_Q:
		return VOE_PLATFORM_KEY_Q;
	case KEY_E:
		return VOE_PLATFORM_KEY_E;
	case KEY_SPACE:
		return VOE_PLATFORM_KEY_SPACE;
	case KEY_LEFTCTRL:
	case KEY_RIGHTCTRL:
		return VOE_PLATFORM_KEY_CONTROL;
	case KEY_LEFTSHIFT:
	case KEY_RIGHTSHIFT:
		return VOE_PLATFORM_KEY_SHIFT;
	case KEY_TAB:
		return VOE_PLATFORM_KEY_TAB;
	case KEY_ESC:
		return VOE_PLATFORM_KEY_ESCAPE;
	case KEY_P:
		return VOE_PLATFORM_KEY_P;
	case KEY_N:
		return VOE_PLATFORM_KEY_N;
	case KEY_O:
		return VOE_PLATFORM_KEY_O;
	case KEY_BACKSPACE:
		return VOE_PLATFORM_KEY_BACKSPACE;
	case KEY_ENTER:
		return VOE_PLATFORM_KEY_ENTER;
	case KEY_DELETE:
		return VOE_PLATFORM_KEY_DELETE;
	case KEY_Z:
		return VOE_PLATFORM_KEY_Z;
	case KEY_Y:
		return VOE_PLATFORM_KEY_Y;
	case KEY_R:
		return VOE_PLATFORM_KEY_R;
	case KEY_F:
		return VOE_PLATFORM_KEY_F;
	default:
		return VOE_PLATFORM_KEY_COUNT;
	}
}

// Also where AltGr is held, exactly the way the twelve keys above are: this
// runs for every scancode from keyboard_key and, on focus arriving, for every
// scancode keyboard_enter finds already down, so both are covered by the one
// check against the keymap's level3_shift flag (ADR-0169).
static void key_set(voe_platform_window *window, uint32_t scancode, bool down)
{
	voe_platform_key key = key_of(scancode);

	if (key != VOE_PLATFORM_KEY_COUNT)
		window->input.keys[key] = down;

	if (scancode < VOE_PLATFORM_KEYMAP_CODES &&
	    window->keymap.level3_shift[scancode])
		window->altgr_held = down;
}

// A KEY IS A PLACE AND NOT A LETTER, AND THAT STAYS TRUE EVEN THOUGH THIS FILE
// NOW READS TEXT TOO. wl_keyboard.key carries the evdev scancode of the key
// that moved — KEY_W is the key where W sits on a US keyboard whatever the
// layout says it types — and <linux/input-event-codes.h> is where those numbers
// are written down; that is still what fills the keys array. What has changed
// is the keymap event, which used to be closed unread: it is now mapped into
// this process and handed to src/keymap.h's in-house reader (ADR-0161), not to
// xkbcommon — a dependency rule 5 has never asked for and D-245 closed without.
// The file descriptor is mmapped read-only and private, read, then unmapped and
// closed every time, because a leaked descriptor per keymap change is still a
// leak whether or not the map is kept.
//
// The keymap: mapped read-only and private into this process, handed whole to
// src/keymap.h's reader, then unmapped and closed — every time, whether the
// read succeeds or not, because the descriptor and the mapping are owed back
// regardless and the event fires again on every layout change. A read that
// fails leaves the keymap table all zero, per voe_platform_keymap_read, which
// is what makes "reported once" also mean "types nothing" with no separate
// flag to check at every keystroke.
static void keyboard_keymap(void *data, struct wl_keyboard *keyboard,
			    uint32_t format, int32_t fd, uint32_t size)
{
	voe_platform_window *window = data;
	void *mapped;

	(void)keyboard;
	(void)format;

	mapped = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (mapped == MAP_FAILED) {
		if (!window->keymap_reported) {
			VOE_BASE_WARNING("platform",
					 "could not map the Wayland keymap: %s",
					 strerror(errno));
			window->keymap_reported = true;
		}
		memset(&window->keymap, 0, sizeof(window->keymap));
		close(fd);
		return;
	}

	if (!voe_platform_keymap_read(mapped, (size_t)size, &window->keymap) &&
	    !window->keymap_reported) {
		VOE_BASE_WARNING("platform",
				 "the compositor's keymap could not be read; nothing will be typed");
		window->keymap_reported = true;
	}

	munmap(mapped, size);
	close(fd);
}

// Focus arrived, and keys carries what is held at this moment — which is the
// answer to the one case an input layer usually gets wrong in the other
// direction. Alt-tab back into the window while still holding W and W is held
// here, so it reads as held rather than as up until the next press.
//
// The array is uint32_t scancodes, and wl_array_for_each is libwayland's macro
// over it. Nothing is cleared first: focus was lost on the way out and
// voe_platform_input_focus_lost emptied it then.
static void keyboard_enter(void *data, struct wl_keyboard *keyboard,
			   uint32_t serial, struct wl_surface *surface,
			   struct wl_array *keys)
{
	voe_platform_window *window = data;
	uint32_t *scancode;

	(void)keyboard;
	(void)serial;
	(void)surface;

	wl_array_for_each(scancode, keys)
		key_set(window, *scancode, true);
}

// Focus gone, and every key with it. See voe_platform_input_focus_lost in
// src/input.h for why this is the important half.
static void keyboard_leave(void *data, struct wl_keyboard *keyboard,
			   uint32_t serial, struct wl_surface *surface)
{
	voe_platform_window *window = data;

	(void)keyboard;
	(void)serial;
	(void)surface;

	window->altgr_held = false;
	voe_platform_input_focus_lost(&window->input);
}

// The keyboard's half of ADR-0161 and ADR-0169: a press with Control up looks
// the scancode and the level it selects up in the keymap table and appends
// whatever it finds, through the one function that also serves
// window_win32.c. The level is AltGr counted twice plus Shift counted once —
// 0 plain, 1 Shift, 2 AltGr, 3 both — which is the same order the keymap
// reader fills VOE_PLATFORM_KEYMAP_LEVELS in. key_set runs first and updates
// altgr_held for this very key, so a press of the AltGr key itself already
// has the level right, though it never matters: level3_shift is never also a
// range that types (see src/keymap.h). Control held types nothing, so a
// shortcut built on a key never also types a character; a release never
// types, and a code point of 0 — a key the reader could not resolve, or a
// keymap that failed to read at all — appends nothing because
// voe_platform_input_append_text drops it.
static void keyboard_key(void *data, struct wl_keyboard *keyboard,
			 uint32_t serial, uint32_t time, uint32_t key,
			 uint32_t state)
{
	voe_platform_window *window = data;
	bool down = state == WL_KEYBOARD_KEY_STATE_PRESSED;

	(void)keyboard;
	(void)serial;
	(void)time;

	key_set(window, key, down);

	if (down && !window->input.keys[VOE_PLATFORM_KEY_CONTROL] &&
	    key < VOE_PLATFORM_KEYMAP_CODES) {
		uint32_t level = (window->altgr_held ? 2 : 0) +
				 (window->input.keys[VOE_PLATFORM_KEY_SHIFT] ? 1 : 0);
		uint32_t code_point = window->keymap.typed[key][level];

		if (code_point != 0)
			voe_platform_input_append_text(&window->input, code_point);
	}
}

// Which modifiers the compositor thinks are latched, and it is deliberately not
// where Shift, Control or AltGr come from. This event reports the state of the
// *keymap's* modifier groups, which is a question about what a keystroke would
// type; the engine wants whether a physical key is held, and that arrives
// through key like every other key — AltGr included, since ADR-0169 makes it a
// place in the keymap rather than a modifier bit read here. Reading both would
// be two answers to one question and they disagree — Caps Lock latches a
// modifier with no key held.
static void keyboard_modifiers(void *data, struct wl_keyboard *keyboard,
			       uint32_t serial, uint32_t depressed,
			       uint32_t latched, uint32_t locked, uint32_t group)
{
	(void)data;
	(void)keyboard;
	(void)serial;
	(void)depressed;
	(void)latched;
	(void)locked;
	(void)group;
}

// THE SEAT IS BOUND AT VERSION 1 LIKE EVERYTHING ELSE, AND THAT IS WHAT MAKES
// THE NULL LISTENER SLOTS BELOW SAFE. wl_seat gained name at 2, wl_pointer
// gained frame and the axis detail at 5, wl_keyboard gained repeat_info at 4.
// None of them is bound high enough to send any of that, so those slots stay
// NULL and libwayland never calls through them. Binding higher without filling
// them in is a crash, not a warning.
//
// repeat_info is version 4 and wl_seat is bound at 1, so that slot stays NULL.
static const struct wl_keyboard_listener keyboard_listener = {
	.keymap = keyboard_keymap,
	.enter = keyboard_enter,
	.leave = keyboard_leave,
	.key = keyboard_key,
	.modifiers = keyboard_modifiers,
	.repeat_info = NULL,
};

// THE POINTER'S POSITION AND BUTTONS NEED NOTHING EXTRA, BECAUSE THEY ARE WHAT A
// VERSION 1 wl_pointer SENDS. enter and motion carry surface-local coordinates as
// wl_fixed_t — 24.8 fixed point, so fractions of a unit are real and are kept —
// with the origin at the surface's top-left, +x right and +y down. A surface unit
// is a logical unit, not a pixel of ours, and is turned into one here — see
// scale in voe_platform_window. button carries an evdev code, BTN_LEFT and
// its neighbours from the same header the scancodes come from. While the pointer
// is locked, the compositor sends no motion at all — the position underneath
// simply stops — which is one of the reasons include/platform/input.h says a
// locked pointer is not over the window.
//
// The five events a version 1 wl_pointer sends. Four of them are the pointer's
// position and buttons and are recorded straight into src/input.h; axis is the
// wheel, turned into notches first — see pointer_axis.
//
// motion IS NOT WHERE MOUSE LOOK COMES FROM, AND THAT IS NOT A GAP. It is the
// pointer's position inside the surface, which stops at the edge of the window;
// a camera needs how far the mouse moved, which comes from
// relative_pointer_motion below. The two are the two different questions the
// public header describes, and they are filled from two different objects here.
//
// enter AND motion BOTH SET pointer_over, AND leave IS WHAT CLEARS IT. A pointer
// that entered is over the surface until the compositor says otherwise, and a
// motion is proof of the same; the compositor keeps sending motion to a surface
// that took a button press until the release, with coordinates outside the
// surface if that is where the pointer went, so a drag past the edge keeps its
// position and stays over — the public header promises exactly that.
//
// THE POSITION ON enter IS RECORDED TOO, AND NOT JUST ON motion. Otherwise a
// pointer that entered and stopped dead would read as over the window at
// wherever it was last seen before it left, which could be the other side.
static void pointer_at(voe_platform_window *window, wl_fixed_t x, wl_fixed_t y)
{
	window->input.pointer_x = (float)voe_platform_scale_position(
		wl_fixed_to_double(x), window->scale);
	window->input.pointer_y = (float)voe_platform_scale_position(
		wl_fixed_to_double(y), window->scale);
	window->input.pointer_over = true;
}

// The stored shape, by name, quoting the last enter's serial. Nothing while
// the pointer is elsewhere or the compositor has no cursor-shape-v1: the next
// enter applies whatever is stored then. While the lock hides the pointer it
// is hidden again instead, so an enter or a shape change mid-lock never shows it.
static void cursor_apply(voe_platform_window *window)
{
	uint32_t shape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_DEFAULT;

	VOE_BASE_DEBUG_ASSERT(window != NULL, "shaping a NULL window's pointer");
	VOE_BASE_DEBUG_ASSERT(window->input.cursor < VOE_PLATFORM_CURSOR_COUNT,
			      "the stored cursor is not a shape");

	if (window->cursor_shape == NULL || !window->input.pointer_over)
		return;
	if (window->cursor_hidden) {
		wl_pointer_set_cursor(window->pointer, window->pointer_serial,
				      NULL, 0, 0);
		return;
	}
	if (window->input.cursor == VOE_PLATFORM_CURSOR_LEFT_RIGHT)
		shape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_EW_RESIZE;
	else if (window->input.cursor == VOE_PLATFORM_CURSOR_UP_DOWN)
		shape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_NS_RESIZE;
	wp_cursor_shape_device_v1_set_shape(window->cursor_shape,
					    window->pointer_serial, shape);
}

static void pointer_enter(void *data, struct wl_pointer *pointer,
			  uint32_t serial, struct wl_surface *surface,
			  wl_fixed_t x, wl_fixed_t y)
{
	voe_platform_window *window = data;

	(void)pointer;
	(void)surface;

	window->pointer_serial = serial;
	pointer_at(window, x, y);
	cursor_apply(window);
}

// The pointer has gone to another surface, and any button still down when it
// went will be released there — see voe_platform_input_pointer_lost.
static void pointer_leave(void *data, struct wl_pointer *pointer,
			  uint32_t serial, struct wl_surface *surface)
{
	voe_platform_window *window = data;

	(void)pointer;
	(void)serial;
	(void)surface;

	voe_platform_input_pointer_lost(&window->input);
}

static void pointer_motion(void *data, struct wl_pointer *pointer,
			   uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
	voe_platform_window *window = data;

	(void)pointer;
	(void)time;

	pointer_at(window, x, y);
}

// An evdev button code to one of the three buttons this engine reads, or
// VOE_PLATFORM_BUTTON_COUNT for everything else — the same shape as key_of and
// the same header the codes come from.
static voe_platform_button button_of(uint32_t code)
{
	switch (code) {
	case BTN_LEFT:
		return VOE_PLATFORM_BUTTON_LEFT;
	case BTN_RIGHT:
		return VOE_PLATFORM_BUTTON_RIGHT;
	case BTN_MIDDLE:
		return VOE_PLATFORM_BUTTON_MIDDLE;
	default:
		return VOE_PLATFORM_BUTTON_COUNT;
	}
}

static void pointer_button(void *data, struct wl_pointer *pointer,
			   uint32_t serial, uint32_t time, uint32_t code,
			   uint32_t state)
{
	voe_platform_window *window = data;
	voe_platform_button button = button_of(code);

	(void)pointer;
	(void)serial;
	(void)time;

	if (button != VOE_PLATFORM_BUTTON_COUNT)
		window->input.buttons[button] =
			state == WL_POINTER_BUTTON_STATE_PRESSED;
}

// The length in surface units a compositor sends for one wheel detent, which
// turns wl_pointer.axis's length into the public header's notches.
//
// IT IS WESTON'S AND MUTTER'S NUMBER, AND NOT EVERY COMPOSITOR'S. Other
// compositors differ — wlroots sends 15 — so a notch there reads as more than
// one. Version 5's axis_discrete is the exact count of detents and is the
// answer; it is not bound, because wl_seat is bound at 1 like every global here,
// and raising it means filling frame and the axis detail as well.
#define AXIS_LENGTH_PER_NOTCH 10.0f

// The wheel, summed in notches until the next poll takes it. Wayland's positive
// already means content further down and further right, which is the public
// header's sign, so nothing is negated. A touchpad's continuous scroll arrives
// here too, as fractions of a notch.
static void pointer_axis(void *data, struct wl_pointer *pointer, uint32_t time,
			 uint32_t axis, wl_fixed_t value)
{
	voe_platform_window *window = data;
	float notches = (float)wl_fixed_to_double(value) / AXIS_LENGTH_PER_NOTCH;

	(void)pointer;
	(void)time;

	if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
		window->input.wheel_y += notches;
	else if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL)
		window->input.wheel_x += notches;
}

// frame and the axis detail are version 5, axis_value120 is 8 and
// axis_relative_direction is 9. wl_seat is bound at 1, so none of them arrives.
static const struct wl_pointer_listener pointer_listener = {
	.enter = pointer_enter,
	.leave = pointer_leave,
	.motion = pointer_motion,
	.button = pointer_button,
	.axis = pointer_axis,
	.frame = NULL,
	.axis_source = NULL,
	.axis_stop = NULL,
	.axis_discrete = NULL,
	.axis_value120 = NULL,
	.axis_relative_direction = NULL,
};

// MOUSE LOOK NEEDS TWO MORE PROTOCOLS AND NEITHER IS OPTIONAL FOR IT.
// wl_pointer.motion reports where the pointer is inside the surface, which stops
// at the edge; a camera needs how far the mouse moved, which does not.
// zwp_relative_pointer_v1 is that number, and zwp_pointer_constraints_v1 is what
// stops the cursor walking out of the window while it is being read. Both are
// vendored beside xdg-shell in protocol/, both are bound at version 1, and both
// are optional in exactly the way the decoration manager is: a compositor that
// offers neither leaves a window whose keyboard works and whose mouse look does
// not, and that is not an error anywhere in here.
//
// How far the mouse moved, summed until the next poll takes it.
//
// THE ACCELERATED PAIR IS THE ONE READ AND THE UNACCELERATED PAIR IS IGNORED, ON
// PURPOSE. dx and dy have the compositor's pointer acceleration applied, which
// is the speed the person configured for their desktop and expects everything to
// move at; dx_unaccel is the raw device delta, which is what a shooter wants so
// that aim does not depend on a desktop setting. This engine has no setting to
// choose between them and no caller that has asked, so it takes the one that
// matches the rest of the desktop. The day something wants raw, this is the line
// it changes and include/platform/input.h is where it says so.
static void relative_pointer_motion(void *data,
				    struct zwp_relative_pointer_v1 *relative,
				    uint32_t utime_hi, uint32_t utime_lo,
				    wl_fixed_t dx, wl_fixed_t dy,
				    wl_fixed_t dx_unaccel,
				    wl_fixed_t dy_unaccel)
{
	voe_platform_window *window = data;

	(void)relative;
	(void)utime_hi;
	(void)utime_lo;
	(void)dx_unaccel;
	(void)dy_unaccel;

	window->input.motion_x += (float)wl_fixed_to_double(dx);
	window->input.motion_y += (float)wl_fixed_to_double(dy);
}

static const struct zwp_relative_pointer_v1_listener relative_pointer_listener = {
	.relative_motion = relative_pointer_motion,
};

// The compositor's answer about the lock, and the reason
// voe_platform_input_pointer_locked reports a fact rather than a wish. Both
// events arrive unasked: a lock is deactivated when the window loses focus and
// activated again when it comes back, with nothing requested in between,
// because the object was created with the PERSISTENT lifetime.
static void locked_pointer_locked(void *data,
				  struct zwp_locked_pointer_v1 *locked)
{
	voe_platform_window *window = data;

	(void)locked;
	window->input.pointer_locked = true;

	// Hidden only when the shape can be named back; see the header.
	if (window->cursor_shape != NULL) {
		window->cursor_hidden = true;
		cursor_apply(window);
	}
}

// The shape back by name, once, if the lock hid the pointer.
static void cursor_unhide(voe_platform_window *window)
{
	if (!window->cursor_hidden)
		return;
	window->cursor_hidden = false;
	cursor_apply(window);
}

static void locked_pointer_unlocked(void *data,
				    struct zwp_locked_pointer_v1 *locked)
{
	voe_platform_window *window = data;

	(void)locked;
	window->input.pointer_locked = false;
	cursor_unhide(window);
}

static const struct zwp_locked_pointer_v1_listener locked_pointer_listener = {
	.locked = locked_pointer_locked,
	.unlocked = locked_pointer_unlocked,
};

// The lock object, made and unmade. Kept apart from the wish in
// voe_platform_window_lock_pointer, because two things ask for it: the caller,
// and a pointer appearing on the seat after the caller already asked.
//
// PERSISTENT AND NOT ONESHOT. A oneshot lock is destroyed by the compositor the
// first time it is deactivated — which is the first alt-tab — and would have to
// be asked for again on the way back with no event to say so. Persistent means
// the object survives losing focus and the compositor reactivates it, which is
// what locked and unlocked above are reporting.
//
// A NULL REGION IS THE WHOLE SURFACE, WHICH IS WHAT A LOCK WANTS. The region
// argument confines where the pointer may be for the lock to be active; the
// whole window is the only sensible answer for mouse look, and NULL says it
// without allocating a wl_region to say it with.
static void lock_start(voe_platform_window *window)
{
	if (window->locked_pointer != NULL)
		return;
	if (window->constraints == NULL || window->pointer == NULL)
		return;

	window->locked_pointer = zwp_pointer_constraints_v1_lock_pointer(
		window->constraints, window->surface, window->pointer, NULL,
		ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_PERSISTENT);
	if (window->locked_pointer == NULL)
		return;

	zwp_locked_pointer_v1_add_listener(window->locked_pointer,
					   &locked_pointer_listener, window);
}

// Destroying the object is what releases the pointer, and the compositor sends
// no unlocked event for a lock the client took away itself — so the flag is
// cleared here rather than waited for.
static void lock_stop(voe_platform_window *window)
{
	if (window->locked_pointer == NULL)
		return;

	zwp_locked_pointer_v1_destroy(window->locked_pointer);
	window->locked_pointer = NULL;
	window->input.pointer_locked = false;
	cursor_unhide(window);
}

// The pointer, and the relative-motion object that hangs off it. Both come and
// go with the seat's capabilities, which is why they are made here and not at
// startup: a mouse plugged in after the window opened arrives as a capabilities
// event and has to work.
//
// _destroy AND NOT _release, AND THAT IS THE SEAT'S BOUND VERSION TALKING.
// wl_pointer.release and wl_keyboard.release are version 3 requests and
// wl_seat.release is version 5; the seat here is bound at version 1, so the
// keyboard and the pointer it hands out are version 1 objects and sending any of
// those three is an unknown opcode — a protocol error that kills the client on
// the way out of a program that ran perfectly. _destroy is the version 1 way to
// let one go and it is what the whole of this file uses. This is the same
// binding-low rule window_wayland.c's header states for listener slots, from the other end: bind
// low and only version 1's requests exist.
//
// WHAT THAT COSTS IS ONE SERVER-SIDE OBJECT PER MOUSE UNPLUGGED, AND IT IS WORTH
// KNOWING RATHER THAN BEING SURPRISED BY. release tells the compositor the client
// is finished with the object; _destroy only forgets it on this side, so the
// compositor keeps its half until the connection closes. Nothing here can do
// better at version 1, because release is the request that does not exist yet.
// It is bounded by how many times a seat's capabilities change in one run — a
// mouse plugged in and out — and it is reclaimed when the program exits. A
// program that hotplugged for days is the one that would care, and what it wants
// is the seat bound at 3, which also means filling in wl_seat.name.
static void pointer_arrived(voe_platform_window *window)
{
	window->pointer = wl_seat_get_pointer(window->seat);
	if (window->pointer == NULL)
		return;
	wl_pointer_add_listener(window->pointer, &pointer_listener, window);

	if (window->relative_pointers != NULL) {
		window->relative_pointer =
			zwp_relative_pointer_manager_v1_get_relative_pointer(
				window->relative_pointers, window->pointer);
		if (window->relative_pointer != NULL)
			zwp_relative_pointer_v1_add_listener(
				window->relative_pointer,
				&relative_pointer_listener, window);
	}

	if (window->cursor_shapes != NULL)
		window->cursor_shape = wp_cursor_shape_manager_v1_get_pointer(
			window->cursor_shapes, window->pointer);

	// A lock asked for before there was a pointer to lock. Nothing else
	// would ever start it: the caller has already asked and got nothing.
	if (window->lock_wanted)
		lock_start(window);
}

static void pointer_left(voe_platform_window *window)
{
	// The lock names the pointer, so it goes first.
	lock_stop(window);

	// A button cannot still be down on a mouse that has been unplugged, and
	// a pointer that is not there is not over anything.
	voe_platform_input_pointer_lost(&window->input);

	if (window->relative_pointer != NULL) {
		zwp_relative_pointer_v1_destroy(window->relative_pointer);
		window->relative_pointer = NULL;
	}
	if (window->cursor_shape != NULL) {
		wp_cursor_shape_device_v1_destroy(window->cursor_shape);
		window->cursor_shape = NULL;
	}
	if (window->pointer != NULL) {
		wl_pointer_destroy(window->pointer);
		window->pointer = NULL;
	}
}

// What the seat has, which changes while the program runs. Every capability is
// created when it appears and destroyed when it goes, and a keyboard that goes
// away takes its held keys with it — a key cannot still be down on a keyboard
// that has been unplugged.
static void seat_capabilities(void *data, struct wl_seat *seat,
			      uint32_t capabilities)
{
	voe_platform_window *window = data;
	bool has_keyboard = (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) != 0;
	bool has_pointer = (capabilities & WL_SEAT_CAPABILITY_POINTER) != 0;

	(void)seat;

	if (has_keyboard && window->keyboard == NULL) {
		window->keyboard = wl_seat_get_keyboard(window->seat);
		if (window->keyboard != NULL)
			wl_keyboard_add_listener(window->keyboard,
						 &keyboard_listener, window);
	} else if (!has_keyboard && window->keyboard != NULL) {
		wl_keyboard_destroy(window->keyboard);
		window->keyboard = NULL;
		voe_platform_input_focus_lost(&window->input);
	}

	if (has_pointer && window->pointer == NULL)
		pointer_arrived(window);
	else if (!has_pointer && window->pointer != NULL)
		pointer_left(window);
}

// name is version 2 and the seat is bound at 1, so that slot stays NULL.
const struct wl_seat_listener voe_platform_seat_listener = {
	.capabilities = seat_capabilities,
	.name = NULL,
};

// Everything the seat made, let go of when the window closes: the lock before
// the pointer inside it, since a locked pointer names the wl_pointer it
// constrains and the relative-motion object names it too.
void voe_platform_seat_release(voe_platform_window *window)
{
	pointer_left(window);
	if (window->keyboard != NULL)
		wl_keyboard_destroy(window->keyboard);
	if (window->seat != NULL)
		wl_seat_destroy(window->seat);
}

// The three functions src/input.h declares, and the whole of what input.c knows
// about this file.
struct voe_platform_input *voe_platform_window_input(voe_platform_window *window)
{
	VOE_BASE_DEBUG_ASSERT(window != NULL, "asking a NULL window for input");

	return &window->input;
}

void voe_platform_window_lock_pointer(voe_platform_window *window, bool lock)
{
	VOE_BASE_DEBUG_ASSERT(window != NULL, "locking a NULL window's pointer");

	// The wish is recorded whether or not it can be acted on now, because a
	// pointer may arrive later and pointer_arrived reads it.
	window->lock_wanted = lock;

	if (lock)
		lock_start(window);
	else
		lock_stop(window);
}

void voe_platform_window_cursor(voe_platform_window *window,
				voe_platform_cursor cursor)
{
	VOE_BASE_DEBUG_ASSERT(window != NULL, "shaping a NULL window's pointer");
	VOE_BASE_DEBUG_ASSERT(cursor == window->input.cursor,
			      "input.c stores the shape before calling here");

	cursor_apply(window);
}
