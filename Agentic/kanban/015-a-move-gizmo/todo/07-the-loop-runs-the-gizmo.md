# 07 — The loop runs the gizmo
folder: editor
decisions: 0168, 0205, 0207

## Change
`editor/src/main.c` alone, wiring card 06's reader to card 04's field. Read `editor/src/gizmo.h` and
`3d/include/3d/draw_system.h`'s `voe_3d_gizmoed` first; this file only carries numbers between them.

A `VOE_EDITOR_GIZMO_MILLIMETRES` beside `VOE_EDITOR_OUTLINE_MILLIMETRES`, an arrow's shaft on the surface, a
comfortable few millimetres, with the same comment's reason: a size on the surface times
`pixels_per_millimetre` is a size in pixels, so the gizmo is one size on a screen of any density.

`EDITOR_CAPACITIES` grows by what draw_system.h's header says a gizmo costs: `objects` from
`(MAX_DRAWN + 1)` to `(MAX_DRAWN + 3)` per view, two more draws a pass; `transient_vertices` and
`transient_indices` from `VOE_3D_OUTLINE_*` to `(VOE_3D_OUTLINE_* + VOE_3D_GIZMO_*)` per view;
`transient_geometries` from one per view to three. Its comment paragraph gains the gizmo as the outline's is
there — one gizmo per view's pass, two ranges and two draws, sized from the room for views.

A `voe_editor_gizmo gizmo = { 0 };` beside `pick`, and in the loop, just before `voe_editor_pick_read`:
`voe_editor_gizmo_read(&gizmo, &scene, &views, VOE_EDITOR_GIZMO_MILLIMETRES * pixels_per_millimetre,
roots[0].pointer.at, left && pointer.over, <the same three panels pick is blocked by>)`. Then
`voe_editor_pick_read`'s `blocked` gains `|| voe_editor_gizmo_taking(&gizmo)`, with a line saying a press the
gizmo took is not a press that selects, and that the order is the point: the gizmo is asked first.

In each view's `voe_3d_draw_system_run`, a `.gizmo` beside `.outlined`, the same selected entity and the same
`shapes.outline` material and `size`, `.pixels` from the new constant, `.marked = voe_editor_gizmo_marked(
&gizmo, v)`, and the two colours from `voe_editor_view_gizmo_colour` with the chosen theme's palette, false
and true.

The edited check becomes `scene.inspector.replaced > 0 || scene.structural > 0 || gizmo.moved > 0`, and its
comment gains the drag: a gizmo move is an edit no menu asked for, the same half of session.h as an Inspector
number dragged. Nothing else about undo changes and nothing is added near `voe_editor_undo_settle`: the drag
holds the left button, so `at_rest` is false for every frame of it and true on the release, which is what
makes a whole drag one step.

The file's header comment gains a short paragraph in its voice on where a gizmo drag sits among this loop's
readers of the pointer — middle to the views, left to the gizmo and then to picking — and why one drag is one
undo step without this file counting anything.

## Done when
`checks.sh --folder editor` exits 0; the editor starts, shows a gizmo on the selected entity in both scene
views, and dragging an arrow moves the entity while the Inspector's numbers follow; `git diff --stat` names
`editor/src/main.c` and no other file outside `Agentic/`.
