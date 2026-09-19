# 16 — The colour swatch and picker, and the feature's test holds
folder: editor
decisions: 0168, 0191, 0192, 0193, 0177
read: feature.md

## Change
`src/inspector.c`: a `COLOUR` field on a type with a replace intent is a button holding a `voe_ui_swatch` (card
11) of the row's colour, not three numbers. A fired swatch opens the picker for that entity, type and field
offset, held in a new `picking` on `voe_editor_scene` (`src/scene.h`). A read-only COLOUR field is a swatch with
no button.

`src/interface.c`: while `picking` names a live entity that still has the row, the picker
(`voe_ui_colour_picker`, seeded from the row) is drawn as an anchored panel over the dock, beside the Inspector
column, the way Preferences is anchored. After the frame, a `changed` result goes through the type's replace
intent at once (the shape changes live) and counts in `scene.inspector.replaced`. An `outside` result closes the
picker, and so do Escape and a change of selection. Closing keeps the colour. Escape's order in `src/main.c`:
`ui` typing first (card 15), then the picker, then the browser and Preferences.

Raise `src/interface.h`'s budgets by the picker's cost as `ui/colour.h` states it. Update `editor.md` and
`src/src.md`.

## Done when
`checks.sh --all` exits 0.

With `C=$(mktemp -d)`, write a project by hand in `$C/p`: `project.voe3d` as `authoring/project.h` says, and
`main.scene` as `authoring/scene_write.h` says. It holds a light; a cube at (-2, 0, 0); a capsule at (0, 0, 0);
and a cylinder at (2, 0, 0) with scale (1, 0.1, 1) and colour (1, 0.578, 0), the linear form of `#FFC800`. Then
`./build/debug/editor/voe_editor $C/p --capture $C/o.png` exits 0, and reading the PNG (ADR-0177) shows a gold
disc, an upright capsule with rounded ends and a grey cube, lit, in both views.

For the human, with `./build/debug/editor/voe_editor` and `feature.md`'s `## How to test`: steps 1–14. Step 2
(shapes look right when orbited), step 9 (the picker's square, strip and hex box) and step 14 (the capture of
the scene built in steps 1–10) are looked at on screen.
