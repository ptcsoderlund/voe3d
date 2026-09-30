// The Windows window's state, shared by the two files that make up the Win32
// backend and seen by nothing outside platform/src. include/platform/window.h
// hands callers an opaque voe_platform_window; this is what it is.
//
// The split: src/window_win32.c owns the class, the window procedure, open,
// close, poll, wait and the queries; src/seat_win32.c owns the keyboard, the
// mouse and the lock, and the three functions src/input.h asks of a window;
// src/gamepad_win32.c owns the pads. What crosses is the declarations at the
// bottom: the handlers window_proc calls for each input message, two of which,
// _apply_lock and _focus_gained, _destroy and _new call as well, and the pads'
// open, poll and close. Nothing crosses the other way; the seat and the pads
// reach the window only through the struct.
#pragma once

#include <platform/library.h>
#include <platform/window.h>

#include "gamepad.h"
#include "input.h"

#include <windows.h>

// hidpi.h returns NTSTATUS, which windows.h leaves undefined in some SDKs; the
// same typedef twice is allowed. Likewise it takes USAGE, and some SDKs'
// hidpi.h expects hidusage.h, where USAGE is typedef'd, already included.
typedef LONG NTSTATUS;

#include <hidusage.h>
#include <hidpi.h>
#include <xinput.h>

#include <stdint.h>

// The most value caps a HID pad may declare; one declaring more is not taken.
#define VOE_PLATFORM_GAMEPAD_HID_VALUES 32

// The five calls loaded from xinput and hid.dll. NTSTATUS is a LONG.
typedef DWORD(WINAPI *voe_platform_xinput_get_state_fn)(DWORD user, XINPUT_STATE *state);
typedef LONG(WINAPI *voe_platform_hidp_get_caps_fn)(PHIDP_PREPARSED_DATA data, HIDP_CAPS *caps);
typedef LONG(WINAPI *voe_platform_hidp_get_value_caps_fn)(HIDP_REPORT_TYPE type,
							  HIDP_VALUE_CAPS *caps, USHORT *length,
							  PHIDP_PREPARSED_DATA data);
typedef LONG(WINAPI *voe_platform_hidp_get_usage_value_fn)(HIDP_REPORT_TYPE type, USAGE page,
							   USHORT collection, USAGE usage,
							   ULONG *value, PHIDP_PREPARSED_DATA data,
							   CHAR *report, ULONG length);
typedef LONG(WINAPI *voe_platform_hidp_get_usages_fn)(HIDP_REPORT_TYPE type, USAGE page,
						      USHORT collection, USAGE *usages,
						      ULONG *count, PHIDP_PREPARSED_DATA data,
						      CHAR *report, ULONG length);

// One XInput user: the slot it holds (-1 for none) and the tick count before
// which an unconnected user is not asked again (0 is at once).
struct voe_platform_gamepad_xinput_user {
	int slot;
	ULONGLONG ask_at;
};

// One HID pad: its raw input handle, its slot, its preparsed data (malloc'd,
// freed on removal) and its input value caps.
struct voe_platform_gamepad_hid_device {
	HANDLE handle;
	int slot;
	PHIDP_PREPARSED_DATA preparsed;
	HIDP_VALUE_CAPS values[VOE_PLATFORM_GAMEPAD_HID_VALUES];
	USHORT value_count;
};

// The loaded DLLs and their calls (NULL when missing), the four XInput users
// and the HID pads. Written only by src/gamepad_win32.c.
struct voe_platform_gamepad_devices {
	voe_platform_library *xinput;
	voe_platform_xinput_get_state_fn get_state;
	voe_platform_library *hid;
	voe_platform_hidp_get_caps_fn get_caps;
	voe_platform_hidp_get_value_caps_fn get_value_caps;
	voe_platform_hidp_get_usage_value_fn get_usage_value;
	voe_platform_hidp_get_usages_fn get_usages;
	struct voe_platform_gamepad_xinput_user users[XUSER_MAX_COUNT];
	struct voe_platform_gamepad_hid_device open[VOE_PLATFORM_GAMEPAD_SLOTS];
	int count;
};

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

	// The cursor's screen position when the clip started, put back when
	// it ends so the pointer shows where it was.
	POINT lock_taken_at;

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

	struct voe_platform_gamepad_devices gamepad_devices;
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

// Loads xinput and hid.dll and registers raw input for HID pads and joysticks;
// once, after the mouse's registration. A missing DLL is only its pads absent.
void voe_platform_gamepads_open(voe_platform_window *window);
// WM_INPUT_DEVICE_CHANGE: a HID pad taken on arrival, let go on removal.
void voe_platform_gamepads_device_change(voe_platform_window *window, WPARAM wparam,
					 LPARAM lparam);
// A WM_INPUT of type RIM_TYPEHID onto its pad's slot.
void voe_platform_gamepads_hid(voe_platform_window *window, const RAWINPUT *raw);
// Asks the XInput users due, attaching, feeding and detaching their slots.
void voe_platform_gamepads_poll(voe_platform_window *window);
// Frees every HID pad and unloads both DLLs; after the window is destroyed.
void voe_platform_gamepads_close(voe_platform_window *window);
