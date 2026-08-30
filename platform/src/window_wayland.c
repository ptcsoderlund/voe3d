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
// DEVIATION: card 004 ("do not invent an event queue, a callback, or a
// listener"), narrowest reading, the placeholder buffer below. The card's only
// verification is a person seeing the window, and on Wayland a surface with no
// buffer ever committed is never mapped: there would be nothing to see and
// nothing to resize. So this file commits a flat-coloured shared-memory buffer
// purely so the window exists on screen. It is a stand-in for the Vulkan
// swapchain, it is the reason wl_shm is bound at all, and every line of it goes
// when render attaches a real surface. It adds nothing to the public API.
//
// A compositor that goes away mid-run sets should_close, because there is no
// other way out: _poll cannot report anything and the caller is in a loop. That
// is a papering-over, not a design, and it is written up on card 004.
//
// The placeholder's memory is one memfd, mapped once, sized for a window no
// larger than PLACEHOLDER_MAX_* and never unmapped until the window is
// destroyed. wl_buffer objects come and go against that one mapping as the
// window resizes, which is what makes destroying a buffer the compositor may
// still be reading from harmless: the object goes, the pages stay.
#define _GNU_SOURCE

#include <platform/window.h>

#include <base/assert.h>

#include <wayland-client.h>

#include "xdg-decoration-unstable-v1-client-protocol.h"
#include "xdg-shell-client-protocol.h"

#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

// The largest window the placeholder can fill. Beyond it the buffer is clamped
// and the window stops growing on screen while _size keeps reporting the truth.
// A real swapchain has no such limit; this one exists so the mapping can be made
// once and never moved.
#define PLACEHOLDER_MAX_WIDTH 2560
#define PLACEHOLDER_MAX_HEIGHT 1440
#define PLACEHOLDER_COLOUR 0xff1e1e28u

struct voe_platform_window {
	struct wl_display *display;
	struct wl_registry *registry;
	struct wl_compositor *compositor;
	struct wl_shm *shm;
	struct xdg_wm_base *wm_base;
	struct zxdg_decoration_manager_v1 *decorations;

	struct wl_surface *surface;
	struct xdg_surface *xdg_surface;
	struct xdg_toplevel *toplevel;
	struct zxdg_toplevel_decoration_v1 *decoration;

	struct wl_shm_pool *pool;
	struct wl_buffer *buffer;
	unsigned char *pool_memory;
	size_t pool_bytes;
	int pool_fd;

	int width;
	int height;
	int wanted_width;
	int wanted_height;
	uint32_t decoration_mode;
	bool configured;
	bool should_close;
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
	else if (strcmp(interface, wl_shm_interface.name) == 0)
		window->shm = wl_registry_bind(registry, name,
					       &wl_shm_interface, 1);
	else if (strcmp(interface, xdg_wm_base_interface.name) == 0)
		window->wm_base = wl_registry_bind(registry, name,
						   &xdg_wm_base_interface, 1);
	// Optional, and absent on a compositor that draws no frames. Everything
	// downstream checks for NULL rather than assuming it arrived.
	else if (strcmp(interface, zxdg_decoration_manager_v1_interface.name) == 0)
		window->decorations = wl_registry_bind(registry, name,
						       &zxdg_decoration_manager_v1_interface,
						       1);
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

// The compositor's half of the resize handshake: it proposes, this acks, and the
// next commit has to carry a buffer of the acked size. The commit is deliberately
// not made here — poll does it — so that a configure arriving in the middle of
// window creation and one arriving mid-frame take the same path.
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

// ---------------------------------------------------------- placeholder pixels

static int clamp_to(int value, int limit)
{
	return value > limit ? limit : value;
}

static bool pool_open(voe_platform_window *window)
{
	size_t bytes = (size_t)PLACEHOLDER_MAX_WIDTH *
		       (size_t)PLACEHOLDER_MAX_HEIGHT * 4u;
	void *mapped;

	window->pool_fd = memfd_create("voe-window-placeholder", MFD_CLOEXEC);
	if (window->pool_fd < 0)
		return false;
	if (ftruncate(window->pool_fd, (off_t)bytes) < 0)
		return false;

	// Sparse: the file is large, and only the pages actually written cost
	// anything.
	mapped = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_SHARED,
		      window->pool_fd, 0);
	if (mapped == MAP_FAILED)
		return false;

	window->pool_memory = mapped;
	window->pool_bytes = bytes;
	window->pool = wl_shm_create_pool(window->shm, window->pool_fd,
					  (int32_t)bytes);
	return window->pool != NULL;
}

// Fill, attach, commit — and only then drop the buffer the compositor was shown
// last, because the object may go while the pages behind it may not.
static void present(voe_platform_window *window)
{
	int width = clamp_to(window->width, PLACEHOLDER_MAX_WIDTH);
	int height = clamp_to(window->height, PLACEHOLDER_MAX_HEIGHT);
	uint32_t *pixel = (uint32_t *)window->pool_memory;
	struct wl_buffer *buffer;

	for (int i = 0; i < width * height; i++)
		pixel[i] = PLACEHOLDER_COLOUR;

	buffer = wl_shm_pool_create_buffer(window->pool, 0, width, height,
					   width * 4, WL_SHM_FORMAT_XRGB8888);
	wl_surface_attach(window->surface, buffer, 0, 0);
	wl_surface_damage(window->surface, 0, 0, width, height);
	wl_surface_commit(window->surface);

	if (window->buffer != NULL)
		wl_buffer_destroy(window->buffer);
	window->buffer = buffer;
}

// -------------------------------------------------------------- open and close

static void close_down(voe_platform_window *window)
{
	if (window->buffer != NULL)
		wl_buffer_destroy(window->buffer);
	if (window->pool != NULL)
		wl_shm_pool_destroy(window->pool);
	if (window->pool_memory != NULL)
		munmap(window->pool_memory, window->pool_bytes);
	if (window->pool_fd >= 0)
		close(window->pool_fd);

	if (window->decoration != NULL)
		zxdg_toplevel_decoration_v1_destroy(window->decoration);
	if (window->toplevel != NULL)
		xdg_toplevel_destroy(window->toplevel);
	if (window->xdg_surface != NULL)
		xdg_surface_destroy(window->xdg_surface);
	if (window->surface != NULL)
		wl_surface_destroy(window->surface);

	if (window->decorations != NULL)
		zxdg_decoration_manager_v1_destroy(window->decorations);
	if (window->wm_base != NULL)
		xdg_wm_base_destroy(window->wm_base);
	if (window->shm != NULL)
		wl_shm_destroy(window->shm);
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

	window->pool_fd = -1;
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
	if (window->compositor == NULL || window->shm == NULL ||
	    window->wm_base == NULL)
		return open_failed(window);
	xdg_wm_base_add_listener(window->wm_base, &wm_base_listener, window);

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

	if (!pool_open(window))
		return open_failed(window);

	// The empty commit that asks for the first configure, then the wait for
	// it. Attaching a buffer before that configure is a protocol error, so
	// there is no window until it arrives. One roundtrip is normally enough;
	// the loop is for a compositor that takes its time, and giving up is a
	// failure to open like any other.
	wl_surface_commit(window->surface);
	for (int attempt = 0; attempt < 4 && !window->configured; attempt++) {
		if (wl_display_roundtrip(window->display) < 0)
			return open_failed(window);
	}
	if (!window->configured)
		return open_failed(window);

	window->width = window->wanted_width;
	window->height = window->wanted_height;
	present(window);
	if (wl_display_roundtrip(window->display) < 0)
		return open_failed(window);

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

	pump(window);

	// A connection that has died cannot be reported: _poll returns nothing,
	// and the NULL from _new only covers a window that never opened. Rather
	// than spin forever on a socket that will never speak again, this is
	// folded into should_close — which is a paper over a hole, and the hole
	// is that the engine has no way to say "this failed after it started".
	if (wl_display_get_error(window->display) != 0)
		window->should_close = true;

	// The commit the acked configure owes the compositor. Doing it here
	// rather than in the handler keeps one path for a resize whenever it
	// lands.
	if (window->wanted_width != window->width ||
	    window->wanted_height != window->height) {
		window->width = window->wanted_width;
		window->height = window->wanted_height;
		present(window);
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
