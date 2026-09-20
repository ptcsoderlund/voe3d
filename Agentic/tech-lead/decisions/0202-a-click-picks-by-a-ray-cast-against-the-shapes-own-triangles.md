# 0202 — A click picks by a ray cast on the CPU against the shape's own triangles
date: 2026-09-20
by: planner

## Decision
Clicking in a scene view names an entity on the CPU, not on the card. The pointer's place in the picture
becomes a ray — the inverse of the same projection and view matrices the pass was opened with, applied to the
near and far points of that pixel — and `3d` answers which entity the ray meets first by testing every shaped
entity's own triangles in its own space: the entity's world matrix inverted takes the ray into the shape's
space, and the built-in shape's vertices and indices, kept on the CPU beside the ones that went to the card,
are walked triangle by triangle. The nearest hit in front of the eye wins; a ray that meets nothing answers a
zeroed entity, which is what clears the selection.

The triangles are kept because they are built anyway: `3d/src/cube.h`, `capsule.h` and `cylinder.h` already
produce them at startup for the upload, and one arena copy per kind — three kinds, about 35 KB — serves both
the ray and 0203's outline. The same store keeps, per kind, the surface's edges welded by position with the
two triangle normals that meet along each, which is what a silhouette is walked over.

Only shaped entities are picked, and that is the whole of what an editor's project can draw: a mesh and a
material are runtime-only components (`3d/mesh_component.h`), a scene file holds described components alone
(`authoring/scene_write.h`), and nothing in the editor imports a model. The mesh table's turn comes with the
card that gives the editor an import: the same ray against the triangles the importer read, kept the same way.

## Reasoning
- **An id drawn into a picture and read back** — rejected. It is exact by construction, but a target's colour
  image is an sRGB format (`render/src/target.c`), so a 24-bit entity number does not survive the encode; the
  read waits for the card to go idle and may not be called inside a frame (`voe_render_target_read`), so a
  click would cost a stalled extra frame; and it needs a pass, a pipeline or a material path that exists for
  no other reason.
- **A box or a bounding volume per entity** — rejected on the feature's own test: a click on the visible sliver
  of a shape standing behind another has to pick the one the pointer is over, and two boxes overlapping on
  screen answer the wrong one. Triangles are the only test that agrees with the picture.
- **Analytic volumes per kind — a box, a capsule, a cylinder** — rejected: three pieces of geometry arithmetic
  to write and test instead of one ray-triangle test, they disagree with the drawn tessellation at the
  silhouette, and they are no use to the outline, which needs the triangles anyway.
- **Keeping the triangles in the mesh component** — rejected for now: it would change `voe_3d_mesh_add`'s
  surface and every call site of it for a case the editor cannot reach, against rule 10. A kind is what a
  saved scene holds (ADR-0191), so a kind is what the store is keyed by.

## Replaces
Nothing.
