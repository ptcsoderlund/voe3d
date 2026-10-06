// The seat half of the Windows window: the keyboard, WM_CHAR text, the mouse
// as a raw input device for look and as ordinary messages for position and
// buttons, the pointer's shape, and the lock. The window it serves, its class
// and its message pump are src/window_win32.c; the struct both halves write,
// and the functions window_proc calls here, are src/window_win32.h.
//
// A SEPARATE FILE OF THE SAME BACKEND, NOT A SEPARATE LAYER. Every keystroke and
// every mouse movement arrives as a message to window_proc, dispatched by the
// same PeekMessageW loop a resize comes through, and window_proc hands each one
// to a function here that takes the whole window. What these fill is
// src/input.h, which is the half that is not Windows' and is shared with
// Wayland. It is its own file because the window file was past 800 lines and
// this is the half that grows.
//
// THE LOCK IS A CLIP AND A HIDE, AND THERE IS NOTHING TO NEGOTIATE. Unlike
// Wayland, nothing here can refuse: ClipCursor confines the cursor to the client
// rectangle and ShowCursor hides it, so the flag src/input.h keeps is set from
// what was asked for rather than from an answer. ShowCursor is a counter and not
// a switch, so hiding is reversible. The lock gives the pointer back where it
// was taken: the screen position is kept when the clip starts and put back
// before the cursor is shown.
#include "window_win32.h"

#include <base/assert.h>

#define RAW_INPUT_BYTES 1024

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
	case VK_DELETE:
		return VOE_PLATFORM_KEY_DELETE;
	case 'Z':
		return VOE_PLATFORM_KEY_Z;
	case 'Y':
		return VOE_PLATFORM_KEY_Y;
	case 'R':
		return VOE_PLATFORM_KEY_R;
	case 'F':
		return VOE_PLATFORM_KEY_F;
	default:
		return VOE_PLATFORM_KEY_COUNT;
	}
}

void voe_platform_seat_key_set(voe_platform_window *window, WPARAM virtual_key,
			       bool down)
{
	voe_platform_key key = key_of(virtual_key);

	if (key != VOE_PLATFORM_KEY_COUNT)
		window->input.keys[key] = down;
}

// A WM_CHAR message, which carries one UTF-16 code unit in wparam.
//
// A KEY IS STILL A PLACE AND NOT A LETTER, AND WM_CHAR ANSWERS A DIFFERENT
// QUESTION FROM WM_KEYDOWN'S — BUT THIS FILE NOW READS BOTH. WM_KEYDOWN carries
// a virtual key, a position on the keyboard, and that is still all key_of and
// voe_platform_seat_key_set ever look at. WM_CHAR carries what the active
// layout says that keystroke types, already resolved by TranslateMessage — no keymap of our own
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
// VK_MENU AND NOT window->input's OWN KEYS, BECAUSE ALT IS NOT ONE OF THEM.
// Nothing else in this engine reads Alt, so it has never earned a line in
// VOE_PLATFORM_KEY (rule 10), and this is the one place that needs to know
// about it — straight from Windows rather than by adding a key nothing else
// would ever read.
void voe_platform_seat_handle_char(voe_platform_window *window, WPARAM wparam)
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
void voe_platform_seat_focus_gained(voe_platform_window *window)
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
		[VOE_PLATFORM_KEY_DELETE] = VK_DELETE,
		[VOE_PLATFORM_KEY_Z] = 'Z',
		[VOE_PLATFORM_KEY_Y] = 'Y',
		[VOE_PLATFORM_KEY_R] = 'R',
		[VOE_PLATFORM_KEY_F] = 'F',
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
void voe_platform_seat_apply_lock(voe_platform_window *window)
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
		// Where it was taken, so it is given back there rather than
		// wherever raw motion pushed it against the clip.
		GetCursorPos(&window->lock_taken_at);
		clip_to_client(window);
		ShowCursor(FALSE);
	} else {
		// NULL releases the cursor to the whole desktop. It is a global
		// setting, so leaving it behind would confine the cursor for
		// every other program on the machine.
		ClipCursor(NULL);
		SetCursorPos(window->lock_taken_at.x, window->lock_taken_at.y);
		ShowCursor(TRUE);
	}
	window->input.pointer_locked = wanted;
}

// MOUSE LOOK IS RAW INPUT AND NOT WM_MOUSEMOVE. WM_MOUSEMOVE reports where the
// cursor is in the client area, which stops at the edge of the window; a camera
// needs how far the mouse moved, which does not. WM_INPUT reports the device's
// own relative counts and keeps reporting them when the cursor is against a
// screen edge, which is the whole reason it is registered for.
//
// One WM_INPUT message, which may carry several mouse movements, or a HID pad's
// reports, handed to src/gamepad_win32.c. The buffer is RAW_INPUT_BYTES because
// a HID report follows the struct; one larger than that is dropped.
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
void voe_platform_seat_raw_input(voe_platform_window *window, HRAWINPUT handle)
{
	union {
		RAWINPUT raw;
		BYTE bytes[RAW_INPUT_BYTES];
	} input;
	const RAWINPUT *raw = &input.raw;
	UINT size = sizeof(input);

	if (GetRawInputData(handle, RID_INPUT, &input, &size,
			    sizeof(RAWINPUTHEADER)) == (UINT)-1)
		return;
	if (raw->header.dwType == RIM_TYPEHID) {
		voe_platform_gamepads_hid(window, raw);
		return;
	}
	if (raw->header.dwType != RIM_TYPEMOUSE)
		return;
	if ((raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) != 0)
		return;

	window->input.motion_x += (float)raw->data.mouse.lLastX;
	window->input.motion_y += (float)raw->data.mouse.lLastY;
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
// Where the mouse messages say the pointer is. Read as two signed shorts and
// not through LOWORD alone, which is unsigned and would turn a captured pointer
// one pixel left of the window into a position sixty-five thousand pixels to the
// right.
void voe_platform_seat_pointer_at(voe_platform_window *window, LPARAM lparam)
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

bool voe_platform_seat_any_button_down(const voe_platform_window *window)
{
	for (int button = 0; button < VOE_PLATFORM_BUTTON_COUNT; button++)
		if (window->input.buttons[button])
			return true;
	return false;
}

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
// A button message: the position it carries, the button's new state, and the
// capture that goes with a held button. ReleaseCapture sends WM_CAPTURECHANGED
// synchronously, so the recomputation of whether the pointer is still over the
// window happens there and in one place.
void voe_platform_seat_button_set(voe_platform_window *window,
				  voe_platform_button button, bool down,
				  LPARAM lparam)
{
	voe_platform_seat_pointer_at(window, lparam);
	window->input.buttons[button] = down;

	if (down)
		SetCapture(window->hwnd);
	else if (!voe_platform_seat_any_button_down(window) &&
		 GetCapture() == window->hwnd)
		ReleaseCapture();
}

// The capture is gone — given back above, or taken by Windows when another
// window was activated mid-drag. Either way no more releases will arrive, so
// every button goes up, and whether the pointer is still over the window is
// decided from where it last was rather than waited for.
void voe_platform_seat_capture_lost(voe_platform_window *window)
{
	bool inside = pointer_inside(window);

	voe_platform_input_pointer_lost(&window->input);
	window->input.pointer_over = inside;
}

// The stored shape as a system cursor, drawn by Windows in the person's theme.
// A shared system cursor from LoadCursorW is never destroyed.
void voe_platform_seat_cursor_show(const voe_platform_window *window)
{
	LPCWSTR name = (LPCWSTR)IDC_ARROW;

	VOE_BASE_DEBUG_ASSERT(window != NULL, "shaping a NULL window's pointer");
	VOE_BASE_DEBUG_ASSERT(window->input.cursor < VOE_PLATFORM_CURSOR_COUNT,
			      "the stored cursor is not a shape");

	if (window->input.cursor == VOE_PLATFORM_CURSOR_LEFT_RIGHT)
		name = (LPCWSTR)IDC_SIZEWE;
	else if (window->input.cursor == VOE_PLATFORM_CURSOR_UP_DOWN)
		name = (LPCWSTR)IDC_SIZENS;
	SetCursor(LoadCursorW(NULL, name));
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

	window->lock_wanted = lock;
	voe_platform_seat_apply_lock(window);
}

void voe_platform_window_cursor(voe_platform_window *window,
				voe_platform_cursor cursor)
{
	VOE_BASE_DEBUG_ASSERT(window != NULL, "shaping a NULL window's pointer");
	VOE_BASE_DEBUG_ASSERT(cursor == window->input.cursor,
			      "input.c stores the shape before calling here");

	// Now, not at the next WM_SETCURSOR, which waits for the mouse to move.
	if (window->input.pointer_over)
		voe_platform_seat_cursor_show(window);
}
