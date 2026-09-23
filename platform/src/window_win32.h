// The Windows window's state, shared by the two files that make up the Win32
// backend and seen by nothing outside platform/src. include/platform/window.h
// hands callers an opaque voe_platform_window; this is what it is.
//
// The split: src/window_win32.c owns the class, the window procedure, open,
// close, poll, wait and the queries; src/seat_win32.c owns the keyboard, the
// mouse and the lock, and the three functions src/input.h asks of a window.
// What crosses is the declarations at the bottom: the handlers window_proc
// calls for each input message, two of which, _apply_lock and _focus_gained,
// _destroy and _new call as well. Nothing crosses the other way; the seat
// reaches the window only through the struct.
#pragma once

#include <platform/window.h>

#include "input.h"

#include <windows.h>

#include <stdint.h>

struct voe_platform_window {
	HWND hwnd;
	HINSTANCE instance;
	int width;
	int height;
	bool should_close;

	// The lock is wanted, and the window has focus. Both have to hold for
	// the cursor to actually be clipped, which is what makes this side
	// behave the way the Wayland side's persistent lock does: losing focus
	// gives the cursor back, and getting focus takes it again with nothing
	// asked for in between.
	bool lock_wanted;
	bool focused;

	// What voe_platform_window_focused and _visible answer: the window is
	// the active one (WM_ACTIVATE), and it is not minimised (IsIconic, read
	// at _poll). Keyboard focus above is the lock's; activation is the
	// person's. A window on another virtual desktop is not detected (0215).
	bool active;
	bool visible;

	// WM_MOUSELEAVE IS ASKED FOR, BECAUSE WINDOWS DOES NOT SEND IT UNASKED.
	// TrackMouseEvent arms one notification and then forgets, so it is re-armed on
	// the first movement inside after each one arrives. A leave with a button held
	// is ignored — the drag is still on and the capture is what decides — and the
	// release recomputes whether the pointer is still over the client area from
	// where it was when the last button came up.
	//
	// A WM_MOUSELEAVE has been asked for and has not yet arrived. It is a
	// one-shot, so this says whether to ask again on the next movement.
	bool tracking_leave;

	// The high half of a UTF-16 surrogate pair, held between two WM_CHAR
	// messages until the low half joins it — see
	// voe_platform_seat_handle_char. 0 means none is pending; 0 is never a
	// valid high surrogate, so it doubles safely as its own "nothing
	// waiting" value.
	uint16_t pending_high_surrogate;

	struct voe_platform_input input;
};

// A key message's virtual key, down or up, into the keys src/input.h keeps.
void voe_platform_seat_key_set(voe_platform_window *window, WPARAM virtual_key,
			       bool down);
// A WM_CHAR's UTF-16 code unit, into the typed text.
void voe_platform_seat_handle_char(voe_platform_window *window, WPARAM wparam);
// Focus arrived: every key re-read from what the OS says is held.
void voe_platform_seat_focus_gained(voe_platform_window *window);
// The cursor clipped and hidden, or given back, from lock_wanted and focused.
void voe_platform_seat_apply_lock(voe_platform_window *window);
// One WM_INPUT's relative mouse motion.
void voe_platform_seat_raw_input(voe_platform_window *window, HRAWINPUT handle);
// Where a mouse message says the pointer is.
void voe_platform_seat_pointer_at(voe_platform_window *window, LPARAM lparam);
bool voe_platform_seat_any_button_down(const voe_platform_window *window);
// A button message: position, state and the capture that goes with it.
void voe_platform_seat_button_set(voe_platform_window *window,
				  voe_platform_button button, bool down,
				  LPARAM lparam);
// WM_CAPTURECHANGED: every button up.
void voe_platform_seat_capture_lost(voe_platform_window *window);
// The stored shape as a system cursor.
void voe_platform_seat_cursor_show(const voe_platform_window *window);
