# 0191 — A shape's colour is a described linear colour, drawn through its object record
date: 2026-09-19
by: planner

## Decision
For spec 010.
- **`base` gains the field kind `VOE_BASE_FIELD_COLOUR`**: three floats, 12 bytes, linear RGB, each 0 to 1. It is
  stored and spelled in scene text exactly as `FLOAT3`. It exists so a tool can show a colour as a swatch without
  naming the component, the way `QUAT` lets the Inspector show three angles.
- **`voe_3d_shape` gains `colour` (`COLOUR`, editable)** after the read-only `kind`. Its default is today's grey
  `(0.7, 0.7, 0.7)` linear. The shape gets a replace intent drained by the shape system: `kind` is put back to the
  entity's own, and each colour channel is clamped to 0..1.
- **The colour reaches the GPU in the per-object record, not in a shading record.** `voe_render_object` gains
  `voe_math_float4 colour` at offset 144 (size 160), multiplied into the shading record's base colour in
  `draw.slang`. The shapes' one material becomes white `(1, 1, 1)`. The draw system writes a shape's colour (alpha
  1) for an entity with a shape and `(1, 1, 1, 1)` for every other.
- **Two more built-in kinds**: `VOE_3D_SHAPE_CAPSULE 2u` (upright along Y, radius 0.5, 2 tall from end to end, a
  hemisphere at each end) and `VOE_3D_SHAPE_CYLINDER 3u` (radius 0.5, 1 tall, flat caps). Both are centred on the
  origin like the cube, 32 segments around, the capsule's hemispheres 8 rings each.
- **The shape system drops what it derived through the structural queue** (0190): an entity holding a mesh on one
  of the shapes' geometries and the shapes' material, with no shape row, gets remove requests for both. The mesh
  is drawn one more frame, white. Accepted.

## Reasoning
A shading record is written once at startup and has no update (render/device.h), so a colour that changes live
while a person drags needs either a new record per change or an update racing frames in flight. The object record
is already written per draw into a per-frame-slot buffer, so a colour there is free to change every frame.
Colour stored linear keeps today's grey exact and is what every shader reads; the picker converts to and from
sRGB for its hex box and its HSV square. A colour kind rather than a field-name convention follows `QUAT`'s
precedent and keeps the Inspector naming no component (ADR-0134). Rejected: a material per shape (no update, 64
records), a tint packed into the record's reserved word (a zero would mean white, a trap), removing derived rows
directly (0190 says structure goes through the queue; one frame of white is cheaper than an exception).

## Replaces
nothing
