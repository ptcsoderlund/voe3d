# 0158 A pass may hide one entity, because a surface showing a target may not be drawn into it

Status: accepted
Date: 2026-09-15

Spec 003 criterion 1 puts a second camera's picture on a surface standing in the world. That
surface is an ordinary entity with a material whose base colour is the target's texture, so the
pass that draws the world into that target draws the surface too — an image read while it is
written, which Vulkan leaves undefined and which `voe_render_target_create`'s debug check already
asserts on. Every entity in the tables is drawn, and there is no way to leave one out.

## Decision

**`voe_3d_frame` gains `hidden`, one entity the run does not draw**, zero for none — and a zeroed
`voe_ecs_entity` is never a live one, so the field costs existing call sites nothing.
`voe_3d_draw_system_run` skips it in both tables and in both layers.

**One entity and not a list, a mask or a layer.** The case that exists is a surface showing the
picture of the pass being drawn, and there is one such surface per target. A visibility mask is a
larger decision — per camera, per layer, authored, saved — and it is made when something needs
it, not to round this off (rule 10).

## Rejected

- Draw the target from a camera looking away from the surface — the assert and the undefined read
  are about the draw being issued, not about what is visible.
- Two worlds, one per camera — every entity in both would have to be created and moved twice.
- Remove and re-add the surface's mesh component around the pass — a write to another module's
  table per frame, to undo it immediately.
- A per-entity "invisible" component — that is a property of the entity, and this is a property of
  one pass.

## Consequences

- A program drawing a camera preview sets one field; forgetting it fires the existing debug
  assert, which names the problem.
- When a second such surface appears, this becomes a list or a mask, in a record of its own.
