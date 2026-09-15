# 0040. Vulkan headers are vendored; the loader is opened at runtime; no SDK anywhere

- **Status:** Accepted
- **Date:** 2026-08-30
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

D-006, the last open pre-study exit criterion. Vulkan is normally consumed
through the LunarG SDK: headers, an import library, the loader, and the
validation layers, installed by the developer. The onboarding invariant says a
programmer clones and builds without assembling an environment, and ADR-0021
draws the line — **tools that transform source are installed; anything linked
or shipped is fetched.** The Vulkan SDK is not a compiler, so it falls on the
fetched side, and requiring its installation would break the invariant outright.
The register has carried this as the sharpest collision with the invariant since
Phase 0.

`spikes/vulkan-without-sdk/` answered the mechanism question on Linux. On a
machine with no SDK, no GPU and no Vulkan headers, built with a bare
`clang -std=c23 probe.c` — no `-l` flag, no fetch, no network — the probe opened
`libvulkan.so.1` via `dlopen`, resolved every entry point through
`vkGetInstanceProcAddr`, created an instance (1.4.341) and enumerated a device.
The loader was present because the graphics stack ships it, independently of the
SDK.

Two things the spike did not prove: the same approach on Windows, and anything
past device enumeration — no surface, no swapchain, no frame.

ADR-0030 already places this work: `render` is the only folder that names
Vulkan, and it depends on `platform` for the window. Card 004 built that seam —
`voe_platform_window_native()` returns the two OS handles as `uintptr_t`, so
`render` creates a `VkSurfaceKHR` without including an OS header and `platform`
never learns what Vulkan is.

## Options considered

### Option A — Require the Vulkan SDK
The ordinary path, and the one every tutorial assumes. Link against the import
library, include the installed headers, get validation layers for free.
Costs the onboarding invariant, which is a given and not a decision.

### Option B — Vendor headers, link the loader at build time
Headers in the tree, but still link `vulkan-1.lib` / `-lvulkan`. Removes the
header dependency but keeps a build-time library dependency, so the build fails
on a machine without the development package — the invariant again, softened but
not satisfied.

### Option C — Vendor headers, open the loader at runtime
Vendor the Vulkan headers into the tree, declarations only. At startup open
`vulkan-1.dll` / `libvulkan.so.1` by name, resolve `vkGetInstanceProcAddr`, and
resolve everything else through it. No `-l` flag, no import library, no
build-time Vulkan dependency of any kind.

## Decision

**Option C.** The deciding factor is that it leaves no build-time Vulkan
dependency at all, which is the only outcome that satisfies the onboarding
invariant completely rather than partially.

Consequently:

- **The Vulkan SDK is never required** — not by end users, not by developers
  building the engine, not by the check script.
- **The loader is found, not shipped.** It arrives with the GPU driver on both
  platforms. An end user who can run any Vulkan application already has it.
- **Validation layers are optional developer tooling**, exactly as ADR-0021
  classifies a tool. Present when someone has installed the SDK, absent
  otherwise; the engine runs identically either way and never requires them.
- **A missing loader is a recoverable failure**, not a crash and not an assert —
  the same shape as a window that will not open. A machine with no
  Vulkan-capable driver is a machine we cannot render on, and it is told so.

**Windows is unproven and is proven by code, not by a spike.** The principal's
decision: no third spike. The first `render` card writes the real loader on both
platforms, and a Windows failure surfaces to a human tester who is competent to
read it. The concern recorded once and not again: a Windows loader surprise then
arrives inside real code rather than a throwaway. Priced and accepted — the code
in question is one file and roughly thirty lines, and it fails loudly at startup
rather than subtly later.

**Exit criterion 4 is amended.** It read "a throwaway spike reaches a working
device + swapchain + cleared frame on both platforms." It now reads: **the first
`render` card reaches a working instance, device, surface, swapchain and cleared
frame in the dev window on both platforms.** The gate closes on real code. The
reason the spike was the right instrument in Phase 3 and is the wrong one now is
that `platform` did not exist then: a spike would have to rebuild a window it
would throw away, to avoid writing code we are about to write anyway.

## Blast radius

**Reversibility: cheap.** Reversing to Option A or B is deleting the runtime
resolution and adding a link line — one file in `render`, plus the tools list.
Nothing above `render` sees the difference, because no other folder names
Vulkan (ADR-0030). The vendored headers stay useful under every option.

The expensive direction would have been the opposite one: an SDK requirement
spreads into the check script, the tools list, both platforms' setup
instructions, and every contributor's machine, and is removed from all of them
only by hand.

## Consequences

- **Vendored Vulkan headers live in the tree**, unmodified, beside `render` —
  the same treatment ADR-0037 gives the Wayland protocol XML. Declarations only;
  they are not a dependency in ADR-0021's sense because nothing links against
  them.
- **Every Vulkan call in the engine goes through a resolved pointer.** No direct
  calls to Vulkan symbols, because there are none to link. This is a real
  constraint on how `render` is written and the first `render` card must state
  it.
- **The set of fetched dependencies remains empty.** ADR-0023 writes the parsers
  and this ADR vendors the headers, so nothing at all is fetched at configure
  time. The ADR-0021 line still stands for anything that arrives later.
- **`check.cmake` gains no Vulkan step.** There is nothing to check for: no
  tool, no library, no header outside the tree. Step 1 is unchanged.
- **We give up build-time type checking against the installed Vulkan version**
  and rely on the vendored headers being the truth. Acceptable: they are pinned
  in our tree, and the loader is versioned by `vkEnumerateInstanceVersion` at
  runtime, which is the check that actually matters.
- **A cleared frame is now a card, not a spike**, so `render` code exists before
  the phase gate formally closes. Consistent with ADR-0006, which already narrows
  the conventions gate to engine logic and lets Phase 3 run in parallel.

## Rejected options and why

- **Option A, require the SDK** — rejected on the given. The onboarding
  invariant is a constraint set by the principal, not a decision, and an option
  that violates it is rejected on that ground alone.
- **Option B, link the loader at build time** — rejected as the worst of both.
  It carries the full cost of vendoring headers while still failing to build on
  a machine without the Vulkan development package, so it buys nothing the
  invariant cares about.
- **A third spike for Windows and the swapchain** — the tech lead recommended a
  narrow version of this: a thirty-line Windows loader probe only, with the
  swapchain done as a card. **Rejected by the principal** in favour of no spike
  at all, trusting a human tester to diagnose a Windows failure. Recorded, not
  relitigated.

## Questions this opens

- **D-006 closed** by this ADR.
- **D-044 (new)** — how `render` holds the resolved function pointers: one
  global table, a struct passed down, or per-object. Decided against the first
  `render` card, not before.
- **D-008 (error handling) is now due.** `render` startup has three distinct
  recoverable failures — no loader, no Vulkan-capable device, no surface — and
  unlike card 004's single "did the window open," a caller may want to tell them
  apart. This is the real call site the register said to wait for.
- **D-032 unchanged.** No Vulkan entry is added to the required-tools list,
  which is the point of this ADR.
