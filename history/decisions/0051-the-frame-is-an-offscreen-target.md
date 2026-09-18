# 0051. The frame is drawn into an offscreen target and copied to the window

- **Status:** Accepted
- **Date:** 2026-09-01
- **Deciders:** Human, Tech Lead
- **Amends:** ADR-0024 (supplies the frame-level half its wording was ambiguous about)
- **Superseded by:** —

## Context

ADR-0024's central sentence — *"render to a texture, and display that on a
surface"* — had two readings. ADR-0049 pinned the one the principal meant:
everything is content in 3D space, and there is no screen-space path. The other
reading was the tech lead's, and it turned out to be a separate real question
that ADR-0049 explicitly declined to answer: **does the frame itself go into an
offscreen image which is then copied to the window, or does the renderer draw
straight into the window's image?**

Today it draws straight into the window's image. The cleared-frame and triangle
cards both acquire a swapchain image and render into it.

Everything this enables is on the principal's *later* capability list and nothing
on the v1 list requires it: post-processing, tone mapping, bloom,
anti-aliasing, rendering at a resolution different from the window, shadow maps,
reflection probes, thumbnails, and an editor viewport inside a panel.

Two things changed the cost since ADR-0024 was written. **ADR-0050** established
that every one-frame-lifetime resource lives in a per-slot array, which is the
plumbing an offscreen target needs. **ADR-0049** requires rendering into textures
for UI panels regardless, so the machinery is not optional work — only its
application to the final frame is.

## Decision

**Yes, and early.** The 3D scene draws into an offscreen colour target — and its
depth target — and copying or compositing that into the acquired swapchain image
is a separate final step. The offscreen target is a per-frame-slot resource under
ADR-0050.

The deciding factor is that this is the only item on the renderer roadmap that is
a rewrite rather than an addition: a renderer written against the swapchain must
be taken apart to render anywhere else, and every item in the list above needs it
first.

## Blast radius

**This is the decision being taken specifically to avoid a blast radius**, so its
own is the point: taken now, it is one target, one copy and one card, while
`render` is five source files. Taken after model loading, materials and lighting
exist, it is those files plus every pass they added.

Reversibility: **load-bearing but benign** — reversing it means deleting the
target and rendering to the swapchain again, which nothing else depends on. The
asymmetry is entirely in the cost of *adding* it late.

## Consequences

- **Presentation becomes one small piece of `render`, not its shape** — exactly
  what ADR-0024 predicted. The final copy is the only code that knows a swapchain
  exists.
- **The swapchain stops being the resolution authority and stops owning depth.**
  Window resize resizes targets, which is local, instead of reaching through the
  renderer.
- **The offscreen target exists once per frame slot**, not once — ADR-0050 point 2.
  Two colour targets and two depth targets at the default of two frames in
  flight. Memory cost accepted; unnoticeable at v1 scale, and ADR-0024 already
  accepted the same trade for one.
- **The copy is a real decision the card must make and state**: a blit
  (`vkCmdBlitImage`, filters and rescales) or a draw of a full-screen quad
  sampling the target (needed the moment tone mapping or any post-process
  arrives). The card starts with the blit and says why, because nothing yet
  post-processes.
- **The target's format need not be the swapchain's**, and eventually should not
  be — a float or higher-precision target is the reason tone mapping is possible
  at all. Not chosen now; the card matches the swapchain format and states that
  it is a starting point, not a conclusion.
- **It gives the engine its first readback path**, which is how the front-face
  and Y-flip pinning test becomes possible at all. The test ADR-0033 asks for and
  the triangle card marked as a deviation lands with this card as its own proof of
  correctness.
- The window's image is still acquired, transitioned and presented exactly as
  today. This card does not touch synchronisation beyond adding the copy.

## Rejected options and why

**Draw straight into the window's image and add the target when something needs
it.** Simpler today, and the honest argument for it is that nothing on the v1
list needs it. Rejected because "when something needs it" is after the renderer
has grown, and this is the one item where waiting converts an addition into a
rewrite. ADR-0034's *implement on demand* rule explicitly does not cover this:
it binds implementation, not decisions, and carves out the case where taking a
decision early changes what existing code looks like.

## Questions this opens

- **Blit versus full-screen-quad composite**, decided in favour of blit for now
  and revisited when the first post-process arrives.
- **The offscreen target's format and precision**, revisited with tone mapping.
- Whether the depth target is per frame slot or shared. It is per slot under
  ADR-0050 unless the card finds a reason it cannot be, which would be a finding.
