# 0024. Nothing renders to the screen directly; 3D first, then text, 2D and GUI

- **Status:** Accepted
- **Date:** 2026-08-28
- **Deciders:** Human, Tech Lead

## Context

The principal's direction: 3D renderer first; when it is good enough, text, 2D
and a GUI renderer. Normally the order is reversed, but rendering GUIs *inside*
3D space is now common, so it is planned for from the beginning rather than
retrofitted.

The concrete requirement: **render to a texture, and display that on a surface.**

## Decision

**Render targets are first-class. Nothing in the engine assumes it is drawing to
the screen.** The 3D scene draws into an offscreen colour and depth target;
presenting that to a window is a separate, final step.

This is one architectural constraint taken now, and nothing more is designed.
Text, 2D and GUI are roadmap, not v1.

**Order:** 3D renderer → good enough → text → 2D → GUI.

## Why this one constraint is worth taking early

A renderer written against the swapchain is a renderer that must be taken apart
to render anywhere else. Everything below falls out of the same seam:

- A GUI drawn into a texture and mapped onto a quad in the world — the case that
  motivated this.
- A GUI drawn into a texture and composited flat over the frame.
- Post-processing, tone mapping, bloom, anti-aliasing — all listed as *later*
  and all requiring the frame to exist as a texture first.
- Rendering at a different resolution than the window.
- Shadow maps, reflection probes, thumbnails, and an editor viewport inside a
  panel — the editor being wanted eventually per ADR-0009.

The cost now is close to zero: with Vulkan 1.3 dynamic rendering (ADR-0018)
drawing to an offscreen target is no harder than drawing to a swapchain image.
The cost of retrofitting it is a renderer rewrite.

## Consequences

- **Presentation is one small piece of `render`, not its shape.** The final blit
  or composite is the only code that knows a swapchain exists.
- The swapchain is not the depth buffer's owner, not the resolution authority,
  and not what the frame graph is built around.
- Window resize stops being a special case that reaches through the renderer —
  it resizes targets, which is local.
- A second render target costs memory. Accepted, and unnoticeable at v1 scale.
- **Text, 2D and GUI are explicitly not designed here.** They are prepared for by
  this one decision and otherwise deferred, per the standing rule against
  designing for consumers that do not exist.

## Questions this opens

None now. When text arrives it brings font rasterisation and atlas questions
with it, which under ADR-0023 will be written rather than imported.
