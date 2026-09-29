// A window: it opens, it resizes, it says when the user asked to close it, and
// it hands out the two OS handles a graphics API needs to draw into it. That is
// the whole of it. Not files and not time — platform will own those, and the
// card that first needs one adds it. Input belongs to a window and is on the
// next page: see platform/input.h.
//
//     voe_platform_window *w = voe_platform_window_new(960, 540, false, "voe3d");
//     if (w == NULL)
//             return 1;                       // see "failure" below
//     while (!voe_platform_window_should_close(w))
//             voe_platform_window_poll(w);
//     voe_platform_window_destroy(w);
//
// FAILURE IS A RETURNED NULL, AND ONLY HERE. Opening a window can genuinely
// fail — no compositor, no display, a session that is not there — and that is
// not the program being wrong, so it is not an assert. Every other function on
// this page assumes a window that opened. Running out of memory is a separate
// matter and stays fatal, through base/assert.h.
//
// A window that opened but has no keyboard and no mouse is not one of those
// failures. It is a window: it draws, it resizes and it closes, and every key on
// it reads as up. See platform/input.h.
//
// POLL DOES NOT RETURN EVENTS, AND THAT IS THE WHOLE FOLDER'S SHAPE AND NOT JUST
// THIS PAGE'S. It drains whatever the OS has queued and folds it into the
// window's own state; you then ask for that state with _should_close, _size,
// _decorated and everything in platform/input.h. There is no event queue, no
// callback and no listener anywhere in platform. Call it once a frame, and note
// that it is the only poll there is — input has no second one, and a frame that
// forgets this one gets a window that never closes and a keyboard that never
// moves.
//
// _poll returns immediately whether or not anything happened. It is not a wait;
// _wait is, and its seconds are a ceiling and never a guarantee. The call
// returns the moment anything at all arrives on the window's connection, and
// that traffic — a compositor's bookkeeping, a swapchain's buffer releases —
// says nothing about what a program cares about. A caller that wants a deadline
// loops around _wait and _poll and asks a clock whether the time has passed
// (ADR-0216). The wait itself reads what arrived and folds nothing — the next
// _poll does that, as ever.
//
// _focused and _visible change only at _poll, and both answer true before the
// first configure. On Wayland focused is the toplevel's activated state and
// visible is the absence of suspended; a compositor older than xdg-shell 6
// never says suspended, so its window is always visible — and that compositor
// is warned about at open, so the always-visible window is a known fact rather
// than a guess. On Windows focused is
// activation and visible is not minimised; a window on another virtual desktop
// is not detected (ADR-0215).
#pragma once

#include <stdint.h>

typedef struct voe_platform_window voe_platform_window;

// The client area, in the buffer's pixels — the part you draw into, never
// including whatever decoration the window system may have put around it. On a
// fractionally scaled Wayland output those pixels are more than the logical size
// asked for in _new (1.25 turns 1536x864 into 1920x1080), because the window is
// drawn at the output's scale rather than stretched by the compositor
// (ADR-0180); elsewhere the two are the same.
typedef struct {
	int width;
	int height;
} voe_platform_size;

// The two OS handles a graphics API needs, as plain integers, because render may
// not include an OS header and platform may not know what Vulkan is. The caller
// casts them back. Deliberately crude: the alternative is this folder naming a
// graphics API, which points the module map the wrong way round.
//
//              context                      window
//     Linux    struct wl_display *          struct wl_surface *
//     Windows  HINSTANCE                    HWND
//
// Linux is Wayland, and there is no X11 backend. A Vulkan caller wants
// VK_KHR_wayland_surface there and VK_KHR_win32_surface on Windows.
typedef struct {
	uintptr_t context;
	uintptr_t window;
} voe_platform_native;

// width and height are the client area asked for; the window system may open the
// window at a different size, so ask _size rather than assuming. title is UTF-8.
// Returns NULL if the window could not be opened.
//
// FULLSCREEN COVERS A WHOLE SCREEN, AND THE SIZE IS THEN NOT THE WINDOW'S. width
// and height are only what the window returns to if the system un-fullscreens
// it; the screen's size arrives the way any size does, so ask _size, as ever.
// Wayland asks for fullscreen on no named output before the first configure;
// Windows opens a frameless popup over the primary monitor. There is no switch
// while open: the choice is made here, once (rule 10).
voe_platform_window *voe_platform_window_new(int width, int height,
					     bool fullscreen, const char *title);
void voe_platform_window_destroy(voe_platform_window *window);

void voe_platform_window_poll(voe_platform_window *window);
// A negative seconds waits with no timeout.
void voe_platform_window_wait(voe_platform_window *window, double seconds);
// The window is the one the person is working in.
bool voe_platform_window_focused(voe_platform_window *window);
// Any of the window can be on screen.
bool voe_platform_window_visible(voe_platform_window *window);
bool voe_platform_window_should_close(voe_platform_window *window);
voe_platform_size voe_platform_window_size(voe_platform_window *window);
voe_platform_native voe_platform_window_native(voe_platform_window *window);

// WHO IS RESPONSIBLE for the window's frame — not whether one is on screen.
// True when the window system has taken the job: Windows always, Linux when the
// compositor answered server-side. False when nothing offered the protocol, or
// when it answered client-side, which is the only case in which drawing our own
// frame would ever be the right thing to do.
//
// The distinction is written down because it was measured, and the measurement
// is worth more than the function. On KWin, toggling "No Borders" makes the
// compositor re-answer — the event fires — and the answer is SERVER_SIDE both
// with a titlebar and without one. KWin is responsible for the frame in both
// states; it has merely chosen to draw nothing in one of them.
//
// SO THIS CANNOT TELL YOU WHETHER A FRAME IS ON SCREEN, and no amount of code
// here can: the protocol does not carry it. What does change is the client area
// — 960x540 decorated, 970x567 with the frame gone, the compositor handing over
// the space the titlebar had. A decoration change is therefore invisible through
// this function and arrives as a plain resize, which is the event anything
// holding a swapchain has to react to anyway. Nothing is lost.
//
// It can still change while the window is open — a compositor that answers
// client-side when it draws nothing would move it, and none has been tested that
// does — so ask rather than remember.
//
// Nothing in the engine behaves differently on the answer, and nothing should.
// It exists to be looked at.
bool voe_platform_window_decorated(voe_platform_window *window);

// Takes back a close the user just asked for: should_close reads false again
// after this, exactly as if the request had never arrived.
//
// THIS IS FOR A CALLER THAT REFUSES TO CLOSE ONCE, NOT FOR ONE THAT NEVER
// CLOSES. Unsaved work is the caller's business — a top bar arms a warning on
// the first close and lets the second one through — and this is the one call
// that lets it undo what the window system already recorded. Calling it when
// should_close is already false does nothing.
//
// A COMPOSITOR OR CONNECTION THAT HAS DIED SETS should_close AGAIN AT THE NEXT
// POLL, ON BOTH PLATFORMS. That is not the user asking twice; it is the same
// "there is no other way out" this folder already falls back to when a socket
// stops answering, and refusing it here would only be reason enough to try it
// once, then watch the window never close.
void voe_platform_window_close_refuse(voe_platform_window *window);
