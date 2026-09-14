# 0107. The frame belongs to a window, the device belongs to the graphics card

- **Status:** Accepted
- **Date:** 2026-09-10
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —
- **Closes:** D-199. Opens D-200, D-201.

## Context

The principal asked whether the engine is good for multi-screen support. **Three
different things go by that name and the engine stands differently on each.**

1. **One window dragged onto another monitor.** Works, and works for a good reason
   rather than by luck: ADR-0104 made the interface scale with the window's height
   and **nothing in the engine reads a display at all**, so a second monitor is a
   resize and the swapchain already handles those. The one known cost is D-188 —
   neither backend is density-aware, so a scaled display gets a stretched picture
   rather than a sharper one. Text goes soft, not wrong-sized.
2. **Fullscreen on a chosen monitor.** Absent entirely. `voe_platform_window_new`
   takes a width, a height and a title: no fullscreen, no monitor list, no position.
   **Additive, and there was no register row for it** — see D-200.
3. **Two windows at once, one per screen.** Structurally blocked, and this ADR is
   about that.

**Why it is blocked.** `struct voe_render_device` is one object that owns the
graphics card *and* one window's presentation: the Vulkan instance, physical device,
logical device and queue, the descriptor pool and the bindless texture array — and
also `VkSurfaceKHR surface`, `VkSwapchainKHR swapchain`, the swapchain images, the
offscreen targets sized to that window, the frame slots and their command pool, the
built-for size, and the present mode. A second window today means a second Vulkan
instance and a second copy of every resource, which is not something we would ship.

**The good half, and it is why this decision is cheap.** Everything worth sharing is
already on the correct side of the line, by decisions taken for other reasons.
ADR-0018 made GPU resources referenced by id and ADR-0060 kept `render`'s public
surface by-id and grown on demand, so meshes, textures and materials belong to the
device and two windows would share them for free. **The only entangled thing is the
frame.**

**And the cut is already half-reasoned.** `voe_render_device_new_headless` exists —
a device with no window, no surface and no swapchain — and its `headless` flag
carries a comment naming *"the four places where a surface would otherwise be asked
a question: which instance and device extensions to enable, which queue families can
present, and where the format comes from"*. Somebody has already enumerated the
questions a surface answers. That is the cut's own map, written for another purpose.

## Where the line falls, stated once so nobody re-derives it

Eighteen public calls take a `voe_render_device`. They divide cleanly:

**The window's side — becomes a surface (13):** `frame_begin`, `frame_end`,
`frame_draw`, `frame_draw_blended`, `frame_draw_elements`, `frame_submit_element`,
`frame_clear_depth`, `frame_is_open`, `frame_draw_count`, `frame_elements_submitted`,
`frame_gpu_time`, `present_set`, `present_get`.

**The card's side — stays on the device (5):** `device_destroy`, `geometry_create`,
`texture_create`, `texture_destroy`, `shading_create`.

**One that reads device-shaped and is not:** `voe_render_geometry_create_transient`.
Its signature takes a device, but ADR-0084 gave it one set per frame in flight, reset
at the top of every frame — **that is frame lifetime, so it follows the window.** It
is exactly the kind of thing that would have been missed on the day of the cut, which
is the argument for writing this down now rather than then.

## Options considered

### Option A — cut now
One mechanical card: thirteen signatures, `dev`, and the cards in flight rewritten.
Makes a second window a small card later. Costs work with no caller today.

### Option B — name the seam, keep the trigger
Write the line down, forbid new `render` surface from quietly assuming one window,
and take the cut on the first card that wants a second window.

### Option C — decide there is never more than one window
A second screen is served by fullscreen on a chosen monitor and nothing else.

## Decision

**Option B, the principal choosing, with the tech lead's recommendation.**

1. **The line is named and it is the one above.** The device is the graphics card and
   what lives on it; the frame is a window's. This ADR is the record of which call is
   which, including the transient-geometry trap.
2. **No new public `render` call may bake in that there is exactly one surface**, and
   the way that is kept honest is that **a new public call in `render` says in its
   header which side of the line it is on.** One sentence per call, written by the
   card that adds it.
3. **The cut is not made.** Nothing is built, no card is written, and the existing
   calls keep their signatures.
4. **Resources need nothing.** ADR-0018 and ADR-0060 already put them on the device
   side; this ADR only records that it was load-bearing for a reason nobody had in
   mind at the time.
5. **Fullscreen and monitor enumeration are not this decision.** They are additive
   `platform` surface, they break nothing when they arrive, and they get D-200 rather
   than being smuggled in here.

**The deciding factor:** everything expensive is already shared correctly, so waiting
costs only a rename whose price is capped by how narrow the frame API deliberately
is — and the caller that would justify cutting now is the editor, which is not
designed yet (D-060). Cutting today means guessing what it wants.

## Blast radius

**The cut's price today is thirteen signatures plus `dev`.** It grows by one for each
new frame-side call. That is the meter this decision starts running, and it is stated
so the day it is paid nobody is surprised by the number.

Reversibility: **cheap now, moderate later.** The seam is a rename and a struct split,
not a redesign, for as long as the frame API stays narrow.

## Consequences

- **Two windows also means two views in one frame**, which is the same machinery
  D-182 needs for a camera's picture inside the scene and D-179 needs for meshes
  under two projections: the view stops being a property of the frame and becomes a
  property of a pass. **Whichever of those three arrives first pays for the others**,
  and none of them is scheduled.
- **The price grows quietly.** Nothing announces that a frame-side call was added, so
  the meter above is read by whoever next looks. Point 2's header sentence is what
  makes it readable at all.
- **The consequence I do not like:** point 2 is a convention nothing checks. This
  project has six times replaced a rule someone must remember with a rule the machine
  checks, and there is no machine check here — a call's side is a question about what
  it means, not about how it is spelled. It is the same weaker instrument ADR-0105
  accepted, for the same reason, and D-201 carries it rather than letting it sit
  unremarked.
- **The editor is the likely trigger and it may arrive at a bad moment** — tearing a
  panel onto a second monitor is how people work. If it lands mid-phase the cut
  competes with the editor's own first card. Naming the trigger now is what makes
  that a scheduling problem rather than a discovery.

## Rejected options and why

**Option A — cut now.** Rejected as premature generality by this project's own
standard: a surface abstraction with one implementation, built for a caller that does
not exist. The engine has refused exactly this shape repeatedly (ADR-0083, rule 10),
and the refusal is not weaker because the tech lead sympathised with the work.

**Option C — never more than one window.** Rejected because it closes a door on the
editor for a saving that is not needed. Nothing about the engine requires the
restriction; it would be a decision to be unable to do something, taken before
anything asked.

## Questions this opens

- **D-200** — fullscreen, and choosing which monitor a window opens on.
- **D-201** — that point 2 has no machine check.
