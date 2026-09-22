// The Wayland window's state, shared by the two files that make up the Linux
// backend and seen by nothing outside platform/src. include/platform/window.h
// hands callers an opaque voe_platform_window; this is what it is.
//
// The split: src/window_wayland.c owns the registry, the shell, decoration,
// fractional scale, open, close and poll; src/seat_wayland.c owns the seat and
// everything that arrives on it. Each writes only its own fields below, and
// what crosses between them is the two declarations at the bottom.
#pragma once

#include <platform/window.h>

#include "input.h"
#include "keymap.h"

#include <wayland-client.h>

#include <stdint.h>

struct voe_platform_window {
	struct wl_display *display;
	struct wl_registry *registry;
	struct wl_compositor *compositor;
	struct xdg_wm_base *wm_base;
	struct zxdg_decoration_manager_v1 *decorations;
	struct zwp_relative_pointer_manager_v1 *relative_pointers;
	struct zwp_pointer_constraints_v1 *constraints;
	struct wp_fractional_scale_manager_v1 *fractional_scales;
	struct wp_viewporter *viewporter;

	struct wl_surface *surface;
	struct xdg_surface *xdg_surface;
	struct xdg_toplevel *toplevel;
	struct zxdg_toplevel_decoration_v1 *decoration;
	struct wp_viewport *viewport;
	struct wp_fractional_scale_v1 *fractional_scale;

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

	// Logical sizes, as the configure sends them; _size scales them.
	int width;
	int height;
	int wanted_width;
	int wanted_height;

	// THE BUFFER IS DRAWN AT THE COMPOSITOR'S FRACTIONAL SCALE, AND ITS PIXELS ARE
	// THE ONES CALLERS SEE (ADR-0180). The configure hands over a logical size, and
	// on an output scaled by 1.25 a buffer that size is stretched by the compositor
	// with a smoothing filter — every edge goes soft. So wp_fractional_scale_v1's
	// preferred_scale is kept (in 120ths, 120 until it says otherwise), the logical
	// size is kept beside it, and voe_platform_window_size answers the logical size
	// times the scale through src/scale.h; the swapchain builds its buffer at that
	// size, and wp_viewport's destination, set to the logical size whenever either
	// number changes, tells the compositor to show it at the logical size without
	// resampling. Pointer positions go through the same scale on enter and motion,
	// so they and voe_platform_size still measure one space; relative motion and the
	// wheel are not positions and are not scaled. wl_surface_set_buffer_scale is
	// never called: an integer cannot say 1.25. Both protocols are optional and
	// bound at version 1 — with either one missing the scale stays 120 and every
	// number here is the logical one, exactly as before.
	//
	// The preferred scale in 120ths, and whether the viewport's destination
	// is behind the logical size or the scale.
	uint32_t scale;
	bool viewport_stale;
	uint32_t decoration_mode;
	bool configured;
	bool should_close;

	// The xdg_wm_base version bound, and the toplevel's activated and
	// suspended states: the wanted_ pair is what the last configure said,
	// the other what _poll folded in and _focused and _visible answer.
	uint32_t shell_version;
	bool focused;
	bool suspended;
	bool wanted_focused;
	bool wanted_suspended;

	// A KEYMAP THE READER REFUSES IS REPORTED ONCE, NOT ON EVERY KEY. The keymap
	// event fires once at startup and again only when the layout changes, so there
	// is nothing to spam here regardless — but the flag exists to say so on
	// purpose rather than by accident, and a window whose keymap could not be read
	// still opens, still closes and still reads keys as places; typing a character
	// is the only thing it has lost.
	//
	// What the compositor's keymap says each evdev code types, and whether
	// a broken one has already been reported — see keyboard_keymap.
	voe_platform_keymap keymap;
	bool keymap_reported;

	// AltGr IS ONE OF THOSE PLACES TOO (ADR-0169), NOT A MODIFIER READ OFF THE
	// COMPOSITOR. The keymap reader flags whichever evdev code its own text names
	// as the level-three shift, and this file holds that flag exactly the way it
	// already holds Shift — set and cleared by key in keyboard_key, restored on
	// refocus from the codes keyboard_enter is handed, and dropped in
	// keyboard_leave with everything else focus takes. A press then picks one of
	// the keymap's four levels by which of Shift and AltGr are held, the same
	// question key already answers for every other key.
	//
	// Whether the key the keymap names as level-three shift (ADR-0169) is
	// currently held. Kept beside the keymap rather than in the shared
	// input state below because AltGr is not one of the twelve keys
	// include/platform/input.h exposes — see key_set.
	bool altgr_held;

	struct voe_platform_input input;
};

// The seat's listener, attached by _new to the seat the registry bound.
extern const struct wl_seat_listener voe_platform_seat_listener;

// Lets go of the pointer, its lock, the keyboard and the seat; close_down's
// first step.
void voe_platform_seat_release(voe_platform_window *window);
