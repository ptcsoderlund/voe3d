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
// Every global but the shell is bound at version 1, and a listener slot for an
// event the bound version never sends stays NULL — which matters, because
// libwayland calls straight through a listener's function pointer and a NULL
// one is a crash rather than a no-op. Binding low is what keeps the slots left
// NULL here and in seat_wayland.c unreachable. xdg_wm_base is bound at up to
// version 6, because 6 is where the toplevel says it is suspended — hidden —
// which is how a window knows to stop drawing (ADR-0215); every toplevel slot a
// version up to 6 can send has a function.
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
// The seat — keyboard, pointer, lock — is src/seat_wayland.c, and the struct
// both files share is src/window_wayland.h.
//
// _GNU_SOURCE is what makes <poll.h> declare poll() under -std=c23, which the
// engine builds with. It is a feature-test macro and not a use of any GNU
// extension.
#define _GNU_SOURCE

#include <platform/window.h>

#include "window_wayland.h"
#include "scale.h"

#include <base/assert.h>

#include "fractional-scale-v1-client-protocol.h"
#include "pointer-constraints-unstable-v1-client-protocol.h"
#include "relative-pointer-unstable-v1-client-protocol.h"
#include "viewporter-client-protocol.h"
#include "xdg-decoration-unstable-v1-client-protocol.h"
#include "xdg-shell-client-protocol.h"

#include <poll.h>
#include <stdlib.h>
#include <string.h>


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

	if (strcmp(interface, wl_compositor_interface.name) == 0)
		window->compositor = wl_registry_bind(registry, name,
						      &wl_compositor_interface, 1);
	else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
		window->shell_version = version < 6 ? version : 6;
		window->wm_base = wl_registry_bind(registry, name,
			&xdg_wm_base_interface, window->shell_version);
	}
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
	// without them. Nothing warns: see relative_pointer_motion.
	else if (strcmp(interface, zwp_relative_pointer_manager_v1_interface.name) == 0)
		window->relative_pointers = wl_registry_bind(registry, name,
			&zwp_relative_pointer_manager_v1_interface, 1);
	else if (strcmp(interface, zwp_pointer_constraints_v1_interface.name) == 0)
		window->constraints = wl_registry_bind(registry, name,
			&zwp_pointer_constraints_v1_interface, 1);
	// Drawing at the fractional scale takes both; either alone is unused.
	else if (strcmp(interface, wp_fractional_scale_manager_v1_interface.name) == 0)
		window->fractional_scales = wl_registry_bind(registry, name,
			&wp_fractional_scale_manager_v1_interface, 1);
	else if (strcmp(interface, wp_viewporter_interface.name) == 0)
		window->viewporter = wl_registry_bind(registry, name,
			&wp_viewporter_interface, 1);
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
// window first opens, so the size asked for at _new is kept in that case. The
// states are the whole set every time, so a state not listed is off.
static void toplevel_configure(void *data, struct xdg_toplevel *toplevel,
			       int32_t width, int32_t height,
			       struct wl_array *states)
{
	voe_platform_window *window = data;
	const uint32_t *state;

	(void)toplevel;

	if (width > 0 && height > 0) {
		window->wanted_width = width;
		window->wanted_height = height;
	}

	window->wanted_focused = false;
	window->wanted_suspended = false;
	wl_array_for_each(state, states) {
		if (*state == XDG_TOPLEVEL_STATE_ACTIVATED)
			window->wanted_focused = true;
		else if (*state == XDG_TOPLEVEL_STATE_SUSPENDED)
			window->wanted_suspended = true;
	}
}

static void toplevel_close(void *data, struct xdg_toplevel *toplevel)
{
	voe_platform_window *window = data;

	(void)toplevel;
	window->should_close = true;
}

// Version 4 and 5 events, which arrive now that the shell is bound higher.
// Nothing here wants either; they exist so the slot is not NULL.
static void toplevel_configure_bounds(void *data, struct xdg_toplevel *toplevel,
				      int32_t width, int32_t height)
{
	(void)data;
	(void)toplevel;
	(void)width;
	(void)height;
}

static void toplevel_wm_capabilities(void *data, struct xdg_toplevel *toplevel,
				     struct wl_array *capabilities)
{
	(void)data;
	(void)toplevel;
	(void)capabilities;
}

static const struct xdg_toplevel_listener toplevel_listener = {
	.configure = toplevel_configure,
	.close = toplevel_close,
	.configure_bounds = toplevel_configure_bounds,
	.wm_capabilities = toplevel_wm_capabilities,
};

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

// The compositor's preferred scale for the surface, in 120ths. Stored, and the
// viewport marked stale; _poll folds it in, as it does a new size.
static void fractional_scale_preferred(void *data,
				       struct wp_fractional_scale_v1 *fractional_scale,
				       uint32_t scale)
{
	voe_platform_window *window = data;

	(void)fractional_scale;

	if (scale != window->scale) {
		window->scale = scale;
		window->viewport_stale = true;
	}
}

static const struct wp_fractional_scale_v1_listener fractional_scale_listener = {
	.preferred_scale = fractional_scale_preferred,
};

// -------------------------------------------------------------- open and close

static void close_down(voe_platform_window *window)
{
	// Input first, before the managers its objects were made from; the
	// order inside the seat is seat_wayland.c's.
	voe_platform_seat_release(window);

	if (window->fractional_scale != NULL)
		wp_fractional_scale_v1_destroy(window->fractional_scale);
	if (window->viewport != NULL)
		wp_viewport_destroy(window->viewport);
	if (window->decoration != NULL)
		zxdg_toplevel_decoration_v1_destroy(window->decoration);
	if (window->toplevel != NULL)
		xdg_toplevel_destroy(window->toplevel);
	if (window->xdg_surface != NULL)
		xdg_surface_destroy(window->xdg_surface);
	if (window->surface != NULL)
		wl_surface_destroy(window->surface);

	if (window->viewporter != NULL)
		wp_viewporter_destroy(window->viewporter);
	if (window->fractional_scales != NULL)
		wp_fractional_scale_manager_v1_destroy(window->fractional_scales);
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

// The viewport shows the buffer at the logical size, whatever the scale made the
// buffer. Set only when stale; the swapchain's next present commits it.
static void viewport_update(voe_platform_window *window)
{
	if (window->viewport != NULL && window->viewport_stale)
		wp_viewport_set_destination(window->viewport, window->width,
					    window->height);
	window->viewport_stale = false;
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
	window->scale = 120;
	window->focused = true;
	window->wanted_focused = true;

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
		wl_seat_add_listener(window->seat, &voe_platform_seat_listener, window);

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

	// Draw at the compositor's fractional scale when it offers the means to;
	// without both objects the scale stays 120 and nothing changes.
	if (window->fractional_scales != NULL && window->viewporter != NULL) {
		window->viewport = wp_viewporter_get_viewport(window->viewporter,
							      window->surface);
		window->fractional_scale =
			wp_fractional_scale_manager_v1_get_fractional_scale(
				window->fractional_scales, window->surface);
		if (window->fractional_scale != NULL)
			wp_fractional_scale_v1_add_listener(window->fractional_scale,
							    &fractional_scale_listener,
							    window);
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
	window->viewport_stale = true;
	viewport_update(window);

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

// A compositor that goes away mid-run sets should_close, because there is no
// other way out: _poll cannot report anything and the caller is in a loop. That
// is a papering-over, not a design, and it is written up on card 004.
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
		window->viewport_stale = true;
	}
	viewport_update(window);

	window->focused = window->wanted_focused;
	window->suspended = window->wanted_suspended;
}

// Blocks on the socket the way pump looks at it, with a timeout. Anything
// already queued is dispatched instead of waited for, and a dead connection
// returns at once: _poll folds it into should_close.
void voe_platform_window_wait(voe_platform_window *window, double seconds)
{
	struct pollfd waiting = {
		.fd = wl_display_get_fd(window->display),
		.events = POLLIN,
	};
	int dispatched = 0;

	VOE_BASE_DEBUG_ASSERT(window != NULL, "waiting on a NULL window");

	if (wl_display_get_error(window->display) != 0)
		return;

	while (wl_display_prepare_read(window->display) != 0) {
		int count = wl_display_dispatch_pending(window->display);

		if (count < 0)
			return;
		dispatched += count;
	}

	wl_display_flush(window->display);

	if (dispatched == 0 &&
	    poll(&waiting, 1, seconds < 0.0 ? -1 : (int)(seconds * 1000.0)) > 0 &&
	    (waiting.revents & POLLIN))
		wl_display_read_events(window->display);
	else
		wl_display_cancel_read(window->display);
}

bool voe_platform_window_focused(voe_platform_window *window)
{
	VOE_BASE_DEBUG_ASSERT(window != NULL, "asking a NULL window");

	return window->focused;
}

bool voe_platform_window_visible(voe_platform_window *window)
{
	VOE_BASE_DEBUG_ASSERT(window != NULL, "asking a NULL window");

	return !window->suspended;
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

	return (voe_platform_size){
		voe_platform_scale_length(window->width, window->scale),
		voe_platform_scale_length(window->height, window->scale),
	};
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
