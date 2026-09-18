# 0163 A built-in shape is a described component in `3d`

Status: accepted
Date: 2026-09-15

Spec 004 saves a scene holding a cube and opens it again as saved. What an entity draws is
`3d`'s mesh and material components, both runtime-only (ADR-0150) because they hold GPU ids — so
a saved scene reloads its cube with no shape, which is D-259 (how a component names an asset). A
path-to-a-model answer needs an asset resolver nobody has asked for; the spec needs exactly one
shape to survive a save.

## Decision

**`3d` gains `voe_3d_shape`, a described component with one read-only `UINT32` field, `kind`**,
where `1` is the cube and no other value is defined yet. It is saved because it is described
(ADR-0150) and shown, not edited, in the inspector (ADR-0139). Its typed creation call is how
code that knows its types gives an entity one.

**`3d` gains the shapes' GPU side**: one call uploads every built-in shape's geometry and its
plain grey material once at startup into a value the program keeps, with the vertex, index,
geometry and shading counts a device must have room for published as constants; and a shape
system run adds a mesh and a material, through `3d`'s own creation calls, to every entity with a
shape and no mesh. A kind it does not know is a warning, edge-triggered as other drains are, and
draws nothing.

**The cube's vertices move into `3d`.** `editor/src/cube.c` goes; `dev` keeps its own copy until
something there wants the shape.

D-259 stays open for assets named by path; this answers it for shapes the engine builds itself.

## Rejected

- An editor-registered component naming the shape — an editor-only component is stripped by the
  cook (ADR-0126), and a game's scene needs its cube too.
- Describing the mesh component — it holds a GPU geometry id, which is meaningless in a file.
- A path to a `.glb` in the project — an asset resolver, a project-relative path rule and an
  importer call on load, for one cube.
- A colour field beside `kind` — nothing in the spec edits a shape's colour (rule 10), and a field
  the inspector drags with no visible effect would be a control that lies.

## Consequences

- Every saved scene with a cube carries `[N.voe_3d_shape]` with `kind = 1`. Changing the number
  of the cube later is a converter over saved files; adding kinds is free.
- A program that draws shapes sizes its device from `3d`'s constants and runs the shape system
  each frame before drawing.
- A scene's material is not saved yet; every shape is the same grey.
