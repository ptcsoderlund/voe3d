# 0203 — A selection outline is the silhouette drawn as this frame's geometry in the overlay layer
date: 2026-09-20
by: planner

## Decision
What marks the selected entity in a scene view is its silhouette, built on the CPU every frame and drawn as
this frame's geometry. `3d` walks the shape's edges (0202's store), keeps the ones whose two triangles face the
eye differently — that set is the silhouette, exactly the outline the drawn triangles have — and turns each
into a quad standing in the world along that edge, as many pixels wide on the picture as it is asked for
whatever the distance, facing the eye. Those quads go through `voe_render_geometry_create_transient` and one
`voe_render_frame_draw`, in the overlay layer, after `voe_3d_draw_system_run`'s depth clear: that is what makes
the outline show through whatever stands in front of the entity, while the entity itself stays where it is in
the picture. The record they wear is an unlit white one, so the colour the draw carries is the whole of what
they are.

**Nothing is added to `render`.** No fourth pipeline, no stencil, no second target, no readback.

**The colour is the palette's, and it is the lighter of `inverse` and `inverse_ink`.** A scene view's background
is the engine's near-black clear colour whatever the theme is (`render/src/frame.c`), so a light theme's
`inverse` — a dark fill — would be an outline nobody can see against it. Both roles carry the theme's one hue
and neither is a colour of its own, so taking whichever of the two has the greater luminance keeps 0194's rule
and keeps the outline visible in every theme.

## Reasoning
- **The classic expanded shell** — a hull pushed out along its normals, front faces culled, masked by the
  entity's own depth — is the cheap way to do this on a card, and it is rejected here because this engine's
  GPU layer offers neither of the two states it needs: there is no front-face-culling pipeline and no draw that
  writes depth without writing colour. Both are new pipelines in `render`, and each of the four files they
  touch — `device.c`, `device_internal.h`, `frame.c` and the folder's one public header — is over 800 lines and
  would have to be split first. That is a rebuild of the engine's most load-bearing folder to mark a selection.
- **Drawing the selected entity itself in the overlay, with a shell round it** — rejected: it needs no new
  pipeline, but the entity then draws in front of whatever is really in front of it. The feature asks for an
  outline that shows through, not for an object that jumps forward.
- **A mask target and an edge-detect pass over it** — rejected for the same reason as the shell: a fourth
  pipeline and a fourth shader.
- **A bounding box drawn in wireframe** — rejected by the feature: it asks for the silhouette.
- The per-frame cost is one walk over one shape's edges per view — 18 for a cube, about 1,500 for a capsule —
  and a copy of at most a few hundred quads into the frame's transient pool. The walk is capped so a mesh
  nobody has measured cannot fill the pool.

## Replaces
Nothing. It reads 0194 and 0196 for the colour and ADR-0191 for the shapes.
