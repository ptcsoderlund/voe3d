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
// INPUT IS HERE AND NOT IN A FILE OF ITS OWN, FOR THE SAME REASON THE WAYLAND
// SIDE'S IS. Every keystroke and every mouse movement arrives as a message to
// the window procedure below, dispatched by the same PeekMessageW loop a resize
// comes through. What the handlers fill is src/input.h, which is the half that
// is not Windows' and is shared with Wayland.
//
// A KEY IS STILL A PLACE AND NOT A LETTER, AND WM_CHAR ANSWERS A DIFFERENT
// QUESTION FROM WM_KEYDOWN'S — BUT THIS FILE NOW READS BOTH. WM_KEYDOWN carries
// a virtual key, a position on the keyboard, and that is still all key_of and
// key_set ever look at. WM_CHAR carries what the active layout says that
// keystroke types, already resolved by TranslateMessage — no keymap of our own
// to read here, unlike Wayland — and it is what fills
// voe_platform_input_text (ADR-0161). TranslateMessage was already called in
// the pump for the message loop's own sake; the WM_CHAR it synthesises is no
// longer left to fall through to DefWindowProcW.
//
// WM_CHAR CARRIES ONE UTF-16 CODE UNIT AT A TIME, SO A CHARACTER PAST THE BASIC
// MULTILINGUAL PLANE ARRIVES AS A SURROGATE PAIR ACROSS TWO MESSAGES. The high
// half is held on the window until the low half arrives and the pair is joined
// into one code point; a high half with no low half following — the sequence
// interrupted by a key that is not text — is simply replaced rather than joined
// into whatever comes next.
//
// EVERYTHING IS DROPPED WHILE CONTROL IS HELD WITHOUT ALT, SO A SHORTCUT DOES
// NOT ALSO TYPE. AltGr types, because Windows reports it as Control+Alt held
// together and that is indistinguishable here from the two held separately —
// so Control+Alt is let through on purpose, and it is also what makes an AltGr
// character in a language that needs one continue to work once this engine
// reads its keysym.
//
// WM_SYSKEYDOWN IS HANDLED ALONGSIDE WM_KEYDOWN AND MUST NOT BE SWALLOWED.
// Windows sends the SYS form for a key pressed while Alt is held, and for F10.
// The engine wants to know the key moved either way, so both are recorded — but
// the SYS pair then falls through to DefWindowProcW rather than returning zero,
// because that is what opens the window menu on Alt and closes the window on
// Alt+F4. A handler that returned zero here would take Alt+F4 away and it would
// not be obvious why.
//
// MOUSE LOOK IS RAW INPUT AND NOT WM_MOUSEMOVE. WM_MOUSEMOVE reports where the
// cursor is in the client area, which stops at the edge of the window; a camera
// needs how far the mouse moved, which does not. WM_INPUT reports the device's
// own relative counts and keeps reporting them when the cursor is against a
// screen edge, which is the whole reason it is registered for.
//
// THE POINTER'S POSITION IS WM_MOUSEMOVE, AND IT IS THE OTHER QUESTION. The
// message carries client coordinates: whole pixels, origin at the client area's
// top-left, +x right and +y down, the same space WM_SIZE measures the client
// area in — so a pointer and voe_platform_size agree without arithmetic. The two
// halves of lparam are signed, and are read as signed, because a captured
// pointer goes negative. This process declares no DPI awareness, so on a scaled
// display Windows virtualises both numbers by the same factor and they still
// agree with each other and with the swapchain. The button messages carry the
// same coordinates and are recorded the same way, so a press with no movement
// before it still knows where it landed.
//
// A BUTTON HELD TAKES THE CAPTURE, SO THAT THE RELEASE ARRIVES WHEREVER THE
// POINTER IS BY THEN. Without SetCapture, a press inside the window and a release
// outside it is a release Windows delivers to whoever is under the cursor, and
// the button here would read down for ever. Capture sends every mouse message to
// this window until it is given back, positions outside the client area
// included, which is what a drag past the edge wants and what Wayland does on
// its own. It is released when the last button goes up; WM_CAPTURECHANGED is the
// one place that reacts to losing it, whether by that release or by Windows
// taking it away, so a capture stolen mid-drag lifts the buttons the same way a
// lost focus lifts the keys.
//
// WM_MOUSELEAVE IS ASKED FOR, BECAUSE WINDOWS DOES NOT SEND IT UNASKED.
// TrackMouseEvent arms one notification and then forgets, so it is re-armed on
// the first movement inside after each one arrives. A leave with a button held
// is ignored — the drag is still on and the capture is what decides — and the
// release recomputes whether the pointer is still over the client area from
// where it was when the last button came up.
//
// THERE IS NO CS_DBLCLKS ON THE CLASS, AND THAT IS ON PURPOSE. With it Windows
// turns the second press of a double-click into a WM_xBUTTONDBLCLK and the
// down message never arrives; without it every press is a plain down, which is
// what a level-state API wants. What a double-click means is a caller's.
//
// THE LOCK IS A CLIP AND A HIDE, AND THERE IS NOTHING TO NEGOTIATE. Unlike
// Wayland, nothing here can refuse: ClipCursor confines the cursor to the client
// rectangle and ShowCursor hides it, so the flag src/input.h keeps is set from
// what was asked for rather than from an answer. ShowCursor is a counter and not
// a switch, which is what makes hiding reversible here and not on the other
// platform — see include/platform/input.h, which says why the two differ.
#include <platform/window.h>

#include "input.h"

#include <base/assert.h>

#include <windows.h>

#include <stdlib.h>

// One class for every window this process opens, registered on the first one.
#define WINDOW_CLASS L"voe_platform_window"

// The title is converted onto the stack rather than allocated, because _new does
// not take an arena and the engine has no global one to reach for. A title
// longer than this is a call-site mistake, not a runtime condition.
#define TITLE_MAX 256

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

	// A WM_MOUSELEAVE has been asked for and has not yet arrived. It is a
	// one-shot, so this says whether to ask again on the next movement.
	bool tracking_leave;

	// The high half of a UTF-16 surrogate pair, held between two WM_CHAR
	// messages until the low half joins it — see handle_char. 0 means none
	// is pending; 0 is never a valid high surrogate, so it doubles safely
	// as its own "nothing waiting" value.
	uint16_t pending_high_surrogate;

	struct voe_platform_input input;
};

static voe_platform_window *window_of(HWND hwnd)
{
	return (voe_platform_window *)(uintptr_t)GetWindowLongPtrW(hwnd,
								   GWLP_USERDATA);
}

// A virtual key to one of the eleven keys this engine reads, or
// VOE_PLATFORM_KEY_COUNT for everything else. The letters are their own ASCII
// capitals, which is what Windows defines VK_A..VK_Z to be and why there are no
// constants for them to name.
//
// VK_SHIFT AND VK_CONTROL ARRIVE AS THE UNSIDED VIRTUAL KEY AND THAT IS ENOUGH.
// Windows sends the generic VK for both ends unless the window asks for the
// sided ones, and include/platform/input.h folds left and right into one key
// anyway — so the two agree by doing nothing.
static voe_platform_key key_of(WPARAM virtual_key)
{
	switch (virtual_key) {
	case 'W':
		return VOE_PLATFORM_KEY_W;
	case 'A':
		return VOE_PLATFORM_KEY_A;
	case 'S':
		return VOE_PLATFORM_KEY_S;
	case 'D':
		return VOE_PLATFORM_KEY_D;
	case 'Q':
		return VOE_PLATFORM_KEY_Q;
	case 'E':
		return VOE_PLATFORM_KEY_E;
	case VK_SPACE:
		return VOE_PLATFORM_KEY_SPACE;
	case VK_CONTROL:
		return VOE_PLATFORM_KEY_CONTROL;
	case VK_SHIFT:
		return VOE_PLATFORM_KEY_SHIFT;
	case VK_TAB:
		return VOE_PLATFORM_KEY_TAB;
	case VK_ESCAPE:
		return VOE_PLATFORM_KEY_ESCAPE;
	case 'P':
		return VOE_PLATFORM_KEY_P;
	case 'N':
		return VOE_PLATFORM_KEY_N;
	case 'O':
		return VOE_PLATFORM_KEY_O;
	case VK_BACK:
		return VOE_PLATFORM_KEY_BACKSPACE;
	case VK_RETURN:
		return VOE_PLATFORM_KEY_ENTER;
	default:
		return VOE_PLATFORM_KEY_COUNT;
	}
}

static void key_set(voe_platform_window *window, WPARAM virtual_key, bool down)
{
	voe_platform_key key = key_of(virtual_key);

	if (key != VOE_PLATFORM_KEY_COUNT)
		window->input.keys[key] = down;
}

// A WM_CHAR message, which carries one UTF-16 code unit in wparam. See the
// header for why Control without Alt drops everything and why AltGr — Control
// and Alt together — still types.
//
// VK_MENU AND NOT window->input's OWN KEYS, BECAUSE ALT IS NOT ONE OF THEM.
// Nothing else in this engine reads Alt, so it has never earned a line in
// VOE_PLATFORM_KEY (rule 10), and this is the one place that needs to know
// about it — straight from Windows rather than by adding a key nothing else
// would ever read.
static void handle_char(voe_platform_window *window, WPARAM wparam)
{
	uint32_t unit = (uint32_t)(wparam & 0xffff);
	bool control = window->input.keys[VOE_PLATFORM_KEY_CONTROL];
	bool alt = (GetKeyState(VK_MENU) & 0x8000) != 0;

	if (control && !alt) {
		window->pending_high_surrogate = 0;
		return;
	}

	if (unit >= 0xd800 && unit <= 0xdbff) {
		// A high surrogate replaces whatever was pending rather than
		// joining it: two high halves in a row means the first one's
		// low half is never coming.
		window->pending_high_surrogate = (uint16_t)unit;
		return;
	}

	if (unit >= 0xdc00 && unit <= 0xdfff) {
		if (window->pending_high_surrogate != 0) {
			uint32_t code_point =
				0x10000 +
				(((uint32_t)window->pending_high_surrogate - 0xd800)
				 << 10) +
				(unit - 0xdc00);

			voe_platform_input_append_text(&window->input, code_point);
		}
		window->pending_high_surrogate = 0;
		return;
	}

	// An ordinary code unit, arriving with no surrogate pending or one that
	// was about to be discarded either way.
	window->pending_high_surrogate = 0;
	voe_platform_input_append_text(&window->input, unit);
}

// Focus arrived, so rebuild what is held from what the OS says is held. This is
// the counterpart of clearing everything on the way out, and it is the same
// thing wl_keyboard.enter's key array does on the other platform — without it,
// alt-tabbing back in while holding W leaves W reading up until it is pressed
// again.
//
// GetAsyncKeyState AND NOT GetKeyState, BECAUSE THE QUESTION IS ABOUT NOW.
// GetKeyState answers as of the last message this thread took off the queue,
// which at WM_SETFOCUS is a moment before focus arrived; GetAsyncKeyState reads
// the hardware state. The high bit is "down" — the low bit means something else
// entirely and reading the whole value as a bool is the classic mistake here.
static void focus_gained(voe_platform_window *window)
{
	static const int VIRTUAL_KEYS[VOE_PLATFORM_KEY_COUNT] = {
		[VOE_PLATFORM_KEY_W] = 'W',
		[VOE_PLATFORM_KEY_A] = 'A',
		[VOE_PLATFORM_KEY_S] = 'S',
		[VOE_PLATFORM_KEY_D] = 'D',
		[VOE_PLATFORM_KEY_Q] = 'Q',
		[VOE_PLATFORM_KEY_E] = 'E',
		[VOE_PLATFORM_KEY_SPACE] = VK_SPACE,
		[VOE_PLATFORM_KEY_CONTROL] = VK_CONTROL,
		[VOE_PLATFORM_KEY_SHIFT] = VK_SHIFT,
		[VOE_PLATFORM_KEY_TAB] = VK_TAB,
		[VOE_PLATFORM_KEY_ESCAPE] = VK_ESCAPE,
		[VOE_PLATFORM_KEY_P] = 'P',
		[VOE_PLATFORM_KEY_N] = 'N',
		[VOE_PLATFORM_KEY_O] = 'O',
		[VOE_PLATFORM_KEY_BACKSPACE] = VK_BACK,
		[VOE_PLATFORM_KEY_ENTER] = VK_RETURN,
	};

	for (int key = 0; key < VOE_PLATFORM_KEY_COUNT; key++)
		window->input.keys[key] =
			(GetAsyncKeyState(VIRTUAL_KEYS[key]) & 0x8000) != 0;
}

// Where the cursor may go while the pointer is locked: the client area, in
// screen coordinates, which is what ClipCursor wants and not what GetClientRect
// hands back. Re-applied on every ask because a window that moved or resized has
// left the old rectangle somewhere it no longer is.
static void clip_to_client(voe_platform_window *window)
{
	RECT client;
	POINT top_left = { 0, 0 };

	if (!GetClientRect(window->hwnd, &client))
		return;
	if (!ClientToScreen(window->hwnd, &top_left))
		return;

	client.left += top_left.x;
	client.right += top_left.x;
	client.top += top_left.y;
	client.bottom += top_left.y;
	ClipCursor(&client);
}

// Take the cursor or give it back, from the two flags that decide it. Every path
// that changes either flag ends here, so there is one place that knows what the
// cursor is doing and it cannot get out of step with itself.
//
// ShowCursor IS A COUNTER, WHICH IS WHY THIS GUARDS ON pointer_locked RATHER
// THAN CALLING IT EVERY TIME. Hiding four times needs showing four times, so a
// caller asking for a lock once a frame with no guard here would push the count
// down by sixty a second and never bring it back. The flag is what keeps the
// calls paired.
static void apply_lock(voe_platform_window *window)
{
	bool wanted = window->lock_wanted && window->focused;

	if (wanted == window->input.pointer_locked) {
		// Already where it should be. The clip is still refreshed,
		// because a window that moved has left it behind.
		if (wanted)
			clip_to_client(window);
		return;
	}

	if (wanted) {
		clip_to_client(window);
		ShowCursor(FALSE);
	} else {
		// NULL releases the cursor to the whole desktop. It is a global
		// setting, so leaving it behind would confine the cursor for
		// every other program on the machine.
		ClipCursor(NULL);
		ShowCursor(TRUE);
	}
	window->input.pointer_locked = wanted;
}

// One WM_INPUT message, which may carry several mouse movements. Only the mouse
// is registered for, so nothing here checks which device it was.
//
// MOUSE_MOVE_ABSOLUTE IS A REAL CASE, IT IS DROPPED, AND THE CONSEQUENCE IS THAT
// SOME MACHINES HAVE NO MOUSE LOOK AT ALL. A tablet, a touch digitiser, and the
// mice some remote-desktop sessions and virtual machines present report where the
// pointer is rather than how far it moved. Adding those coordinates up as though
// they were deltas sends the camera to the far corner of the world on the first
// event, so they are skipped — and skipping them means a device that only ever
// reports this way moves the camera not at all. Keys still work.
//
// IT IS NOT A FEW LINES TO FIX AND THAT IS WHY IT IS REPORTED HERE RATHER THAN
// HALF-DONE. Differencing consecutive absolute positions is easy on its own, but
// it does not survive the lock above: a cursor clipped to the client rectangle
// stops at the edge, so the differences go to nought exactly when a person is
// still turning. Making it work means locking a different way — recentring the
// cursor every frame and measuring from the middle — which is a second lock
// strategy, not a branch in this function. Whoever meets a dead mouse on a
// virtual machine should read this paragraph and write that card.
static void raw_input(voe_platform_window *window, HRAWINPUT handle)
{
	RAWINPUT raw;
	UINT size = sizeof(raw);

	if (GetRawInputData(handle, RID_INPUT, &raw, &size,
			    sizeof(RAWINPUTHEADER)) == (UINT)-1)
		return;
	if (raw.header.dwType != RIM_TYPEMOUSE)
		return;
	if ((raw.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) != 0)
		return;

	window->input.motion_x += (float)raw.data.mouse.lLastX;
	window->input.motion_y += (float)raw.data.mouse.lLastY;
}

// Whether the last recorded position is inside the client area. Asked when a
// capture ends, because a release outside the window is the one moment the
// pointer can be not over the window without a WM_MOUSELEAVE saying so.
static bool pointer_inside(const voe_platform_window *window)
{
	return window->input.pointer_x >= 0.0f &&
	       window->input.pointer_y >= 0.0f &&
	       window->input.pointer_x < (float)window->width &&
	       window->input.pointer_y < (float)window->height;
}

// Where the mouse messages say the pointer is. Read as two signed shorts and
// not through LOWORD alone, which is unsigned and would turn a captured pointer
// one pixel left of the window into a position sixty-five thousand pixels to the
// right.
static void pointer_at(voe_platform_window *window, LPARAM lparam)
{
	window->input.pointer_x = (float)(short)LOWORD(lparam);
	window->input.pointer_y = (float)(short)HIWORD(lparam);
	window->input.pointer_over = true;

	// Arm the leave notification once per visit, and only while inside:
	// asked for with the cursor already outside, it fires at once and says
	// nothing new.
	if (!window->tracking_leave && pointer_inside(window)) {
		TRACKMOUSEEVENT track = {
			.cbSize = sizeof(track),
			.dwFlags = TME_LEAVE,
			.hwndTrack = window->hwnd,
		};

		if (TrackMouseEvent(&track))
			window->tracking_leave = true;
	}
}

static bool any_button_down(const voe_platform_window *window)
{
	for (int button = 0; button < VOE_PLATFORM_BUTTON_COUNT; button++)
		if (window->input.buttons[button])
			return true;
	return false;
}

// A button message: the position it carries, the button's new state, and the
// capture that goes with a held button. ReleaseCapture sends WM_CAPTURECHANGED
// synchronously, so the recomputation of whether the pointer is still over the
// window happens there and in one place.
static void button_set(voe_platform_window *window, voe_platform_button button,
		       bool down, LPARAM lparam)
{
	pointer_at(window, lparam);
	window->input.buttons[button] = down;

	if (down)
		SetCapture(window->hwnd);
	else if (!any_button_down(window) && GetCapture() == window->hwnd)
		ReleaseCapture();
}

// The capture is gone — given back above, or taken by Windows when another
// window was activated mid-drag. Either way no more releases will arrive, so
// every button goes up, and whether the pointer is still over the window is
// decided from where it last was rather than waited for.
static void capture_lost(voe_platform_window *window)
{
	bool inside = pointer_inside(window);

	voe_platform_input_pointer_lost(&window->input);
	window->input.pointer_over = inside;
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
		apply_lock(window);
		return 0;
	case WM_KEYDOWN:
		key_set(window, wparam, true);
		return 0;
	case WM_KEYUP:
		key_set(window, wparam, false);
		return 0;
	case WM_CHAR:
		handle_char(window, wparam);
		return 0;
	// Recorded and then handed on, so that Alt and Alt+F4 still do what
	// Windows means them to do. See the header.
	case WM_SYSKEYDOWN:
		key_set(window, wparam, true);
		return DefWindowProcW(hwnd, message, wparam, lparam);
	case WM_SYSKEYUP:
		key_set(window, wparam, false);
		return DefWindowProcW(hwnd, message, wparam, lparam);
	case WM_INPUT:
		raw_input(window, (HRAWINPUT)lparam);
		// Handed on as well: the documentation asks for it, and the
		// system does cleanup for the message there.
		return DefWindowProcW(hwnd, message, wparam, lparam);
	case WM_MOUSEMOVE:
		pointer_at(window, lparam);
		return 0;
	case WM_MOUSELEAVE:
		window->tracking_leave = false;
		// With a button held the capture has the pointer and the drag
		// is still on; the release decides. See the header.
		if (!any_button_down(window))
			voe_platform_input_pointer_lost(&window->input);
		return 0;
	case WM_LBUTTONDOWN:
		button_set(window, VOE_PLATFORM_BUTTON_LEFT, true, lparam);
		return 0;
	case WM_LBUTTONUP:
		button_set(window, VOE_PLATFORM_BUTTON_LEFT, false, lparam);
		return 0;
	case WM_RBUTTONDOWN:
		button_set(window, VOE_PLATFORM_BUTTON_RIGHT, true, lparam);
		return 0;
	case WM_RBUTTONUP:
		button_set(window, VOE_PLATFORM_BUTTON_RIGHT, false, lparam);
		return 0;
	case WM_MBUTTONDOWN:
		button_set(window, VOE_PLATFORM_BUTTON_MIDDLE, true, lparam);
		return 0;
	case WM_MBUTTONUP:
		button_set(window, VOE_PLATFORM_BUTTON_MIDDLE, false, lparam);
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
		capture_lost(window);
		return 0;
	case WM_SETFOCUS:
		window->focused = true;
		focus_gained(window);
		apply_lock(window);
		return 0;
	case WM_KILLFOCUS:
		// The clip is not ours to hold while somebody else has focus,
		// and a hidden cursor over another window is worse still. Both
		// come back on the way in, without the caller asking again —
		// lock_wanted is still set and apply_lock reads it.
		window->focused = false;
		voe_platform_input_focus_lost(&window->input);
		// A high surrogate half-typed before focus went away has no low
		// half coming from whoever gets it next.
		window->pending_high_surrogate = 0;
		apply_lock(window);
		return 0;
	default:
		return DefWindowProcW(hwnd, message, wparam, lparam);
	}
}

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

voe_platform_window *voe_platform_window_new(int width, int height,
					     const char *title)
{
	voe_platform_window *window;
	wchar_t wide_title[TITLE_MAX];
	RECT wanted = { 0, 0, width, height };
	DWORD style = WS_OVERLAPPEDWINDOW;

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

	if (!class_ready(window->instance)) {
		free(window);
		return NULL;
	}

	// width and height are the client area, and CreateWindowExW is given the
	// outer size, so the decorations have to be added on.
	AdjustWindowRect(&wanted, style, FALSE);

	window->hwnd = CreateWindowExW(0, WINDOW_CLASS, wide_title, style,
				       CW_USEDEFAULT, CW_USEDEFAULT,
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

	ShowWindow(window->hwnd, SW_SHOW);

	// ShowWindow activates the window and WM_SETFOCUS has already been
	// through the procedure by here — but only if this window really got
	// focus, which a shell policy or another program grabbing it can
	// prevent. Asking rather than assuming costs one call and means the
	// flag is right either way.
	if (GetFocus() == window->hwnd && !window->focused) {
		window->focused = true;
		focus_gained(window);
	}

	return window;
}

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

	window->lock_wanted = lock;
	apply_lock(window);
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
	apply_lock(window);

	if (window->hwnd != NULL)
		DestroyWindow(window->hwnd);
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
