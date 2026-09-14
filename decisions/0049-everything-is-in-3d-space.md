# 0049. Everything is in 3D space; there is no screen-space 2D path

- **Status:** Accepted
- **Date:** 2026-09-01
- **Deciders:** Human, Tech Lead
- **Amends:** ADR-0024 (pins the interpretation of its central requirement)
- **Superseded by:** —

## Context

ADR-0024 recorded the principal's requirement as *"render to a texture, and
display that on a surface"*, and stated the order 3D → text → 2D → GUI.

**That sentence has two readings and the tech lead took the wrong one.** On
2026-09-01, while proposing the 3D roadmap, the tech lead described the next
card as "draw into a texture, then put that on screen" — meaning the whole frame
becomes an offscreen image which is then blitted to the window. The principal
corrected it:

> *"I dont think we have the same interpretation of 'Draw directly to screen'. I
> was thinking about 2d and text here. Everything should be in 3d Space. 2D games
> have to do billboarding sprites as textures, text renders to textures."*

"Surface" meant a surface **in the scene**, not the window's `VkSurfaceKHR`. The
requirement is about where *content* lives, not about where the *frame* goes.

Both readings appear in ADR-0024 — its *Why this one constraint is worth taking
early* section lists post-processing, resolution independence and editor
viewports, which are all consequences of the frame-as-texture reading. That is
why the ambiguity survived: every sentence in the ADR was true under either
reading, so nothing collided until a card had to be written.

This ADR pins the reading. ADR-0024 is not edited; it is append-only.

## Decision

**There is one rendering space, and it is 3D.** No part of the engine gets a
screen-space or orthographic-overlay drawing path.

- **Text** rasterises into a texture and is drawn on geometry in the world.
- **2D and sprites** are billboarded quads in 3D space, sampling textures.
- **A GUI panel** is a quad — placed in the world, or locked to the camera. It is
  still a quad, in the same space, going through the same renderer.
- `text`, `sprite` and `ui` therefore **produce textures and 3D geometry**. They
  do not get their own path to the framebuffer.

The deciding factor is the principal's original intent, stated plainly, and the
fact that GUI-inside-3D-space was the case ADR-0024 was written to protect in the
first place — a separate screen-space path is the thing that makes it impossible.

## Blast radius

**Cheap today, extremely expensive later, and that asymmetry is the point.**
`text`, `sprite` and `ui` do not exist yet, so deciding this now costs nothing.
Deciding it after a screen-space text renderer exists costs that renderer.

Reversibility: **load-bearing.** Adding a screen-space path later is not a
feature addition — it is a second renderer, with its own coordinate conventions,
its own depth handling and its own reason to disagree with the first one.

## Consequences

- **One renderer, not four.** `text`, `sprite` and `ui` are content producers on
  top of `3d`, not sibling renderers racing to the same framebuffer. This is a
  tighter reading of ADR-0030's "each on `render`, low level" than the tech lead
  had been working to.
- **Text cannot precede the camera.** A camera-locked quad needs transforms and a
  camera to exist. This fixes the roadmap order more firmly than taste did: the
  camera card genuinely blocks the text card.
- **Debug text is not a cheap shortcut.** Any on-screen text goes through the
  texture-and-quad path like everything else. There is no quick screen-space
  overlay to reach for, which is a cost paid deliberately.
- **Everything is subject to the depth buffer, the reversed-depth setup and the
  single viewport Y flip** (ADR-0033). A UI quad is not exempt from any of them.
  Ordering UI above the world becomes a depth and pass question, not a separate
  code path — and it is a real question when it arrives.
- **Transparency moves up in importance.** Text and UI quads are alpha-blended by
  nature, and ADR-0024's capability list has transparency as *later*. The first
  text card will want it. Noted, not decided.
- Two mechanisms hide behind the phrase "text renders to textures" and they are
  not the same: a **glyph atlas** (rasterise on the CPU, upload once, draw quads
  sampling it — no render-to-texture pass) and a **panel rendered offscreen**
  (draw widgets into a target, map it onto a quad — a render-to-texture pass).
  This ADR permits both; which is used is the text card's decision.

## Rejected options and why

**A screen-space overlay pass for text and UI** — the conventional answer, and
what nearly every engine does. Rejected because it defeats the case ADR-0024
exists for: a GUI mapped onto a quad in the world. An engine with both paths
maintains two, and the screen-space one always becomes the default because it is
easier, which quietly makes the 3D-space case second-class.

## Questions this opens

- **Whether the frame itself is also an offscreen target, presented as a separate
  final step** — the tech lead's misreading, which is a real and separate
  question. It is not answered by this ADR either way. Register row D-053.
- How UI ordering above the world is achieved without a separate path — a depth
  and pass question, when the first UI card arrives.
- Whether transparency must move from *later* into v1 to support text.
