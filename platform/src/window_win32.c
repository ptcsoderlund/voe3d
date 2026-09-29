// The Win32 window. See include/platform/window.h for the API; this file is
// everything below that line and is invisible to callers.
//
// Ordinary Win32: register a class, CreateWindowExW, and a PeekMessage pump that
// takes what is queued and returns. The Unicode entry points throughout —
// CreateWindowExW, not CreateWindowExA — so a title with a character outside the
// active code page survives, which means the UTF-8 title the API takes is
// converted on the way in.
//
// The window procedure needs to reach the voe_platform_window behind an HWND, so
// the pointer is put in the window's own user data. It is not there yet during
// the first few messages CreateWindowExW sends, so the procedure copes with not
// finding one and defers to DefWindowProcW.
//
// WM_CLOSE is answered by recording that the user asked and returning — the
// window is not destroyed there. Destroying it is voe_platform_window_destroy's
// job, and a window that tore itself down inside poll would leave the caller
// holding a pointer to nothing until it next thought to ask.
//
// Input is src/seat_win32.c: window_proc takes each keyboard and mouse message
// and hands it to a voe_platform_seat_ function there. Pads are
// src/gamepad_win32.c, opened with the window, routed their device changes,
// polled after the messages and closed after the window is gone. The struct
// these files write is src/window_win32.h.
#include "window_win32.h"

#include <base/assert.h>

#include <stdlib.h>

// One class for every window this process opens, registered on the first one.
#define WINDOW_CLASS L"voe_platform_window"

// The title is converted onto the stack rather than allocated, because _new does
// not take an arena and the engine has no global one to reach for. A title
// longer than this is a call-site mistake, not a runtime condition.
#define TITLE_MAX 256

static voe_platform_window *window_of(HWND hwnd)
{
	return (voe_platform_window *)(uintptr_t)GetWindowLongPtrW(hwnd,
								   GWLP_USERDATA);
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam,
				    LPARAM lparam)
{
	voe_platform_window *window = window_of(hwnd);

	// Messages sent by CreateWindowExW before the user data is set.
	if (window == NULL)
		return DefWindowProcW(hwnd, message, wparam, lparam);

	switch (message) {
	case WM_CLOSE:
		window->should_close = true;
		return 0;
	case WM_SIZE:
		window->width = LOWORD(lparam);
		window->height = HIWORD(lparam);
		// A window that has just changed size has a client rectangle
		// somewhere else, and a clip left where it was would hold the
		// cursor outside it.
		voe_platform_seat_apply_lock(window);
		return 0;
	case WM_KEYDOWN:
		voe_platform_seat_key_set(window, wparam, true);
		return 0;
	case WM_KEYUP:
		voe_platform_seat_key_set(window, wparam, false);
		return 0;
	case WM_CHAR:
		voe_platform_seat_handle_char(window, wparam);
		return 0;
	// Recorded and then handed on, so that Alt and Alt+F4 still do what
	// Windows means them to do.
	//
	// WM_SYSKEYDOWN IS HANDLED ALONGSIDE WM_KEYDOWN AND MUST NOT BE SWALLOWED.
	// Windows sends the SYS form for a key pressed while Alt is held, and for F10.
	// The engine wants to know the key moved either way, so both are recorded — but
	// the SYS pair then falls through to DefWindowProcW rather than returning zero,
	// because that is what opens the window menu on Alt and closes the window on
	// Alt+F4. A handler that returned zero here would take Alt+F4 away and it would
	// not be obvious why.
	case WM_SYSKEYDOWN:
		voe_platform_seat_key_set(window, wparam, true);
		return DefWindowProcW(hwnd, message, wparam, lparam);
	case WM_SYSKEYUP:
		voe_platform_seat_key_set(window, wparam, false);
		return DefWindowProcW(hwnd, message, wparam, lparam);
	case WM_INPUT:
		voe_platform_seat_raw_input(window, (HRAWINPUT)lparam);
		// Handed on as well: the documentation asks for it, and the
		// system does cleanup for the message there.
		return DefWindowProcW(hwnd, message, wparam, lparam);
	case WM_INPUT_DEVICE_CHANGE:
		voe_platform_gamepads_device_change(window, wparam, lparam);
		return 0;
	case WM_MOUSEMOVE:
		voe_platform_seat_pointer_at(window, lparam);
		return 0;
	case WM_MOUSELEAVE:
		window->tracking_leave = false;
		// With a button held the capture has the pointer and the drag
		// is still on; the release decides. See tracking_leave.
		if (!voe_platform_seat_any_button_down(window))
			voe_platform_input_pointer_lost(&window->input);
		return 0;
	case WM_LBUTTONDOWN:
		voe_platform_seat_button_set(window, VOE_PLATFORM_BUTTON_LEFT, true,
						     lparam);
		return 0;
	case WM_LBUTTONUP:
		voe_platform_seat_button_set(window, VOE_PLATFORM_BUTTON_LEFT, false,
						     lparam);
		return 0;
	case WM_RBUTTONDOWN:
		voe_platform_seat_button_set(window, VOE_PLATFORM_BUTTON_RIGHT, true,
						     lparam);
		return 0;
	case WM_RBUTTONUP:
		voe_platform_seat_button_set(window, VOE_PLATFORM_BUTTON_RIGHT, false,
						     lparam);
		return 0;
	case WM_MBUTTONDOWN:
		voe_platform_seat_button_set(window, VOE_PLATFORM_BUTTON_MIDDLE, true,
						     lparam);
		return 0;
	case WM_MBUTTONUP:
		voe_platform_seat_button_set(window, VOE_PLATFORM_BUTTON_MIDDLE, false,
						     lparam);
		return 0;
	// The wheel, in notches. Windows' positive vertical is the wheel turned
	// away, which shows content further up, so it is negated into the public
	// header's sign; its positive horizontal is already further right.
	// Written, not verified on Windows (ADR-0130).
	case WM_MOUSEWHEEL:
		window->input.wheel_y -=
			GET_WHEEL_DELTA_WPARAM(wparam) / (float)WHEEL_DELTA;
		return 0;
	case WM_MOUSEHWHEEL:
		window->input.wheel_x +=
			GET_WHEEL_DELTA_WPARAM(wparam) / (float)WHEEL_DELTA;
		return 0;
	case WM_CAPTURECHANGED:
		voe_platform_seat_capture_lost(window);
		return 0;
	// The client area's shape is ours; the frame's edges are Windows'.
	case WM_SETCURSOR:
		if (LOWORD(lparam) != HTCLIENT)
			return DefWindowProcW(hwnd, message, wparam, lparam);
		voe_platform_seat_cursor_show(window);
		return TRUE;
	// Passed on: DefWindowProcW is what gives an activated window the
	// keyboard focus.
	case WM_ACTIVATE:
		window->active = LOWORD(wparam) != WA_INACTIVE;
		return DefWindowProcW(hwnd, message, wparam, lparam);
	case WM_SETFOCUS:
		window->focused = true;
		voe_platform_seat_focus_gained(window);
		voe_platform_seat_apply_lock(window);
		return 0;
	case WM_KILLFOCUS:
		// The clip is not ours to hold while somebody else has focus,
		// and a hidden cursor over another window is worse still. Both
		// come back on the way in, without the caller asking again —
		// lock_wanted is still set and voe_platform_seat_apply_lock
		// reads it.
		window->focused = false;
		voe_platform_input_focus_lost(&window->input);
		// A high surrogate half-typed before focus went away has no low
		// half coming from whoever gets it next.
		window->pending_high_surrogate = 0;
		voe_platform_seat_apply_lock(window);
		return 0;
	default:
		return DefWindowProcW(hwnd, message, wparam, lparam);
	}
}

// THERE IS NO CS_DBLCLKS ON THE CLASS, AND THAT IS ON PURPOSE. With it Windows
// turns the second press of a double-click into a WM_xBUTTONDBLCLK and the
// down message never arrives; without it every press is a plain down, which is
// what a level-state API wants. What a double-click means is a caller's.
//
// Registering a class that is already registered fails with
// ERROR_CLASS_ALREADY_EXISTS, which is the second window opening and not a
// failure.
static bool class_ready(HINSTANCE instance)
{
	// IDC_ARROW is an ordinal, not a string, and the header's
	// MAKEINTRESOURCE gives it the ANSI pointer type unless UNICODE is
	// defined — which this file does not rely on, naming every W entry point
	// itself. The cast says what the value already is.
	WNDCLASSEXW description = {
		.cbSize = sizeof(description),
		.style = CS_HREDRAW | CS_VREDRAW,
		.lpfnWndProc = window_proc,
		.hInstance = instance,
		.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW),
		.lpszClassName = WINDOW_CLASS,
	};

	if (RegisterClassExW(&description) != 0)
		return true;
	return GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

// Fullscreen is a frameless popup covering the primary monitor's rectangle;
// the rectangle is the outer size and the client area both. False if the
// monitor cannot be asked, which is a failure to open like any other.
// Written, not verified on Windows (ADR-0130).
static bool fullscreen_rect(RECT *rect)
{
	MONITORINFO monitor = { .cbSize = sizeof(monitor) };
	HMONITOR primary = MonitorFromPoint((POINT){ 0, 0 },
					    MONITOR_DEFAULTTOPRIMARY);

	if (!GetMonitorInfoW(primary, &monitor))
		return false;
	*rect = monitor.rcMonitor;
	return true;
}

voe_platform_window *voe_platform_window_new(int width, int height,
					     bool fullscreen, const char *title)
{
	voe_platform_window *window;
	wchar_t wide_title[TITLE_MAX];
	RECT wanted = { 0, 0, width, height };
	DWORD style = fullscreen ? WS_POPUP : WS_OVERLAPPEDWINDOW;
	int x = CW_USEDEFAULT;
	int y = CW_USEDEFAULT;

	VOE_BASE_DEBUG_ASSERT(width > 0 && height > 0, "a window needs a size");
	VOE_BASE_DEBUG_ASSERT(title != NULL, "a window needs a title");

	VOE_BASE_ASSERT(MultiByteToWideChar(CP_UTF8, 0, title, -1, wide_title,
					    TITLE_MAX) != 0,
			"the window title is not UTF-8, or is longer than TITLE_MAX");

	window = calloc(1, sizeof(*window));
	VOE_BASE_ASSERT(window != NULL, "out of memory opening a window");

	window->instance = GetModuleHandleW(NULL);
	window->width = width;
	window->height = height;
	window->active = true;
	window->visible = true;

	if (!class_ready(window->instance)) {
		free(window);
		return NULL;
	}

	// width and height are the client area, and CreateWindowExW is given the
	// outer size, so the decorations have to be added on. Fullscreen has
	// none, and its place and size are the monitor's.
	if (fullscreen) {
		if (!fullscreen_rect(&wanted)) {
			free(window);
			return NULL;
		}
		x = wanted.left;
		y = wanted.top;
	} else {
		AdjustWindowRect(&wanted, style, FALSE);
	}

	window->hwnd = CreateWindowExW(0, WINDOW_CLASS, wide_title, style,
				       x, y,
				       wanted.right - wanted.left,
				       wanted.bottom - wanted.top,
				       NULL, NULL, window->instance, NULL);
	if (window->hwnd == NULL) {
		free(window);
		return NULL;
	}

	SetWindowLongPtrW(window->hwnd, GWLP_USERDATA, (LONG_PTR)(uintptr_t)window);

	// The mouse, as a raw device, so that WM_INPUT arrives with relative
	// counts. Registered before the window is shown so no movement is
	// missed, and not checked: a machine that refuses this has a keyboard
	// and no mouse look, which is the same outcome as a compositor without
	// the relative-pointer protocol and is not a failure to open a window.
	//
	// The usage page and usage are HID's numbering for "generic desktop,
	// mouse" and there are no constants for them in windows.h. RIDEV_INPUTSINK
	// is deliberately absent: it would deliver movement while another program
	// has focus, which is a keylogger's flag and not a game's.
	RAWINPUTDEVICE mouse = {
		.usUsagePage = 0x01,
		.usUsage = 0x02,
		.dwFlags = 0,
		.hwndTarget = window->hwnd,
	};

	RegisterRawInputDevices(&mouse, 1, sizeof(mouse));
	voe_platform_gamepads_open(window);

	ShowWindow(window->hwnd, SW_SHOW);

	// ShowWindow activates the window and WM_SETFOCUS has already been
	// through the procedure by here — but only if this window really got
	// focus, which a shell policy or another program grabbing it can
	// prevent. Asking rather than assuming costs one call and means the
	// flag is right either way.
	if (GetFocus() == window->hwnd && !window->focused) {
		window->focused = true;
		voe_platform_seat_focus_gained(window);
	}

	return window;
}

void voe_platform_window_destroy(voe_platform_window *window)
{
	VOE_BASE_DEBUG_ASSERT(window != NULL, "destroying a NULL window");

	// The clip is a machine-wide setting and the hidden cursor is a count,
	// so a window that went away while holding either would leave the
	// desktop worse than it found it. Both are given back before anything
	// is torn down.
	window->lock_wanted = false;
	window->focused = false;
	voe_platform_seat_apply_lock(window);

	if (window->hwnd != NULL)
		DestroyWindow(window->hwnd);
	// After the window, so no pad message arrives to a closed table.
	voe_platform_gamepads_close(window);
	free(window);
}

void voe_platform_window_poll(voe_platform_window *window)
{
	MSG message;

	VOE_BASE_DEBUG_ASSERT(window != NULL, "polling a NULL window");

	// Before anything is dispatched, so that the motion the messages below
	// bring is this frame's and not this frame's added to the last one's.
	voe_platform_input_begin_poll(&window->input);

	while (PeekMessageW(&message, NULL, 0, 0, PM_REMOVE) != 0) {
		TranslateMessage(&message);
		DispatchMessageW(&message);
	}
	voe_platform_gamepads_poll(window);

	window->visible = !IsIconic(window->hwnd);
}

// Wakes on anything queued for this thread, including what arrived before the
// call and was not yet read; nothing is removed, the next _poll does that.
void voe_platform_window_wait(voe_platform_window *window, double seconds)
{
	VOE_BASE_DEBUG_ASSERT(window != NULL, "waiting on a NULL window");

	MsgWaitForMultipleObjectsEx(0, NULL,
				    seconds < 0.0 ? INFINITE
						  : (DWORD)(seconds * 1000.0),
				    QS_ALLINPUT, MWMO_INPUTAVAILABLE);
}

bool voe_platform_window_focused(voe_platform_window *window)
{
	VOE_BASE_DEBUG_ASSERT(window != NULL, "asking a NULL window");

	return window->active;
}

bool voe_platform_window_visible(voe_platform_window *window)
{
	VOE_BASE_DEBUG_ASSERT(window != NULL, "asking a NULL window");

	return window->visible;
}

bool voe_platform_window_should_close(voe_platform_window *window)
{
	VOE_BASE_DEBUG_ASSERT(window != NULL, "asking a NULL window");

	return window->should_close;
}

void voe_platform_window_close_refuse(voe_platform_window *window)
{
	VOE_BASE_DEBUG_ASSERT(window != NULL, "asking a NULL window");

	window->should_close = false;
}

voe_platform_size voe_platform_window_size(voe_platform_window *window)
{
	VOE_BASE_DEBUG_ASSERT(window != NULL, "asking a NULL window");

	return (voe_platform_size){ window->width, window->height };
}

// Always. WS_OVERLAPPEDWINDOW is a frame, the window manager draws it, and there
// is no protocol to negotiate and nothing that can take it away. The Wayland
// side is where this question has more than one answer.
bool voe_platform_window_decorated(voe_platform_window *window)
{
	VOE_BASE_DEBUG_ASSERT(window != NULL, "asking a NULL window");

	return true;
}

voe_platform_native voe_platform_window_native(voe_platform_window *window)
{
	VOE_BASE_DEBUG_ASSERT(window != NULL, "asking a NULL window");

	return (voe_platform_native){ (uintptr_t)window->instance,
				      (uintptr_t)window->hwnd };
}
