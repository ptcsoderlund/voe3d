# 007 — `render`: a cleared frame, on both platforms

claimed-by: claude-opus-5 (kanban-coder)
status: review

**Needs cards 004 and 005 finished.** Both are. This is the first `render` code
and the first Vulkan in the engine.

**This card closes the pre-study.** It is the last open exit criterion, and it
is the biggest card written so far. Read all of it before starting, and read
`CLAUDE.md` rules 8, 10, 11, 13 and 14 and the *Givens* section — three of those
rules were decided for this card and have never been used.

## Goal

`voe_dev` opens its window and the window is filled with a solid colour that the
graphics card put there. Resize it and it stays filled. Close it and nothing
leaks.

Done means: `cmake -P check.cmake` exits zero on both platforms, `voe_dev` shows
a cleared window on both, and the card reports what the loader found on each.

## The one constraint that shapes everything

**Nothing links against Vulkan.** There is no `-l` flag, no import library, and
no SDK. The headers are vendored into this tree as declarations only; the loader
ships with the graphics driver and is opened by name at runtime:

    Linux     libvulkan.so.1     dlopen / dlsym
    Windows   vulkan-1.dll       LoadLibraryA / GetProcAddress

From that library resolve exactly one symbol, `vkGetInstanceProcAddr`, and
resolve **everything else through it** — instance functions from the instance,
device functions from the device once it exists.

**So define `VK_NO_PROTOTYPES` before including any Vulkan header.** Without it
the headers declare prototypes, the code links against symbols that are not
there, and the failure is at link time with a confusing message. This is the
single most likely way to lose an hour on this card.

**The function-pointer table is `render`'s alone.** Resolved once at startup,
never reassigned, never passed as a parameter, never stored in a component. Say
that in the header of the file that owns it, and say why: it is the engine's one
deliberate exception to a rule we otherwise hold, and the next reader needs to
know it was a decision.

### Vendoring the headers

Take `vulkan_core.h` and whatever it needs from the Khronos `Vulkan-Headers`
repository, unmodified, under `render/`. Unlike the Wayland protocol files these
**are** committed — they are source we carry, not something generated at build
time. Record the version you took in the file header or a note beside them.

## The trap this card has, stated up front

Creating a surface needs a struct that names OS types —
`VkWaylandSurfaceCreateInfoKHR` wants a `wl_display *` and a `wl_surface *`,
`VkWin32SurfaceCreateInfoKHR` wants an `HINSTANCE` and an `HWND`. The Vulkan
headers get those by including `wayland-client.h` and `windows.h`.

**`render` may not include an OS header.** `platform` is the only OS-aware
folder, `check.cmake` step 5 greps for exactly this, and it will fail you.

`platform` already hands you what you need — `voe_platform_window_native()`
returns the two handles as `uintptr_t`, deliberately, for this. The question is
only how to name the types in the create-info struct without pulling in the OS
header behind them.

**Work out how, and report what you did.** I have not prescribed it because I
am not confident which way is cleanest until someone has both platforms in front
of them, and a wrong prescription here is worse than none. If every route is
ugly, say so and say which ugly you picked — that is a finding, not a failure.
If you conclude the rule itself is wrong, `BLOCKED:` it rather than working
around it.

## What to do

Roughly in this order, because each step gives you something to look at:

1. **Open the loader** and resolve `vkGetInstanceProcAddr`.
2. **Create an instance.** Enable the surface extensions you need —
   `VK_KHR_surface` plus the platform one.
3. **Turn on validation if it is there.** If `VK_LAYER_KHRONOS_validation` is
   present, enable it in debug builds and attach a debug messenger that prints.
   **If it is absent, carry on silently** — no warning, no error. It comes with
   the Vulkan SDK, which nobody is required to have. This is worth doing early
   and not last: it is the difference between a clear error and a black window.
4. **Create the surface**, from `voe_platform_window_native()`.
5. **Pick a physical device.** Require Vulkan 1.3 and a queue family that can
   both render and present to that surface. If more than one qualifies, prefer a
   discrete GPU; if none qualifies, that is a failure to report, not an assert.
6. **Create the device and get the queue.**
7. **Create the swapchain**, sized from `voe_platform_window_size()`.
8. **Clear and present, every frame**, driven by the loop already in
   `dev/src/main.c` beside `voe_platform_window_poll()`.

Use **dynamic rendering** (`vkCmdBeginRendering`) and **synchronization2**. Both
are core in 1.3 and both exist to delete the boilerplate that the older way
needs — no render pass objects, no framebuffers. Do not write the old style.

### Resize is not optional, and card 006 is why

A swapchain that is not recreated on resize breaks the moment the window
changes, and on this project that is not hypothetical: **card 006 measured KWin
resizing the client area by the titlebar's height when decorations are toggled**,
with no other signal that anything happened. `voe_platform_window_size()` is the
truth. Handle `VK_ERROR_OUT_OF_DATE_KHR` and `VK_SUBOPTIMAL_KHR`, and rebuild.

A minimised or zero-sized window must not crash and must not spin — skip the
frame.

### Delete the placeholder buffer — you are authorised to edit `platform`

`platform/src/window_wayland.c` carries a shared-memory buffer that exists only
so the surface has something to show. Card 004 said it disappears with the
swapchain. **It disappears now.** It also caps the window at 2560x1440, which
would otherwise become your bug.

This card names two folders, `render` and `platform`, which is deliberate and is
the only reason it is allowed — do not read it as licence to touch a third.

**Ordering trap:** a Wayland surface with no buffer attached is **not mapped at
all** — the window simply does not appear. So the swapchain must be providing
the buffer before the placeholder goes, and if you remove it too early you get
an invisible window and no error. If the two cannot be cleanly separated, say
so.

## Error handling — you are the first user of rule 13

`CLAUDE.md` rule 13 was decided for this card. `voe_base_error` does not exist
yet. **You create it, in `base`, with exactly the codes this card needs and no
others** (rule 10). Codes are **categories, not incidents**: which category it
was goes in the enum, what exactly happened goes in the message at the site.

Startup has at least three genuinely different failures — no loader, no device
meeting the 1.3 requirement, no surface — so this is the "several ways" case:
return `NULL` and take a `voe_base_error *error` beside it. Add
`voe_base_error_string()`. `[[nodiscard]]` on everything that can fail.

**A missing loader is a returned failure, not an assert and not a crash.** A
machine with no Vulkan driver is a machine we cannot draw on, and it gets told
so. Running out of memory stays fatal, through rule 11.

If three codes turn out to be the wrong number — more, or fewer — say so and use
the right number. The card is not the authority on that; the failures you
actually hit are.

## Scope — do not exceed this

**No triangle. No shaders. No `slangc` in this card at all.** A clear needs no
pipeline.

Also not now: depth buffer, descriptor arrays, render-to-texture, any component
or ECS type, any resource-id scheme, frames in flight beyond what presenting
needs, and any abstraction over Vulkan. There is one graphics API and there will
never be another one — an interface with one implementation is the thing
`CLAUDE.md` rule 10 exists to prevent.

**Clear the swapchain image directly.** Drawing to an offscreen target and
copying is the engine's eventual shape and it is *not* this card — it arrives
with the first `3d` work, which is where it earns its keep.

`render`'s public surface should be close to nothing: enough for `dev` to start
it, draw a frame and shut it down. Its real shape gets designed against `3d`,
which does not exist. Resist inventing it now.

## Tests

**Two, and they are worth having.** `render/tests/` — the maths shipped with
none and it is still a gap; do not repeat it.

1. **The loader opens and an instance is created.** No window, no surface, no
   device. This asserts the whole no-SDK claim, and it runs on a machine with no
   GPU — the spike proved exactly this against a software driver.
2. **Instance version is reported and is sane.** `vkEnumerateInstanceVersion`
   returns something ≥ 1.0 and the call goes through a resolved pointer.

If a test cannot run where there is no driver at all, it must **skip cleanly and
say so**, not fail. A machine without Vulkan is not a broken checkout.

The frame itself is looked at, not asserted on, same as cards 004 and 006.

## Report back in this card

- **What the loader found, on each platform separately.** Library name, whether
  `vkGetInstanceProcAddr` resolved, instance version, and the device you got.
  Windows has never been tested — this is the card that proves or breaks
  ADR-0040, and the Windows half is its real deliverable.
- **Whether validation layers were present**, on each machine, and what they
  said if anything.
- **How you named the OS types in the create-info structs** without including an
  OS header. Expected to be the fiddliest part of the card.
- **What happened to the placeholder buffer**, and whether removing it and
  gaining the swapchain could be done cleanly or had to be one step.
- **Which error codes you ended up with**, and whether three was right.
- **Anything the two platforms disagreed about** that `render` had to paper
  over. That is where the next card's work is.

---

# Report

## What the loader found

**Linux (Fedora 44, KWin session, this machine).** `libvulkan.so.1` opened with
`dlopen`, `vkGetInstanceProcAddr` resolved from it, instance version reported as
**1.4.341**. Nothing is linked against Vulkan: `ldd` on `voe_dev` shows no
`libvulkan`, there is no `-l` flag anywhere, and no SDK is installed.

**The device it picks on the human's session:**

    render     NVIDIA GeForce RTX 4070 Laptop GPU, Vulkan 1.4.341, queue family 0

The discrete preference works, and one queue family does both graphics and
present. The coder's own session cannot reach that card at all — it sees only
**`llvmpipe (LLVM 22.1.8, 256 bits)`, Vulkan 1.4.354, device type CPU**, Mesa's
software rasteriser, which qualifies under the card's rule as written (it claims
1.3 and has a graphics queue) and would lose to any discrete card. So the same
tree picks a software device in one session and the right GPU in another, which
is the selection rule doing exactly its job.

`vkCreateDevice` succeeded on it with `synchronization2` and `dynamicRendering`
enabled and `VK_KHR_swapchain` on, and **every one of the 27 device-level
function names in the table resolved**. So the table is not merely spelled
correctly, it is answered.

**Windows: nothing. Not compiled, not run, not seen.** `backend_win32.c`,
`library_win32.c` and the seven typedefs in the first of those have never been
through a compiler. This is the card's real deliverable and it is the half I
cannot deliver — ADR-0040 is proven on Linux and untested on Windows.

## Validation layers

**Present on this machine, and enabled.** `VK_LAYER_KHRONOS_validation` 1.4.341
is installed (`/usr/share/vulkan/explicit_layer.d/`) along with
`VK_EXT_debug_utils`, and an instance created with both enabled returns
`VK_SUCCESS`.

**They said nothing across a full run** — open, two swapchain rebuilds, close —
on the NVIDIA driver. No warning, no error, and nothing at `vkDestroyInstance`
about an object left behind, which is the layer's own leak report and the closest
thing this card has to the Goal's "close it and nothing leaks".

One thing to confirm: that requires the run to have been the **Debug** binary,
`build/debug/dev/voe_dev`. A Release build turns validation off by design and
would be silent for a different reason.

The messenger's own formatting is therefore still unread — a clean run never
reaches it.

The absent case is the one that is exercised least and matters most for the
onboarding claim: it is handled by `wants_validation()` returning false and
nothing else changing, and it is silent by construction — there is no branch that
can warn.

## How the OS types were named — the trap

**Wayland: nothing had to be done.** `VkWaylandSurfaceCreateInfoKHR` spells its
fields `struct wl_display *` and `struct wl_surface *`, and C declares a struct
tag it has not seen on the spot — an incomplete type is all a pointer needs. So
`vulkan_wayland.h` compiles with no `wayland-client.h` behind it, and the
`uintptr_t` handles cast straight in. The header only pulls in the window system
when it is reached through `vulkan.h`, which nothing here includes.

**Windows: seven typedefs, in `backend_win32.c`, and this is the ugly one.**
`vulkan_win32.h` names `DWORD`, `HANDLE`, `HINSTANCE`, `HMONITOR`, `HWND`,
`LPCWSTR` and `SECURITY_ATTRIBUTES` and declares none of them. Four of the seven
are only there because the header also carries the external-memory extensions,
which nothing in this engine touches. They are declared with the SDK's own
spellings — `DECLARE_HANDLE(x)` is `typedef struct x##__ *x`, so naming the same
tags produces the same types rather than lookalikes, and `SECURITY_ATTRIBUTES` is
only ever pointed at so an incomplete type is enough.

**Both routes are ugly and this is the less ugly one.** The alternative was not
vendoring `vulkan_win32.h` and writing `VkWin32SurfaceCreateInfoKHR` out by hand.
That trades seven typedefs whose definitions are fixed by a published ABI for a
hand-copied API struct that has to be kept in step with a header already in the
tree — a silent field-order bug waiting for the next header bump. **It is a
finding, not a failure, and it is written up in the file's header where whoever
meets it will be standing.**

The rule itself is not wrong and is not `BLOCKED:`. It cost seven lines.

## `BLOCKED:` — no. Two scope findings instead

**`BLOCKED: platform` was the alternative and I did not take it.** Opening a
library by name is `dlfcn.h` and `windows.h`, both banned outside `platform` by
`check.cmake` step 5, so `render` cannot open its own loader. The card authorises
editing `platform`, so `platform` grew a small module rather than the card
stopping:

    platform/include/platform/library.h
    platform/src/library_wayland.c        dlopen / dlsym
    platform/src/library_win32.c          LoadLibraryA / GetProcAddress

Three functions, an opaque handle that is the OS's own handle rather than
anything allocated, and no knowledge of Vulkan: **the caller passes the library's
name**, because a library's name is knowledge about that library and not about
the operating system. This is an addition to `platform`'s public surface that the
card did not name, and it is the biggest judgement call in the card. If it should
have been a `BLOCKED:` and its own card, say so and it moves.

**The card names two folders and the work needed four.** `dev/src/main.c` and
`dev/CMakeLists.txt` had to change — the Goal is "`voe_dev` opens its window and
the window is filled", which cannot be reached without editing `dev` — and the
root `CMakeLists.txt` needed `add_subdirectory(render)` or the folder is not in
the build at all. Both are the card's own Goal contradicting its "do not read it
as licence to touch a third". Nothing else was touched.

## What happened to the placeholder buffer

**Gone, in one step, and the ordering trap did not bite** — because it runs the
other way round from the way the card feared. `platform` never had to hand the
buffer over to anything; it simply stopped attaching one, and the Vulkan
swapchain's first present is now what maps the surface. There was no window in
between where two things were attaching buffers and no window where neither was.

Removed: `wl_shm` and its binding, the `memfd`, the `mmap`, the pool, the
`wl_buffer` churn, `present()`, `clamp_to()`, `pool_open()`, and with them
`<sys/mman.h>`, `<unistd.h>` and the **2560x1440 cap**. `#define _GNU_SOURCE`
stayed, and now carries a comment saying why: it is what makes `<poll.h>` declare
`poll()` under `-std=c23`. It is a feature-test macro, not a GNU extension.

**The two could not be verified separately here and that is the honest answer.**
Removing the buffer and gaining the swapchain is one commit because there is no
compositor in this session to look at the state in between — see *Not verified*.

## Which error codes, and was three right

**Three was right. They are not the three the card named.**

    VOE_BASE_OK = 0
    VOE_BASE_ERROR_UNAVAILABLE    not on this machine at all
    VOE_BASE_ERROR_UNSUPPORTED    here, and cannot do what we require
    VOE_BASE_ERROR_REFUSED        here, capable, and the call failed anyway

The card suggested "no loader, no device meeting 1.3, no surface". The first two
are categories and survived as `UNAVAILABLE` and `UNSUPPORTED`. **"No surface" is
an incident, not a category** — it is one of six ways the driver can refuse
during startup, alongside the instance, the device, the format, the frame objects
and the swapchain, and a caller can do exactly the same thing about all six. It
folded into `REFUSED` and the detail went into the line printed at the site, which
is what rule 13 asks for.

None of the three names Vulkan, a window or a graphics card, which was deliberate:
`base` must be able to keep them when `assets` fails to open a file.

## What the two platforms disagreed about, and what render papers over

- **The surface's extent.** Wayland answers `0xFFFFFFFF` to
  `currentExtent` — always — meaning the client chooses; Win32 answers the real
  size and Vulkan then *requires* the swapchain to match it. `swapchain.c` takes
  the surface's answer where there is one and the window's size where there is
  not. That is why the device keeps both `extent` and `built`: they are not
  always the same number, and comparing the wrong one rebuilds the swapchain
  every frame forever.
- **Who maps the window.** On Wayland nothing is on screen until a buffer is
  attached, so `voe_dev` shows nothing at all until the first present. On Windows
  the window is on screen the moment it is created and the first present merely
  changes its colour. **A failure to present is invisible on one platform and
  obvious on the other.**
- **The loader's file name and the surface extension**, which is what
  `backend.h` exists for. Three functions, two implementations, and **no `#ifdef`
  anywhere in `render`**.

## Verified

**The Goal holds, on Linux, looked at by a person.** `voe_dev` opens and the
window is filled with the clear colour, corner to corner. Closing it prints
`closed` and exits zero.

**Card 006's case is the one that got exercised, and it behaved.** Toggling the
frame off and on printed exactly what card 006 measured and nothing else:

    render     NVIDIA GeForce RTX 4070 Laptop GPU, Vulkan 1.4.341, queue family 0
    opened     960x540
    decorated  yes
    size       970x567
    size       960x540
    closed

960x540 decorated, 970x567 with the frame gone, and back — the compositor handing
over the titlebar's space and taking it again. **No `decorated` line either time**,
which is card 006's finding standing up: KWin re-answers SERVER_SIDE in both
states. So the swapchain was rebuilt twice, from nothing but a size that
disagreed with `built`, with no event and no other signal that anything had
happened. That is the whole reason `built` is a separate field from `extent`, and
it is the case the card said was not hypothetical.

`cmake -P check.cmake`, every step ok:

    ok    tools (clang 22, cmake 4.3.0, slangc, wayland-scanner)
    ok    standalone base / dev / math / platform / render
    ok    root configure and build
    ok    guard compiler / version / map
    ok    includes
    ok    tests (2 passed)
    ok    harness reports a failure

Same standing caveat as cards 003–006: **`slangc` is still not installed**, so
the run used a stub on `PATH` from the scratch directory, with nothing in the
repository changed. No shader is compiled by this card, so the stub is not
covering for anything the card needed.

Also verified, on Linux:

- **Debug and Release both build clean** at `-Wall -Wextra -Wpedantic -Werror`.
  Release caught a real thing: wrapping `wants_validation`'s body in `#ifdef
  NDEBUG` left two functions nothing called. It is a constant now, so both calls
  stay compiled and type-checked in every build.
- **`clang --analyze` is clean** on every file this card touched. It was run by
  hand — see the finding below.
- **`render` configures standalone**, and its four-line `CMakeLists.txt` needed
  no exception for the vendored headers: `vk_video/` sits inside `vulkan/` so
  `vulkan_core.h`'s quoted include resolves without an include path.
- **`render/loader` passes**, and it is the whole no-SDK claim: dlopen, resolve,
  report 1.4.341, create an instance, destroy it, and see the table emptied.
- **The startup path up to the surface**, by a scratch program outside the
  repository: instance with `VK_KHR_surface` + `VK_KHR_wayland_surface` +
  validation, the instance function pass, physical-device enumeration, device
  creation with the 1.3 features, and the device function pass. All ok. That
  program is deleted.

## Not verified — this is the part that matters

- **Windows, entirely.** See above. This is now the only untouched half of the
  card.
- **An arbitrary resize.** The two rebuilds below came from the decoration
  toggle, which changes the size in one step. Dragging a corner rebuilds the
  swapchain many times a second and goes through
  `VK_ERROR_OUT_OF_DATE_KHR` / `VK_SUBOPTIMAL_KHR` on the way, which the one-step
  case need not; that path is written and unexercised.
- **Validation has never printed.** A clean run never reaches the messenger, so
  if its format string is wrong the first person to see a real validation error
  sees it wrong.
- **A minimised window skips the frame but the loop still spins.** `render`
  returns true and draws nothing, which is what the card asked for; the spin is
  `platform`'s, because `_poll` returns immediately and `platform` has no way to
  wait. The card's "must not spin" is half-met and the other half is not this
  folder's to fix.
- **`git` is still unusable in this checkout** — `.git` is a gitlink to a
  `../.git/modules/voe3d` that does not exist. The card was moved with `mv` and
  nothing was committed.

## `DEVIATION:` and `BLOCKED:` markers left in code

**None.** The two judgement calls above are in this report and in the file
headers that carry them, not as markers.

## Notes — suggestions, not done here

- **`check.cmake` has no step 7.** `CLAUDE.md` rule 8 says the run "includes step
  6 (tests) and step 7 (`clang --analyze`)"; the script stops at 6b and never
  invokes the analyser. So the analyser ran by hand on this card and will not run
  on the next one unless someone remembers. Either the script grows the step or
  the rule stops claiming it — the rule and the script disagree today and the
  script is the one that runs.
- **The `_wayland` / `_win32` suffix is a window-system word doing a platform's
  job.** `platform/src/library_wayland.c` has nothing to do with Wayland; it is
  `dlopen`, and it carries that name only because `voe.cmake` recognises exactly
  two suffixes. A `_linux` / `_windows` pair, or both, would let a file be named
  for what it is. It is a change to `cmake/voe.cmake` and belongs in its own card.
- **`render` does not `DEPENDS math` and should not yet.** Nothing here has a
  vector: the clear colour is three floats in `frame.c`. The row in the map allows
  it the day something needs it (rule 10).
- **Two tests, one file.** Rule 12 is one test program per module and both of the
  card's tests are the loader's, so they are two checks in
  `render/tests/loader.c`. If two files were meant, it is a one-line split.
- **`VOE_RENDER_MAX_IMAGES` is 8 and a driver over it is refused, not clamped.**
  Clamping would leave images an acquire can still return and no view to draw
  into. If a real driver ever asks for more, the fix is a bigger number, not a
  clamp.
- **`render` prints one line on success** — the card it picked, its Vulkan version
  and its queue family — because the card asks for the device to be reported.
  It is the only thing this folder prints that is not a failure, and it is the
  first candidate to go when something better than `printf` exists.
- **The public surface is one object and it will not stay one.** `device.h` says
  so in its header: the split into instance / device / swapchain / target is the
  card that arrives with `3d`, not a gap in this one.
