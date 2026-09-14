# 0033. Coordinate, matrix and depth conventions

- **Status:** Accepted
- **Date:** 2026-08-30
- **Deciders:** Human, Tech Lead
- **Amended by:** ADR-0035 — storage order is row-major (`m[row][column]`), matching Slang. §2's *semantics* — column-vector, `M·v`, translation in the last column — are unchanged, as is everything else here.
- **Supersedes:** —
- **Superseded by:** —

## Context

D-039. `math` cannot be written without it: `look_at`, `perspective`,
`orthographic` and `from_trs` each encode a handedness, an up axis, a matrix
order and a depth range, and a coder guessing any one of them produces code that
compiles, passes a weak test, and is wrong in a way that surfaces months later
as a mirrored or z-fighting world.

It is also the one place where three fixed things disagree with each other by
default:

- **glTF 2.0** (ADR-0010, import-only, parser written by us per ADR-0023) is
  right-handed, Y-up, −Z forward, with column-major matrices.
- **Vulkan clip space** (ADR-0018, 1.3 baseline) has +Y **down** and depth
  0..1 — unlike OpenGL, which is +Y up and −1..1.
- **`text`, `sprite` and `ui`** (ADR-0030) are screen-space renderers and are
  unambiguously X-right, Y-up.

Something has to reconcile them, exactly once, in a named place.

The decision is not technical in its main axis. Y-up and Z-up cost the same
number of instructions and roughly twenty lines of code. It is an interop and
ergonomics decision, and it was taken as one.

## Options considered

### Option A — Right-handed, Y-up, −Z forward
glTF's convention, and Bevy's, Godot's, and that of essentially all graphics
reference material. The importer performs no coordinate conversion. "Up" means
the same axis in the 3D world and in the three screen-space renderers.

Costs: the ground plane is `v.xz`, not `v.xy`, so terrain, navigation, spatial
hashing and any top-down maths lose a natural swizzle. Authoring in Blender
(Z-up) means a mental swap when reading coordinates off a panel — though
Blender's glTF exporter already converts on the way out, so no data is affected.

### Option B — Right-handed, Z-up
Unreal's convention, Blender's, and that of CAD, GIS, physics and robotics. The
ground plane is `v.xy`. Matches how the principal authors.

Costs: a coordinate conversion on every glTF import — not one matrix multiply
but a conversion that must be applied consistently to node transforms, camera
nodes, animation channels, morph targets, tangent frames and skinning matrices,
and whose failure mode is applying it twice or to five of six things. A
permanent disagreement about "up" between the 3D world and the three
screen-space renderers. And constant friction with reference material, which is
overwhelmingly Y-up.

### Option C — Configurable
Rejected without elaboration: an engine that cannot assume its own up axis must
write every piece of code axis-agnostically, which is the premature-generality
anti-pattern this project has declined six times.

## Decision

**Option A, with the package that follows from it.**

Deciding factor, named by the principal's choice and the argument that carried
it: `text`, `sprite` and `ui` are on the roadmap and are Y-up by nature, and an
engine in which "up" means Y in four folders and Z in five is a tax paid
forever.

Fixed by this ADR:

1. **Right-handed. +X right, +Y up, −Z forward.** The camera looks down its own
   −Z. Identical to glTF, so the importer performs no coordinate conversion —
   and *is not permitted to*, since the absence of a conversion is the property
   being bought.
2. **Column-vector, column-major.** Transforms compose as `M * v`; a combined
   transform reads right-to-left, `P * V * M`; translation lives in the last
   column. This matches glTF's storage, GLSL, and the maths literature.
3. **Reverse-Z, depth 0..1.** The near plane maps to depth **1.0** and the far
   plane to **0.0**. Float depth buffer, `GREATER` comparison, depth cleared to
   0. Floating point concentrates its precision near zero; reverse-Z lines that
   up with the far distance, where a conventional depth buffer wastes it. Every
   current engine — Unreal, Unity, Godot 4, Bevy — does this.
4. **The default perspective has an infinite far plane.**
   `voe_math_mat4_perspective(fov_y, aspect, near)` is reverse-Z with no far
   plane, which is numerically well-conditioned in this arrangement and removes
   a tuning knob from every camera. `voe_math_mat4_perspective_far(...)` exists
   for the cases that genuinely need one, such as shadow cascades.
5. **Orthographic uses the same reverse-Z convention**, even though a linear
   depth mapping gains no precision from it. Consistency is the point: one depth
   comparison operator and one clear value across the whole engine, with no
   pass that is the exception.
6. **Exactly one Y flip exists in the engine, and it lives in the viewport.**
   Vulkan's clip space is Y-down; our world and our maths are Y-up. That
   reconciliation is done with a **negative viewport height** (core Vulkan since
   1.1; we baseline 1.3), **not** by negating the second row of the projection
   matrix. The projection matrix therefore stays the textbook one, which is what
   keeps `math` free of any knowledge of Vulkan.
7. **The front-face winding constant is determined empirically and proven by a
   test.** Flipping Y anywhere reverses apparent triangle winding. The rule is
   the invariant, not the sign: there is one flip, it is in the viewport, and
   `render` sets its front-face constant to match it and pins that with a test
   that draws a known triangle and asserts it is visible. Getting this wrong by
   flipping twice is the classic form of this bug and it is invisible until
   backface culling is switched on.
8. **Minor, and free-standing:** angles are radians in every API — degrees exist
   only inside `voe_math_degrees` and `voe_math_radians`. Units are metres and
   seconds, matching glTF. Recorded here because they share this surface; neither
   is load-bearing.

## Blast radius

**Reversibility: load-bearing.** This is the most expensive decision in the
pre-study to reverse. Changing the up axis or the handedness later touches every
axis constant, every camera, every test's expected numbers, every authored asset
and every shader that reasons about direction. Changing the matrix order inverts
every multiplication in the codebase. Changing the depth convention touches every
depth comparison, every clear, every shadow pass and every shader that reads
depth.

Point 3 is the one taken *now* specifically because it is nearly free now and a
rewrite later — the same shape as ADR-0024's render-to-texture constraint.

## Consequences

- **The ground plane is `v.xz`.** Terrain, navigation, spatial hashing and
  top-down maths lose the free `.xy` swizzle they would have had under Z-up.
  This is a genuine, permanent, daily cost in any folder that grows those
  features, and it was accepted with the trade understood.
- **Authoring in Blender means a mental swap.** Blender is Z-up; its glTF
  exporter converts on the way out, so no data is wrong — but a coordinate read
  off a Blender panel is not a coordinate to type into a debug camera.
- **The glTF importer must not convert coordinates.** This is now a rule, not an
  omission. A future card that adds a conversion "to fix" an orientation is
  fixing the wrong thing and must be rejected.
- **Reverse-Z means the intuitive test is inverted.** "Near maps to 0" is wrong
  here and will be written that way by anyone working from habit. The `math`
  card states it explicitly and tests it.
- **Depth format is constrained.** Reverse-Z is only worth having with a float
  depth buffer; a normalised integer depth format cancels the benefit. `render`
  uses `D32_SFLOAT`. That is a `render` decision this ADR pre-empts, and it is
  stated so the two folders cannot disagree.
- **`math` stays free of Vulkan.** Point 6 is what buys that: the projection
  matrix is the textbook one and the API-specific flip lives in the one folder
  allowed to name Vulkan.
- **Generated and copied code will fight point 3 and point 6.** Reference
  material and model output default to non-reversed depth and to negating the
  projection. Both are wrong here, and both compile.

## Rejected options and why

- **Option B (Z-up)** — its one concrete win, the `.xy` ground plane, does not
  outweigh a conversion layer on every glTF import whose failure mode is silent,
  plus a permanent split between the 3D world and three screen-space renderers.
- **Option C (configurable)** — an interface with one configuration, forcing
  every folder to be written against an axis it may not assume.
- **Negating the projection matrix to flip Y** — the common Vulkan workaround.
  Rejected because it puts API-specific knowledge inside `math`, and because it
  makes the projection matrix differ from every reference implementation, which
  is precisely the kind of silent divergence this ADR exists to prevent.
- **Conventional depth (near 0, far 1)** — the default everywhere, and strictly
  worse, for no saving. Cheap now, a rewrite later.
- **Left-handed** — Direct3D heritage, no benefit here, and it disagrees with
  glTF and with every cross product in the literature.

## Questions this opens

- **Closes D-039.** The `math` card is unblocked.
- **Constrains `render` before it is written:** `D32_SFLOAT`, `GREATER`, clear
  to 0, negative viewport height, and a winding test. These belong to D-035 and
  are recorded there so the first `render` card inherits them.
- Nothing new is opened. This decision removes questions rather than adding them.
