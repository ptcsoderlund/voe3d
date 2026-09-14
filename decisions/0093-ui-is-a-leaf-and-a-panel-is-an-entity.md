# 0093. `ui` is a leaf on `render` and `text`, and a panel reaches the frame as an entity

- **Status:** Accepted
- **Date:** 2026-09-07
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —
- **Closes:** D-149

## Context

The GUI is being cut into cards, and two structural questions block every one of
them. Both were found by reading the code rather than the plan.

**First: there is no `ui` row in the dependency map.** `cmake/voe.cmake`'s
`voe_allowed_deps` has rows for `base`, `math`, `ecs`, `platform`, `scene`,
`assets`, `render`, `text`, `3d`, `sprite`, `app` and `dev` — and nothing for `ui`.
Engine rule 2 says an edge outside the map fails configuration and **a change that
needs one is reported, not made.** So the folder cannot exist until this is
decided.

**Second: `voe_3d_draw_system_run` owns the whole frame.** It calls
`voe_render_frame_begin`, issues every draw, and calls `_end`. There is no seam a
program can use to inject its own drawing, so a GUI cannot simply draw itself.

Constraints already fixed:

- **ADR-0022 / ADR-0030** — `text`, `sprite`, `ui` planned beside `3d`, each on
  `render`. **`sprite` already departed from that** and depends on `3d`, decided by
  card 022 — so a planned row is not binding, but changing one is a decision.
- **Rule 2 — no upward or sideways dependencies, no cycles.**
- **`text`'s precedent, and it is the model here.** Its map comment: *"text sits
  beside 3d rather than under it: it turns a string into a mesh and a texture and
  knows nothing about entities or files, so it names render and no more. What wears
  that mesh is a 3d material, and putting the two together is the caller's — which
  is what keeps this a leaf and keeps 3d from growing a reason to name a font."*
- **ADR-0092 — the GUI's output is an element buffer**, drawn in one instanced call.
- **ADR-0091 — immediate mode**, so the buffer is rebuilt each frame.
- **ADR-0089 — authored in millimetres.** **ADR-0090 — the GUI takes a font.**
- **ADR-0085 — a mesh's geometry is replaceable**, the precedent for a component
  holding an id that changes each frame.

## Decision

### `ui` depends on `render`, `text`, `math` and `base` — and nothing else

It is a leaf, on exactly `text`'s model. It turns widget calls into an element
buffer and leaves *placing* that buffer in the world to the caller.

Specifically, and these are the load-bearing exclusions:

- **Not `platform`.** `ui` does not read input. It is *given* a pointer position
  and button state as plain values.
- **Not `scene`, not `ecs`, not `3d`.** `ui` knows nothing about entities,
  cameras, transforms or layers.
- **`text`, yes.** Laying out a label needs glyph advances and the atlas, and
  ADR-0090 already says the GUI takes a `voe_text_font *`. This is a **new edge**
  and it is the reported change engine rule 2 requires. It is downward — `text`
  depends on `render`, `math`, `base`, so nothing cycles.

### A GUI is a flat surface with a transform, and picking happens outside it

`ui` builds elements in millimetres in two dimensions, origin at the panel's
corner. **Where that surface sits in the world is one matrix and it is not `ui`'s.**

This is what keeps the leaf a leaf, and it also resolves picking: `ui` receives a
pointer as **a two-dimensional point in millimetres, in the panel's own space**.
Whoever owns the camera intersects the ray with the panel's plane and hands over
the result. `ui` never sees a ray, a camera or a matrix.

It also means both of card 023's placements are the same thing to `ui` — a panel on
a wall and a panel in front of your face differ only in their matrix, which is the
constraint card 023 said the GUI most had to respect.

### A panel reaches the frame as an entity, and `3d` never names `ui`

`render` owns element buffers and hands back an id, exactly as it does for
geometry and textures. `3d` gains a **panel component** holding that id, a
transform and a layer, and its draw system draws panels alongside meshes in the
existing pass and layer order.

So the chain is `ui` → an element buffer id → a `3d` component → the draw system,
and **`3d` names `render`, which it already does.** No new edge into `3d`, and the
GUI is not a special case in the frame.

**Why not the alternatives.** Having `render` draw submitted elements at a fixed
point in the frame was the tempting shortcut and it is wrong: it hardcodes *the GUI
draws last*, which means a panel bolted to a wall is not occluded by the wall.
Panels must take part in depth and layer ordering like everything else, and the
mechanism for that already exists and is the draw system.

## Blast radius

**Moderate.** The dependency edge and the component are both public and permanent
once cards build on them. What stays cheap is the picking split — if handing `ui` a
two-dimensional point turns out awkward, a helper in `3d` that produces it is an
addition rather than a change.

The thing that would be expensive and is deliberately avoided: `ui` growing a
dependency on `scene` or `3d` to do its own picking or placement. That is the edge
that would make the GUI un-testable without a world, and it is the one `text`
avoided for the same reason.

Reversibility: **moderate.**

## Consequences

- **`ui` is testable without a world, a camera or a window**, the same property that
  makes `text` testable. A test hands it a font, a pointer position and a size, and
  reads the elements back.
- **A new edge exists in the map** — `ui → text` — and `cmake/voe.cmake` gains a
  row. That row's comment should say why, in the same voice as `text`'s and
  `sprite`'s, because those comments are how the next person understands the map.
- **`3d` gains a second drawable component**, which is the first time it has had
  more than meshes. The draw system's walk grows a second table.
- **Picking is split across two folders**, and that is a real cost: the ray maths
  lives with the camera and the hit test lives with the widgets, so a bug in
  pointing at a panel could be in either. The seam is one `float2` and it should be
  stated plainly in both headers.
- **The consequence I do not like:** `ui` cannot draw anything on its own. Every
  test and every program needs `3d` to put a panel on screen, so the leaf property
  is real for unit tests and slightly fictional in practice.

## Rejected options and why

**`ui` depends on `platform` and reads input itself.** Convenient, and it welds the
GUI to a window. A panel in a world seen through a headset has no window-relative
pointer, and a test would need one. Rejected.

**`ui` depends on `3d` and places itself.** The `sprite` shape. Rejected because
`sprite`'s reason does not apply — sprite hands back a `voe_3d_material`, which is
`3d`'s type, whereas `ui` hands back an element buffer, which is `render`'s. There
is nothing `ui` needs to say in `3d`'s vocabulary.

**`render` draws submitted element buffers at a fixed point in the frame.** The
shortcut, and it breaks world-space panels by drawing them on top of everything.

## Questions this opens

- **D-150 — what the panel component carries** beyond an element buffer id, a
  transform and a layer, and whether its size in millimetres is on the component or
  implied by the elements. Trigger: the panel card.
- **D-151 — where the ray-to-panel-space helper lives.** `3d` is the obvious home
  since it owns cameras and transforms, but nothing needs it until the second
  caller. Trigger: the picking card.

## Terminology note · 2026-09-09

**This ADR's title and its third decision say *a panel reaches the frame as an
entity*, and that phrasing is wrong.** The principal corrected it the day card 032
was written: **an entity is an id and nothing else.** `voe_ecs_entity` is two
`uint32_t`, and its own header says it *"names nothing but a row in whatever tables
hold one for it."*

**A panel is a component** — a row in `3d`'s new panel table, keyed by an entity id,
beside the `voe_scene_transform` row that places it. Nothing in the decision changes:
`ui` stays a leaf, `3d` still never names `ui`, picking still happens outside, and
panels still take part in the existing pass and layer order. What changes is the
words, and they matter because the loose phrase hides where the data lives — the one
thing a component's header has to state plainly.

**The title and the filename stay as they are**, because ADRs are append-only and
every link into this one would break. The card is renamed instead — *032 — a panel
is a component* — and the register carries the correction.

**The sloppiness is older than this ADR and it is house-wide**: ADR-0084 says
*everything drawn is an entity* and ADR-0085 opens with the same sentence. Both mean
*has a mesh component*. They are not being rewritten; anything written from here on
says *component*, and a card that finds the old phrasing in a header it is touching
fixes it there.
