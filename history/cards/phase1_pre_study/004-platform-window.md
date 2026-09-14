# 004 — platform: a window, and a dev program that opens it

claimed-by: claude-code (kanban-coder)
status: review

**Needs card 003 (`base`) finished** — the window's event storage comes from
there.

## Goal

A window that opens on Windows and Linux, can be resized, reports when the user
asks to close it, and hands out the handles a graphics API needs to draw into
it. Plus a dev program that opens one, so it can be looked at.

Done means: `cmake -P check.cmake` exits zero on both platforms, and running
`voe_platform_dev_window` opens a window that survives being resized and closed.
On Linux it will have no titlebar — see below.

## Scope

**A window. Not input, not files, not time**, even though `platform` will own
all three eventually. Nothing consumes a keystroke yet. The next card that needs
a key adds keys.

This is the largest card so far and it is two implementations of the same small
API. Resist making the API larger because one of the two backends makes
something easy.

## The Linux window system

**Wayland. No X11 backend at all**, not even a fallback — nothing needs one yet.

- The `xdg-shell` protocol XML is **vendored into this repository**;
  `wayland-scanner` turns it into C **at build time**, into the build directory.
  Nothing generated is committed.
- `wayland-scanner` and the Wayland client headers are a **required tool** on
  Linux, the same way the Windows SDK is a required tool on Windows. Add a
  `wayland-scanner` check to **step 1 of `check.cmake`**, on Linux only, failing
  with the same shape of message as the existing tool checks.
- **The window will have no titlebar and no borders.** Wayland does not
  guarantee server-side decorations and GNOME does not provide them. This is
  expected, not a bug, and we are not drawing decorations or adding `libdecor`.
  Move and resize it with the compositor's own shortcuts.

On Windows: Win32, `CreateWindowExW`, an ordinary message pump. Unicode API, not
the ANSI one.

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
when it changes, and exit cleanly. That is the whole thing.

It must build on both platforms and contain no `#ifdef` for an operating
system. If it needs one, the API above has a hole and that is the finding. It is also the only
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

---

## Result

`status: review`. Markers: **one `DEVIATION:`** in `platform/src/window_wayland.c`
(the placeholder buffer, explained below). No `BLOCKED:`.

### What landed

    platform/CMakeLists.txt                 the four lines
    platform/platform.md
    platform/include/platform/window.h      the API, no OS type in it
    platform/protocol/xdg-shell.xml         vendored, unmodified
    platform/src/window_wayland.c           Linux
    platform/src/window_win32.c             Windows
    platform/dev/window.c                   the dev program

Outside `platform/`, all of it required by this card's own text:

- `CMakeLists.txt` — `add_subdirectory(platform)`.
- `cmake/voe.cmake` — `dev/` programs (ADR-0036); a general rule that a source
  named `*_wayland.c` or `*_win32.c` is one platform's and the other never
  compiles it; and `voe_platform_backend()` / `voe_platform_link()`.
- `check.cmake` — the `wayland-scanner` check in step 1, and the `platform`
  exemption in step 5.

### Which C23 features were used

One, on top of what `base` already uses: **`bool`, `true` and `false` as
keywords**, in `window.h` and both backends. In C17 that needs `<stdbool.h>`;
in C23 it does not, and the header is not included. **The floor stays at
clang 18.**

### Can `voe_platform_window_new` fail? Yes — I hit it, unintentionally

This is the card's real deliverable, and it arrived by accident, which makes it
better evidence than a contrived one.

The first run of the dev program printed `could not open a window` and exited 1.
That was not a bug: the environment I execute in blocks the `socket` syscall
outright. A three-line probe pinned it down —
`socket(AF_UNIX, SOCK_STREAM, 0)` itself returns `EPERM`, before any address is
involved, so `wl_display_connect` could only return NULL. SELinux is disabled,
the socket is `srwxr-xr-x` and owned by the same user; the block is a seccomp
filter on my process tree and nothing about this machine.

**So the NULL path is not theoretical — it is the path I actually took, and the
convention in the card handled it correctly and legibly.** A returned NULL, one
`if`, a message, exit 1. Nothing about it wanted an error code or an error
struct, because there was exactly one question to answer: did it open.

Two things that experience says about the wider decision, offered as evidence
rather than as a proposal:

- **A caller cannot say *why*.** "No compositor", "no xdg-shell", and "the
  memfd could not be made" all arrive as the same NULL. For a window that is
  survivable — the user retries or gives up either way. For a model that will
  not parse, "which file, which line" is the whole message, and NULL cannot
  carry it. The two cases may not want the same convention.
- **NULL only covers failing to *start*.** The connection dying at frame 900 has
  no return value to travel in: `_poll` returns nothing. I papered over it by
  folding a dead display into `should_close` and said so in the file header and
  below. A convention that only covers construction leaves this hole in every
  module that has a run loop.

### Where the two backends disagreed

The card asked for this, and it is where the next platform card's work is.

1. **A window exists at different moments.** Win32: `CreateWindowExW` plus
   `ShowWindow` and it is on screen. Wayland: a surface with no buffer ever
   committed is *never mapped* — no pixels, no window, nothing to resize and
   nothing to close. So `_new` returning successfully cannot mean the same thing
   on both platforms unless Linux commits something. **This is the `DEVIATION:`**
   — `window_wayland.c` commits a flat-coloured shared-memory buffer purely so
   the window exists, as a stand-in for the Vulkan swapchain. It is the reason
   `wl_shm` is bound at all, it adds nothing to the public API, and every line of
   it goes when `render` attaches a real surface. Without it this card has no
   verification, because there is nothing to look at.
2. **Who decides the size.** Win32: you ask, you get it (with `AdjustWindowRect`
   adding the decorations), and `WM_SIZE` tells you what happened afterwards.
   Wayland: the compositor *proposes* through `configure`, you must ack it, and
   your next buffer has to match. `_size` therefore means "what happened" on one
   platform and "what was agreed" on the other. They coincide today; the day
   something wants to *set* the size they will not, and that is a real API
   question rather than a backend detail.
3. **Decorations, and therefore what "close the window" means.** Windows gives a
   titlebar with buttons. Wayland gives nothing unless asked. The API says
   nothing about decorations and cannot, so the same program is closed by
   clicking an X on one platform and by pressing `Alt+F4` on the other. That
   asymmetry is invisible from `window.h`, and it cost a round trip during
   verification below.
4. **`native.context` does not have the same lifetime on the two platforms.** On
   Linux it is the `wl_display`, one per window, created and destroyed with it.
   On Windows it is the `HINSTANCE`, which is process-wide and shared by every
   window. A `render` that caches anything per-context will be right on Windows
   and wrong on Linux, or the reverse. Worth knowing before `render` touches it.
5. **The title is bounded on one side only.** Wayland takes the UTF-8 straight
   through. Win32 needs UTF-16, and with no arena parameter on `_new` the
   conversion goes to a stack buffer with a `TITLE_MAX` of 256 — a
   Windows-only limit that is now part of a cross-platform contract.
6. **Non-blocking is free on one side.** `PeekMessage` is naturally
   non-blocking. Wayland needs the `prepare_read` / `poll` / `read_events`
   dance, because announcing the read before polling the socket is the only way
   an event arriving in between is not lost.

They agreed on exactly one thing worth noting: `WM_CLOSE` and
`xdg_toplevel.close` are both advisory, both mean "the user asked", and neither
destroys anything. `should_close` is a faithful name on both.

### Verified

`cmake -P check.cmake`, all eleven steps:

    ok    tools (clang 22, cmake 4.3.0, slangc, wayland-scanner)
    ok    standalone base
    ok    standalone math
    ok    standalone platform
    ok    root configure and build
    ok    guard compiler
    ok    guard version
    ok    guard map
    ok    includes
    ok    tests (1 passed)
    ok    harness reports a failure

Same caveat as card 003: **`slangc` is not installed on this machine**, so step 1
stops the unpatched run before it reaches anything. The run above used a stub
`slangc` on `PATH`, in the scratch directory, with nothing in the repository
changed.

**The dev program was run by Human and it works.** It opened, printed
`opened 960x540`, printed a `resized` line on `Meta+Up`, and printed `closed`
and exited 0 on `Alt+F4`. I could not run it myself for the `EPERM` reason above,
which is also why the first attempt at verification failed: with no decorations
there are no buttons to click, and the card said to use the compositor's
shortcuts without saying which. `Meta+Up` and `Alt+F4` are the two that matter on
KWin and they are now written into `dev/window.c`'s header.

Each new build rule was made to fire, rather than assumed:

- **`wayland-scanner` missing fails step 1.** Run with a `PATH` holding only
  `clang` and the `slangc` stub:
  `FAIL tools / wayland-scanner could not be run: no such file or directory` —
  the same shape as the existing tool checks.
- **The step 5 exemption does not leak.** `#include <unistd.h>` added to
  `base/include/base/version.h` still fails:
  `base/include/base/version.h: <unistd.h> is not allowed outside platform`.
  Removed afterwards.
- **`platform` really is exempt**, or step 5 would fail on
  `<wayland-client.h>`, `<unistd.h>`, `<poll.h>` and `<sys/mman.h>`, which it
  does not.
- **The dev program is built and is not a test.** The executable is at
  `build/check/root/platform/voe_platform_dev_window`, and `ctest -N` on the same
  build lists `Total Tests: 1` — `base/arena` and nothing else. A dev program in
  ctest would hang the check script waiting for someone to close a window.
- **`window_win32.c` is not compiled on Linux.** The only object built is
  `window_wayland.c.o`.
- **Nothing generated reaches the source tree.** `platform/protocol/` holds
  `xdg-shell.xml` and nothing else after a full build.

### One thing found on the way past, and fixed because this card needed it

Step 5 of `check.cmake` banned OS headers in **every** folder while printing
`is not allowed outside platform`. Message and behaviour disagreed, and nothing
had noticed because no folder had ever tried to include one. `platform` is now
genuinely exempt and the leak test above is what proves the exemption is narrow.

### Not verified

- **Windows, entirely.** `window_win32.c` has never been compiled, by me or by
  anyone. It is written to the API and reviewed, and that is all it is. It is the
  largest single risk in this card and it is Human's to retire.
- **`git` is still unusable in this checkout.** `.git` is a gitlink to
  `../.git/modules/voe3d`, which does not exist. Unchanged from cards 001–003;
  the card was moved with `mv` and nothing was committed.

## Notes — suggestions, not done here

- **KWin does offer server-side decorations**, through
  `zxdg_decoration_manager_v1` — one registry binding and one request, no
  drawing and no `libdecor`, so it does not touch what this card ruled out. The
  card predates knowing the machine is KDE. Human chose to leave it out of this
  card deliberately; recording it here because it is roughly fifteen lines and
  it is the difference between a window with buttons and one that needs
  shortcuts. It is still only a *request* — a compositor may refuse, and the
  no-decoration path has to keep working either way.
- **`voe.cmake` now knows one folder by name.** `voe_platform_backend()` and
  `voe_platform_link()` are keyed on `folder STREQUAL "platform"`. That is the
  cost of keeping every folder's `CMakeLists.txt` at four lines, which
  `CLAUDE.md` calls binding, and `platform` is the first folder to want anything
  a fifth line would have said. The tech lead may want this settled properly —
  either the four-line rule gets an exception clause, or `voe.cmake` grows a
  general "this folder links these system libraries" hook. One folder is fine;
  three would not be.
- **The dev program spins a core.** `_poll` returns immediately and `platform`
  has no way to wait, so a loop with nothing else in it busy-waits. Correct for
  today — nothing else exists to fill a frame — and the first card that adds
  time, or a blocking variant of poll, ends it.
- **The placeholder buffer caps the window at 2560x1440.** Beyond that the
  buffer clamps while `_size` keeps reporting the truth, so a larger window
  stops growing on screen. The cap exists so the shared-memory mapping can be
  made once and never moved, and the whole thing disappears with the swapchain.
