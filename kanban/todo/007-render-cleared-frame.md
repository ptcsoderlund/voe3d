# 007 — `render`: a cleared frame, on both platforms

claimed-by:
status: todo

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
