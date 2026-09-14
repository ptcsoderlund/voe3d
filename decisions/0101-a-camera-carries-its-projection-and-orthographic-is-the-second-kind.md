# 0101. A camera carries its projection, and orthographic is the second kind

- **Status:** Accepted
- **Date:** 2026-09-09
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —
- **Closes:** D-181, and the orthographic half of D-178

## Context

The principal: *"We need to plan for orthogonal camera. 2d games need it and so do
a gui layer. But not all gui layers (VR)."*

The exception in his last sentence is the load-bearing part of this ADR, and it is
the thing an engine gets wrong by welding *orthographic* to *interface*.

**What exists.** `voe_3d_projection(camera, aspect)` is the only projection builder
in the engine and it builds perspective. `voe_scene_camera` carries an eye, a yaw,
a pitch, a vertical field of view, and a near and far plane in metres. **There is
no orthographic matrix anywhere**, in `math`, `3d` or `scene`.

Constraints already fixed:

- **ADR-0033 point 5 already decided the depth half of this**: *"Orthographic uses
  the same reverse-Z convention, even though a linear depth mapping gains no
  precision from it. Consistency is the point: one depth comparison operator and one
  clear value across the whole engine, with no pass that is the exception."* This ADR
  does not reopen it.
- **ADR-0033 point 4 said the default perspective has an infinite far plane, and the
  code has moved past it.** A camera carries a finite far today and
  `3d/src/projection.c` derives the infinite form as its limit — *"the finite form is
  the honest one and the infinite one falls out of it rather than having to be
  chosen."* Which matters here, because **an orthographic projection requires a
  finite far**: a linear depth range needs both ends.
- **`fov_y`'s own comment states the rule this ADR reuses**: *"Vertical field of
  view, radians. The horizontal one falls out of the aspect ratio, which is the
  render target's and not the camera's."*
- **ADR-0100 made the GUI obey the same rule** — scale from the height, width follows
  the aspect. So the engine will now say it in three places.
- **The draw system asserts exactly one camera in the world**, and both layers —
  world and overlay — are seen through it.
- **ADR-0010 — components are text-serialisable from day one**, so whatever shape the
  camera takes will appear in scene files later.

## The clarification that reframes the question

**A screen-filling GUI surface already has no camera and no perspective.**
`voe_render_element_transform` maps millimetres straight to clip space — *that is an
orthographic mapping*, arrived at by a different route, and ADR-0100 just settled
how its scale is derived. So *"a gui layer needs ortho"* is already true and already
built.

**And a GUI panel in the world must stay perspective** — a panel on a wall, a panel
floating in a room in VR. That is the principal's own exception, and ADR-0093
already puts such a panel in the world layer with a transform like anything else's.

So what is actually missing is narrower than the question sounds: **an orthographic
camera for the world.** A 2D game, whose entire scene is orthographic. An editor's
front or top viewport. Neither of those is a GUI at all.

## Options considered

### Option A — the camera carries a projection kind; one camera per frame stays

`voe_scene_camera` gains a kind — perspective or orthographic — and orthographic is
parameterised by **the view's height in metres**, with the width falling out of the
target's aspect exactly as the field of view's does. `voe_3d_projection` grows one
branch. Nothing about how a frame is assembled changes.

### Option B — many cameras now: a camera per layer, or per drawable

The draw system stops asserting one camera; a layer or a drawable names the camera
it is seen through; a frame can hold a perspective world and an orthographic
interface at once. This is the full thing, and it is what an editor with 2D gizmos
over a 3D scene eventually wants.

### Option C — an orthographic matrix only, with callers composing their own

Add the matrix somewhere and let a program build its own projection, as the element
path already does. The draw system keeps building perspective from the camera.

## Decision

**Option A, and the deciding factor is that it serves both of the principal's named
cases with one branch in one function** — because the GUI half of his question is
already answered by the element transform, and what is left needs a camera, and a
world has one camera.

Option B is where this goes eventually and Option A is deliberately shaped so that
getting there is an addition rather than a rewrite: the projection lives on the
camera either way, so a later *which camera does this layer use* question changes
the draw system and touches nothing about the camera component.

### What this pins

1. **A camera carries its projection kind.** Orthographic is parameterised by the
   **view height in metres**; the width falls out of the render target's aspect. That
   is the third place in this engine where the vertical dimension is authored and the
   horizontal one is derived — `fov_y`, ADR-0100's surfaces, and now this — and they
   are one rule, not three coincidences.
2. **Reverse-Z, `GREATER`, cleared to nought, for both kinds.** Near maps to 1 and
   far to 0, linearly for orthographic. ADR-0033 point 5, unchanged, and no pass is
   the exception.
3. **Orthographic requires a finite far plane**, which the camera already carries.
   ADR-0033 point 4's infinite default is a perspective-only property and should be
   read that way.
4. **One camera per frame stays.** Two projections in one frame is a separate
   decision — D-179 — and its trigger is the first program that needs *meshes* in
   both: an editor drawing 2D gizmos over a 3D scene, or a 2D game with one
   perspective element in it.
5. **The GUI is not coupled to orthographic, and this is the principal's VR
   exception made structural.** A panel's projection is a property of *where the
   panel is*, never of its being an interface. A screen-filling surface bypasses
   cameras entirely; a panel in the world uses whatever camera the world has, which
   in VR is emphatically a perspective one.
6. **How the fields are spelled is the card's**, on card 038's and card 033's
   precedent: a kind plus the parameters each kind reads, with the header saying
   which field is read when. The ADR fixes the parameterisation and the conventions,
   not the struct.
7. **Nothing is built until something draws through it.** Rule 10 and ADR-0083's
   guard: an orthographic camera with no caller is surface with no caller. **The
   trigger is the first orthographic viewport** — a 2D scene in `dev`, or the
   editor's first non-perspective view.

## Blast radius

**Moderate, and mostly about text.** The camera component is public and
text-serialisable, so its shape reaches scene files and anything that reads them.
Today it is built by hand in one program, so changing it costs a struct literal.

What stays cheap: the projection maths, which is one function with one branch and a
test that can assert both kinds with no graphics card.

Reversibility: **cheap now, moderate once a scene file holds a camera.**

## Consequences

- **A 2D game still goes through the whole 3D pipeline** — a depth buffer, the
  blended pass and its sort, the same shading. That is correct and it is not free,
  and it is worth saying plainly so nobody expects a *2D mode* that skips work. What
  they get is a projection.
- **Reverse-Z buys nothing in an orthographic projection** and is kept anyway.
  ADR-0033 already weighed that and chose one depth convention over a per-pass
  optimum; this is that decision being paid rather than a new cost.
- **The consequence I do not like:** a component with fields that only matter for one
  of its kinds — a field of view that means nothing orthographically, a view height
  that means nothing in perspective. A tagged union inside a text-serialisable
  component is worse, so the smell is accepted and the header is required to say
  which field is read when. If it ever becomes two components instead, that is a
  decision with a real trigger and not a tidy-up.
- **An editor still needs more than this.** A pixel-exact orthographic viewport, and
  a perspective scene view beside it in the same frame, are D-180 and D-179. This
  ADR is the floor they both stand on, not the whole building.

## Rejected options and why

**Option B — many cameras now.** It is the right destination and nothing today needs
it: no program in the tree draws meshes under two projections, and the draw system
asserting one camera is a fact one line long. Building the general form first would
be deciding how layers and cameras relate with no caller to test the answer against
— and layer identity is itself an open question (D-124).

**Option C — a matrix and no camera support.** It serves a program willing to build
its own projection and not the case the principal named: a 2D game's world is drawn
by the draw system, which builds the projection from the camera. It would also put a
second projection convention in reach of every caller, which is how a reverse-Z sign
gets flipped somewhere nobody is looking.

## Questions this opens

- **D-179 — whether a layer names its own camera**, so that a frame can hold a
  perspective world and an orthographic interface at once. Closed in principle by
  this ADR's point 4 as *not yet*; the row carries the trigger.
- **D-180 — whether an orthographic camera gets a pixel-exact mode**, its view height
  set from the target's pixel height so that one unit is one pixel. That is what an
  editor viewport wants, and it is the same question D-178 asks from the GUI side.
