// A window: it opens, it resizes, it says when the user asked to close it, and
// it hands out the two OS handles a graphics API needs to draw into it. That is
// the whole of it. Not input, not files, not time — platform will own those, and
// the card that first needs one adds it.
//
//     voe_platform_window *w = voe_platform_window_new(960, 540, "voe3d");
//     if (w == NULL)
//             return 1;                       // see "failure" below
//     while (!voe_platform_window_should_close(w))
//             voe_platform_window_poll(w);
//     voe_platform_window_destroy(w);
//
// FAILURE IS A RETURNED NULL, AND ONLY HERE. Opening a window can genuinely
// fail — no compositor, no display, a session that is not there — and that is
// not the program being wrong, so it is not an assert. How a recoverable failure
// gets reported across this engine is not decided yet, and nothing here invents
// an answer: _new returns NULL and the caller decides. Every other function on
// this page assumes a window that opened. Running out of memory is a separate
// matter and stays fatal, through base/assert.h.
//
// POLL DOES NOT RETURN EVENTS. It drains whatever the OS has queued and folds it
// into the window's own state; you then ask for that state with
// _should_close and _size. There is no event queue, no callback and no listener,
// because nothing in this engine consumes a keystroke yet. Call it once a frame.
//
// _poll returns immediately whether or not anything happened. It is not a wait,
// and platform has no way to wait yet, so a loop with nothing else in it will
// spin a core.
#pragma once

#include <stdint.h>

typedef struct voe_platform_window voe_platform_window;

// The client area, in pixels — the part you draw into, never including whatever
// decoration the window system may have put around it.
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
voe_platform_window *voe_platform_window_new(int width, int height,
					     const char *title);
void voe_platform_window_destroy(voe_platform_window *window);

void voe_platform_window_poll(voe_platform_window *window);
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
