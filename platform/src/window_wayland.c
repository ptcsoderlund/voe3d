// The Wayland window. See include/platform/window.h for the API; this file is
// everything below that line and is invisible to callers.
//
// The shape is the ordinary xdg-shell one: connect, bind the globals off the
// registry, make a wl_surface, wrap it in an xdg_surface and an xdg_toplevel,
// commit, take the first configure, and from then on ack every configure and
// answer every ping. A missed ping is how a client gets killed for not
// responding, so xdg_wm_base's listener is not optional even though nothing here
// wants the event.
//
// Every global is bound at version 1. Only version 1 of each is used, and a
// listener slot for an event the bound version never sends stays NULL — which
// matters, because libwayland calls straight through a listener's function
// pointer and a NULL one is a crash rather than a no-op. Binding low is what
// keeps the slots this file leaves NULL unreachable.
//
// DECORATIONS ARE ASKED FOR, NOT ASSUMED. xdg-decoration is how a client says it
// would rather the compositor drew the frame, and the compositor answers with
// the mode it actually chose — which may not be the one asked for. Three
// outcomes, and all three are normal: the manager is absent from the registry
// (GNOME's Mutter does not offer the protocol), the manager is there and answers
// client-side, or it answers server-side and a real titlebar appears.
//
// Only the last of those gets a frame. The other two leave the window bare, and
// bare is not an error: nothing here warns, nothing here fails, and nothing here
// draws a titlebar of its own. Drawing one needs pointer input and somewhere to
// draw, neither of which exists yet. Move and resize a bare window with the
// compositor's own shortcuts.
//
// NOTHING HERE PUTS A PIXEL ON THE SCREEN, AND THAT IS WHY THE WINDOW IS NOT
// VISIBLE UNTIL SOMETHING ELSE DOES. A Wayland surface with no buffer ever
// attached is not mapped at all: it exists, it has a size, it answers configure,
// and the compositor shows nothing. Card 004 filled that gap with a flat
// shared-memory buffer; card 007 deleted it, because render now creates a Vulkan
// swapchain against this surface and the swapchain's first present is what maps
// the window.
//
// So a caller that opens a window and never draws sees no window. That is not a
// fault here and there is nothing to fix in this file — it is what a Wayland
// surface is. It is written down because the symptom is an invisible window with
// no error anywhere, which is a long afternoon for whoever meets it without
// having read this.
//
// A compositor that goes away mid-run sets should_close, because there is no
// other way out: _poll cannot report anything and the caller is in a loop. That
// is a papering-over, not a design, and it is written up on card 004.
//
// INPUT IS HERE AND NOT IN A FILE OF ITS OWN, BECAUSE IT ARRIVES DOWN THIS
// SOCKET. A keyboard and a pointer are wl_seat's, wl_seat comes off the same
// registry as the compositor, and every event reaches us through the same
// wl_display_dispatch_pending that a resize does. A separate input file would
// have to be handed the display, the registry and the surface — which is this
// struct, with a second name. So the listeners are here and what they fill is
// src/input.h, which is the half that is not Wayland's and is shared with
// Windows.
//
// THE SEAT IS BOUND AT VERSION 1 LIKE EVERYTHING ELSE, AND THAT IS WHAT MAKES
// THE NULL LISTENER SLOTS BELOW SAFE. wl_seat gained name at 2, wl_pointer
// gained frame and the axis detail at 5, wl_keyboard gained repeat_info at 4.
// None of them is bound high enough to send any of that, so those slots stay
// NULL and libwayland never calls through them. Binding higher without filling
// them in is a crash, not a warning.
//
// A KEY IS A PLACE AND NOT A LETTER, AND THAT IS WHY THERE IS NO xkbcommon HERE.
// wl_keyboard.key carries the evdev scancode of the key that moved — KEY_W is
// the key where W sits on a US keyboard whatever the layout says it types — and
// <linux/input-event-codes.h> is where those numbers are written down. Turning a
// scancode into a character needs the keymap, which needs xkbcommon, which is a
// dependency this engine has not asked for and does not need: nothing here reads
// text. The keymap event is therefore ignored, apart from closing the file
// descriptor it comes with, which is not optional — one leaked descriptor per
// keymap change is still a leak.
//
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
// NOTHING HERE TOUCHES THE CURSOR IMAGE, AND THAT IS A LIMIT RATHER THAN AN
// OVERSIGHT. A locked pointer is frozen by the compositor and stays visible,
// because hiding it means calling wl_pointer_set_cursor with a surface, and a
// surface needs a buffer with a cursor drawn in it — an image this engine does
// not have. Passing NULL hides it with no way back, since restoring the arrow
// would mean drawing an arrow. A frozen cursor is worse than a hidden one and
// better than one that vanishes for good; include/platform/input.h says so, and
// says which card fixes it.
//
// _GNU_SOURCE is what makes <poll.h> declare poll() under -std=c23, which the
// engine builds with. It is a feature-test macro and not a use of any GNU
// extension.
#define _GNU_SOURCE

#include <platform/window.h>

#include "input.h"

#include <base/assert.h>

#include <wayland-client.h>

#include "pointer-constraints-unstable-v1-client-protocol.h"
#include "relative-pointer-unstable-v1-client-protocol.h"
#include "xdg-decoration-unstable-v1-client-protocol.h"
#include "xdg-shell-client-protocol.h"

#include <linux/input-event-codes.h>

#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct voe_platform_window {
	struct wl_display *display;
	struct wl_registry *registry;
	struct wl_compositor *compositor;
	struct xdg_wm_base *wm_base;
	struct zxdg_decoration_manager_v1 *decorations;
	struct zwp_relative_pointer_manager_v1 *relative_pointers;
	struct zwp_pointer_constraints_v1 *constraints;

	struct wl_surface *surface;
	struct xdg_surface *xdg_surface;
	struct xdg_toplevel *toplevel;
	struct zxdg_toplevel_decoration_v1 *decoration;

	// The seat and what it currently has. A seat's capabilities change while
	// the program runs — a mouse is unplugged, a tablet is picked up — so
	// each of the three below is created and destroyed by the capabilities
	// event and every one of them may be NULL at any moment.
	struct wl_seat *seat;
	struct wl_keyboard *keyboard;
	struct wl_pointer *pointer;
	struct zwp_relative_pointer_v1 *relative_pointer;

	// The lock, and what was asked for. Both are needed: the object is what
	// exists, and the wish is what to do about it when a pointer appears
	// after the wish was made — plugging in a mouse while flying.
	struct zwp_locked_pointer_v1 *locked_pointer;
	bool lock_wanted;

	int width;
	int height;
	int wanted_width;
	int wanted_height;
	uint32_t decoration_mode;
	bool configured;
	bool should_close;

	struct voe_platform_input input;
};

// ------------------------------------------------------------------ listeners

static void wm_base_ping(void *data, struct xdg_wm_base *wm_base, uint32_t serial)
{
	(void)data;
	xdg_wm_base_pong(wm_base, serial);
}

static const struct xdg_wm_base_listener wm_base_listener = {
	.ping = wm_base_ping,
};

static void registry_global(void *data, struct wl_registry *registry,
			    uint32_t name, const char *interface, uint32_t version)
{
	voe_platform_window *window = data;

	(void)version;

	if (strcmp(interface, wl_compositor_interface.name) == 0)
		window->compositor = wl_registry_bind(registry, name,
						      &wl_compositor_interface, 1);
	else if (strcmp(interface, xdg_wm_base_interface.name) == 0)
		window->wm_base = wl_registry_bind(registry, name,
						   &xdg_wm_base_interface, 1);
	// Optional, and absent on a compositor that draws no frames. Everything
	// downstream checks for NULL rather than assuming it arrived.
	else if (strcmp(interface, zxdg_decoration_manager_v1_interface.name) == 0)
		window->decorations = wl_registry_bind(registry, name,
						       &zxdg_decoration_manager_v1_interface,
						       1);
	// The keyboard and the mouse. A seat is not optional in practice — every
	// compositor with a user attached has one — but it is treated as
	// optional anyway, because a window whose input never arrives should
	// still open and still close.
	else if (strcmp(interface, wl_seat_interface.name) == 0)
		window->seat = wl_registry_bind(registry, name,
						&wl_seat_interface, 1);
	// These two are genuinely optional and mouse look is what is lost
	// without them. Nothing warns: see the header.
	else if (strcmp(interface, zwp_relative_pointer_manager_v1_interface.name) == 0)
		window->relative_pointers = wl_registry_bind(registry, name,
			&zwp_relative_pointer_manager_v1_interface, 1);
	else if (strcmp(interface, zwp_pointer_constraints_v1_interface.name) == 0)
		window->constraints = wl_registry_bind(registry, name,
			&zwp_pointer_constraints_v1_interface, 1);
}

// Nothing here holds a global long enough to care that one went away.
static void registry_global_remove(void *data, struct wl_registry *registry,
				   uint32_t name)
{
	(void)data;
	(void)registry;
	(void)name;
}

static const struct wl_registry_listener registry_listener = {
	.global = registry_global,
	.global_remove = registry_global_remove,
};

// The compositor's half of the resize handshake: it proposes, this acks, and
// whatever draws into the surface next is expected to be the size that was
// acked. Nothing is committed here, because nothing here has a buffer to commit
// — the swapchain does, and it finds out about the new size by asking _size.
static void surface_configure(void *data, struct xdg_surface *xdg_surface,
			      uint32_t serial)
{
	voe_platform_window *window = data;

	xdg_surface_ack_configure(xdg_surface, serial);
	window->configured = true;
}

static const struct xdg_surface_listener surface_listener = {
	.configure = surface_configure,
};

// A width or height of zero means "you choose", which is what arrives when the
// window first opens, so the size asked for at _new is kept in that case.
static void toplevel_configure(void *data, struct xdg_toplevel *toplevel,
			       int32_t width, int32_t height,
			       struct wl_array *states)
{
	voe_platform_window *window = data;

	(void)toplevel;
	(void)states;

	if (width > 0 && height > 0) {
		window->wanted_width = width;
		window->wanted_height = height;
	}
}

static void toplevel_close(void *data, struct xdg_toplevel *toplevel)
{
	voe_platform_window *window = data;

	(void)toplevel;
	window->should_close = true;
}

// configure_bounds and wm_capabilities are version 4 and 5 events. xdg_wm_base is
// bound at version 1, so they never arrive and these slots stay NULL.
static const struct xdg_toplevel_listener toplevel_listener = {
	.configure = toplevel_configure,
	.close = toplevel_close,
	.configure_bounds = NULL,
	.wm_capabilities = NULL,
};

// The compositor's answer. It arrives when the decoration object is created and
// again on every change of state — measured on KWin: one at startup, one when
// "No Borders" takes the frame away, one when it puts it back. All three said
// SERVER_SIDE. The mode names who is *responsible* for the frame, and KWin is
// responsible in both states; it has merely chosen to draw nothing in one of
// them. So the event fires and the value never moves. Nothing here behaves
// differently on it: a client that changed its mind about how to draw depending
// on who drew the frame would be the bug this protocol exists to avoid. It is
// recorded so voe_platform_window_decorated can report it.
static void decoration_configure(void *data,
				 struct zxdg_toplevel_decoration_v1 *decoration,
				 uint32_t mode)
{
	voe_platform_window *window = data;

	(void)decoration;
	window->decoration_mode = mode;
}

static const struct zxdg_toplevel_decoration_v1_listener decoration_listener = {
	.configure = decoration_configure,
};

// --------------------------------------------------------------------- input

// An evdev scancode to one of the nine keys this engine reads, or
// VOE_PLATFORM_KEY_COUNT for the hundred-odd it does not.
//
// A SWITCH AND NOT A TABLE INDEXED BY SCANCODE, BECAUSE THE SCANCODES ARE SPARSE
// AND THE ENGINE'S SET IS TINY. Nine cases against an array with two hundred and
// fifty entries in it, of which nine are used; the switch is also the form the
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
	default:
		return VOE_PLATFORM_KEY_COUNT;
	}
}

static void key_set(voe_platform_window *window, uint32_t scancode, bool down)
{
	voe_platform_key key = key_of(scancode);

	if (key != VOE_PLATFORM_KEY_COUNT)
		window->input.keys[key] = down;
}

// The keymap, which is ignored, and the file descriptor it arrives on, which is
// not. The compositor maps a keymap into a file and hands over the descriptor;
// reading it needs xkbcommon and this engine reads scancodes instead, so the
// only thing owed here is the close. It fires again whenever the layout changes,
// which is why leaking it would be a leak that grows.
static void keyboard_keymap(void *data, struct wl_keyboard *keyboard,
			    uint32_t format, int32_t fd, uint32_t size)
{
	(void)data;
	(void)keyboard;
	(void)format;
	(void)size;

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

	voe_platform_input_focus_lost(&window->input);
}

static void keyboard_key(void *data, struct wl_keyboard *keyboard,
			 uint32_t serial, uint32_t time, uint32_t key,
			 uint32_t state)
{
	voe_platform_window *window = data;

	(void)keyboard;
	(void)serial;
	(void)time;

	key_set(window, key, state == WL_KEYBOARD_KEY_STATE_PRESSED);
}

// Which modifiers the compositor thinks are latched, and it is deliberately not
// where Shift and Control come from. This event reports the state of the
// *keymap's* modifier groups, which is a question about what a keystroke would
// type; the engine wants whether a physical key is held, and that arrives
// through key like every other key. Reading both would be two answers to one
// question and they disagree — Caps Lock latches a modifier with no key held.
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

// repeat_info is version 4 and wl_seat is bound at 1, so that slot stays NULL.
static const struct wl_keyboard_listener keyboard_listener = {
	.keymap = keyboard_keymap,
	.enter = keyboard_enter,
	.leave = keyboard_leave,
	.key = keyboard_key,
	.modifiers = keyboard_modifiers,
	.repeat_info = NULL,
};

// The five events a version 1 wl_pointer sends, and four of them do nothing.
//
// motion IS NOT WHERE MOUSE LOOK COMES FROM AND IT IS NOT A GAP THAT IT IS
// EMPTY. This is the pointer's position inside the surface, which stops at the
// edge of the window; a camera needs how far the mouse moved, which comes from
// relative_pointer_motion below. Something that wants to know where the cursor
// is — a cursor to draw, a thing to click — is what fills this in, and rule 10
// says that is the card that has one.
//
// enter, leave, button and axis are the same story: nothing reads them, so
// nothing is recorded. They are written out rather than left NULL because
// libwayland calls straight through a listener slot and version 1 sends all
// five.
static void pointer_enter(void *data, struct wl_pointer *pointer,
			  uint32_t serial, struct wl_surface *surface,
			  wl_fixed_t x, wl_fixed_t y)
{
	(void)data;
	(void)pointer;
	(void)serial;
	(void)surface;
	(void)x;
	(void)y;
}

static void pointer_leave(void *data, struct wl_pointer *pointer,
			  uint32_t serial, struct wl_surface *surface)
{
	(void)data;
	(void)pointer;
	(void)serial;
	(void)surface;
}

static void pointer_motion(void *data, struct wl_pointer *pointer,
			   uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
	(void)data;
	(void)pointer;
	(void)time;
	(void)x;
	(void)y;
}

static void pointer_button(void *data, struct wl_pointer *pointer,
			   uint32_t serial, uint32_t time, uint32_t button,
			   uint32_t state)
{
	(void)data;
	(void)pointer;
	(void)serial;
	(void)time;
	(void)button;
	(void)state;
}

static void pointer_axis(void *data, struct wl_pointer *pointer, uint32_t time,
			 uint32_t axis, wl_fixed_t value)
{
	(void)data;
	(void)pointer;
	(void)time;
	(void)axis;
	(void)value;
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
}

static void locked_pointer_unlocked(void *data,
				    struct zwp_locked_pointer_v1 *locked)
{
	voe_platform_window *window = data;

	(void)locked;
	window->input.pointer_locked = false;
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
// binding-low rule the header states for listener slots, from the other end: bind
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

	// A lock asked for before there was a pointer to lock. Nothing else
	// would ever start it: the caller has already asked and got nothing.
	if (window->lock_wanted)
		lock_start(window);
}

static void pointer_left(voe_platform_window *window)
{
	// The lock names the pointer, so it goes first.
	lock_stop(window);

	if (window->relative_pointer != NULL) {
		zwp_relative_pointer_v1_destroy(window->relative_pointer);
		window->relative_pointer = NULL;
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
static const struct wl_seat_listener seat_listener = {
	.capabilities = seat_capabilities,
	.name = NULL,
};

// The two functions src/input.h declares, and the whole of what input.c knows
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

// -------------------------------------------------------------- open and close

static void close_down(voe_platform_window *window)
{
	// Input first, and the lock before the pointer inside it: a locked
	// pointer names the wl_pointer it constrains, and the relative-motion
	// object names it too.
	pointer_left(window);
	if (window->keyboard != NULL)
		wl_keyboard_destroy(window->keyboard);
	if (window->seat != NULL)
		wl_seat_destroy(window->seat);

	if (window->decoration != NULL)
		zxdg_toplevel_decoration_v1_destroy(window->decoration);
	if (window->toplevel != NULL)
		xdg_toplevel_destroy(window->toplevel);
	if (window->xdg_surface != NULL)
		xdg_surface_destroy(window->xdg_surface);
	if (window->surface != NULL)
		wl_surface_destroy(window->surface);

	if (window->constraints != NULL)
		zwp_pointer_constraints_v1_destroy(window->constraints);
	if (window->relative_pointers != NULL)
		zwp_relative_pointer_manager_v1_destroy(window->relative_pointers);
	if (window->decorations != NULL)
		zxdg_decoration_manager_v1_destroy(window->decorations);
	if (window->wm_base != NULL)
		xdg_wm_base_destroy(window->wm_base);
	if (window->compositor != NULL)
		wl_compositor_destroy(window->compositor);
	if (window->registry != NULL)
		wl_registry_destroy(window->registry);
	if (window->display != NULL)
		wl_display_disconnect(window->display);

	free(window);
}

// Every failure below lands here, so a half-open window is torn down by the same
// code that tears down a whole one.
static voe_platform_window *open_failed(voe_platform_window *window)
{
	close_down(window);
	return NULL;
}

voe_platform_window *voe_platform_window_new(int width, int height,
					     const char *title)
{
	voe_platform_window *window;

	VOE_BASE_DEBUG_ASSERT(width > 0 && height > 0, "a window needs a size");
	VOE_BASE_DEBUG_ASSERT(title != NULL, "a window needs a title");

	window = calloc(1, sizeof(*window));
	VOE_BASE_ASSERT(window != NULL, "out of memory opening a window");

	window->width = width;
	window->height = height;
	window->wanted_width = width;
	window->wanted_height = height;

	// No compositor, no WAYLAND_DISPLAY, no session: recoverable, and the
	// reason this function returns a pointer that can be NULL.
	window->display = wl_display_connect(NULL);
	if (window->display == NULL)
		return open_failed(window);

	window->registry = wl_display_get_registry(window->display);
	wl_registry_add_listener(window->registry, &registry_listener, window);
	if (wl_display_roundtrip(window->display) < 0)
		return open_failed(window);

	// A compositor that offers no xdg-shell cannot give us a window at all.
	if (window->compositor == NULL || window->wm_base == NULL)
		return open_failed(window);
	xdg_wm_base_add_listener(window->wm_base, &wm_base_listener, window);

	// A seat that never arrived is a window with no keyboard and no mouse,
	// which opens and closes and draws exactly as it would have. Nothing
	// here fails on it and nothing warns — the same standing the decoration
	// manager has. The keyboard and the pointer are not created here: the
	// capabilities event is what says whether there are any, and it arrives
	// on the roundtrip at the end of this function.
	if (window->seat != NULL)
		wl_seat_add_listener(window->seat, &seat_listener, window);

	window->surface = wl_compositor_create_surface(window->compositor);
	if (window->surface == NULL)
		return open_failed(window);

	window->xdg_surface = xdg_wm_base_get_xdg_surface(window->wm_base,
							  window->surface);
	if (window->xdg_surface == NULL)
		return open_failed(window);
	xdg_surface_add_listener(window->xdg_surface, &surface_listener, window);

	window->toplevel = xdg_surface_get_toplevel(window->xdg_surface);
	if (window->toplevel == NULL)
		return open_failed(window);
	xdg_toplevel_add_listener(window->toplevel, &toplevel_listener, window);
	xdg_toplevel_set_title(window->toplevel, title);

	// Ask for a server-drawn frame. Asking is all a client can do; the reply
	// comes back through decoration_configure and may say client-side. A
	// compositor that offers no manager at all is the same outcome by a
	// shorter route, and neither is worth a word at runtime.
	if (window->decorations != NULL) {
		window->decoration = zxdg_decoration_manager_v1_get_toplevel_decoration(
			window->decorations, window->toplevel);
		if (window->decoration != NULL) {
			zxdg_toplevel_decoration_v1_add_listener(window->decoration,
								 &decoration_listener,
								 window);
			zxdg_toplevel_decoration_v1_set_mode(window->decoration,
				ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
		}
	}

	// The empty commit that asks for the first configure, then the wait for
	// it. Attaching a buffer before that configure is a protocol error, so
	// whoever draws into this surface may not start until it has arrived —
	// which is what makes waiting for it part of opening the window rather
	// than something the first frame could do. One roundtrip is normally
	// enough; the loop is for a compositor that takes its time, and giving up
	// is a failure to open like any other.
	wl_surface_commit(window->surface);
	for (int attempt = 0; attempt < 4 && !window->configured; attempt++) {
		if (wl_display_roundtrip(window->display) < 0)
			return open_failed(window);
	}
	if (!window->configured)
		return open_failed(window);

	window->width = window->wanted_width;
	window->height = window->wanted_height;

	return window;
}

void voe_platform_window_destroy(voe_platform_window *window)
{
	VOE_BASE_DEBUG_ASSERT(window != NULL, "destroying a NULL window");

	close_down(window);
}

// ------------------------------------------------------------------------ poll

// Read whatever is already on the socket and nothing more. The prepare/cancel
// dance is libwayland's, and it is the only way to look without blocking: the
// read has to be announced before the socket is polled, or an event arriving in
// between is lost.
static void pump(voe_platform_window *window)
{
	struct pollfd waiting = {
		.fd = wl_display_get_fd(window->display),
		.events = POLLIN,
	};

	while (wl_display_prepare_read(window->display) != 0)
		wl_display_dispatch_pending(window->display);

	wl_display_flush(window->display);

	if (poll(&waiting, 1, 0) > 0 && (waiting.revents & POLLIN))
		wl_display_read_events(window->display);
	else
		wl_display_cancel_read(window->display);

	wl_display_dispatch_pending(window->display);
}

void voe_platform_window_poll(voe_platform_window *window)
{
	VOE_BASE_DEBUG_ASSERT(window != NULL, "polling a NULL window");

	// Before anything is dispatched, so that the motion the events below
	// bring is this frame's and not this frame's added to the last one's.
	voe_platform_input_begin_poll(&window->input);

	pump(window);

	// A connection that has died cannot be reported: _poll returns nothing,
	// and the NULL from _new only covers a window that never opened. Rather
	// than spin forever on a socket that will never speak again, this is
	// folded into should_close — which is a paper over a hole, and the hole
	// is that the engine has no way to say "this failed after it started".
	if (wl_display_get_error(window->display) != 0)
		window->should_close = true;

	// A configure that proposed a new size is folded into the size callers
	// ask for. Doing it here rather than in the handler keeps one path for a
	// resize whenever it lands, and whoever is drawing finds out the same way
	// everyone else does: by asking _size and seeing a different number.
	if (window->wanted_width != window->width ||
	    window->wanted_height != window->height) {
		window->width = window->wanted_width;
		window->height = window->wanted_height;
	}
}

bool voe_platform_window_should_close(voe_platform_window *window)
{
	VOE_BASE_DEBUG_ASSERT(window != NULL, "asking a NULL window");

	return window->should_close;
}

voe_platform_size voe_platform_window_size(voe_platform_window *window)
{
	VOE_BASE_DEBUG_ASSERT(window != NULL, "asking a NULL window");

	return (voe_platform_size){ window->width, window->height };
}

bool voe_platform_window_decorated(voe_platform_window *window)
{
	VOE_BASE_DEBUG_ASSERT(window != NULL, "asking a NULL window");

	// Zero until the compositor answers, and zero forever if there was no
	// manager to ask — neither is server-side, so neither is decorated.
	return window->decoration_mode ==
	       ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE;
}

voe_platform_native voe_platform_window_native(voe_platform_window *window)
{
	VOE_BASE_DEBUG_ASSERT(window != NULL, "asking a NULL window");

	return (voe_platform_native){ (uintptr_t)window->display,
				      (uintptr_t)window->surface };
}
