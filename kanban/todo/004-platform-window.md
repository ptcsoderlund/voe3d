# 004 — platform: a window, and a dev program that opens it

claimed-by:
status: todo

**Needs card 003 (`base`) finished** — the window's event storage comes from
there.

## Goal

A window that opens on Windows and Linux, can be resized, reports when the user
asks to close it, and hands out the handles a graphics API needs to draw into
it. Plus a dev program that opens one, so it can be looked at.

Done means: `cmake -P check.cmake` exits zero on both platforms, and running
`voe_platform_dev_window` opens a window that survives being dragged, resized
and closed.

## Scope

**A window. Not input, not files, not time**, even though `platform` will own
all three eventually. Nothing consumes a keystroke yet. The next card that needs
a key adds keys.

This is the largest card so far and it is two implementations of the same small
API. Resist making the API larger because one of the two backends makes
something easy.

## The Linux window system

<!-- FILL THIS IN BEFORE STARTING — X11 or Wayland. See the tech lead. -->

## Constraints

- `platform` depends on `base` only.
- **`platform` is the only folder allowed to include an OS header.** The check
  script enforces this. Everything below the API line is invisible to callers.
- **The public headers must not name an OS type.** No `HWND`, no
  `xcb_window_t`, no `struct wl_surface` in `include/platform/`. A caller that
  cannot compile without `<windows.h>` breaks the rule for everyone.
- No `**`. Allocating functions return the pointer.
- Failing to allocate is fatal (`base`). Failing to *open a window* is not the
  same thing — see below.
- Every name is `voe_platform_*`.

## The API

Keep it to what is needed to put a picture in a window:

```
voe_platform_window_new(width, height, title)   -> window, caller owns it
voe_platform_window_destroy(window)
voe_platform_window_poll(window)                 drain the OS event queue
voe_platform_window_should_close(window)         set when the user asked
voe_platform_window_size(window)                 current client area
voe_platform_window_native(window)               handles, for the graphics API
```

- The window is long-lived and individually owned, so it is `new`/`destroy`, not
  arena memory. Any per-poll scratch it needs comes from an arena passed in.
- `poll` is called once per frame and returns nothing. It updates the window's
  own state; asking for that state is a separate call. Do not invent an event
  queue, a callback, or a listener — nothing consumes events yet.

**`_native` is the interesting one.** A graphics API needs the OS handles, but
`render` may not include an OS header. So return a small plain struct of two
`uintptr_t` — on Windows the module instance and the window handle, on Linux
whatever the chosen backend's pair is — and document in the header comment
exactly which is which per platform. The caller casts. This is deliberately
crude, and crude is the right answer: the alternative is `platform` knowing what
Vulkan is, which would put the module map the wrong way round.

## Failure

Opening a window can genuinely fail — no display, no compositor, a driver
without the extension. **That is not an assert.** It is the first recoverable
failure in this engine, and how those are reported is not decided yet.

So: **do not invent an error convention.** Have `voe_platform_window_new` return
`NULL` on failure, have the dev program print a message and exit non-zero, and
**write on this card that you hit it**. That is the real deliverable of this
section — a concrete call site to decide the error convention against, instead
of deciding it in the abstract.

## The dev program

`platform/dev/window.c`, per ADR-0036: one file, its own `main`, built
automatically, never run by the check script.

It should open a window, loop on `poll` until `should_close`, print the size
when it changes, and exit cleanly. That is the whole thing. It is also the only
verification this card has, which is the point of it existing.

## Tests

**There are none, deliberately.** Opening a window needs a display and a user;
a test that asserts a window opened either needs a desktop session or lies. The
dev program is the verification, and a person runs it.

If you find something in here that *is* unit-testable without a display — a size
calculation, a handle-packing helper — test that. Do not manufacture a test for
the window itself.

## Report back in this card

- Which C23 features you used.
- Whether `voe_platform_window_new` can fail on your machine, and how you found
  out. This feeds the error-handling decision.
- Anything the two backends disagreed about that the API had to paper over. That
  disagreement is where the next platform card's work is.
