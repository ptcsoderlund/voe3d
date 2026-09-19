# 18 — The colour swatch and picker, and the feature's test holds
folder: editor
decisions: 0168, 0191, 0192, 0193, 0177
read: feature.md

## Change
None expected. The swatch in `src/inspector.c`, `picking` in `src/scene.h`, the anchored picker in
`src/interface.c`, Escape's order in `src/main.c` and the budgets in `src/interface.h` are committed (card 16,
commit bdceb1a); card 17 cleared the analyser findings that kept `checks.sh --all` from passing. Run the proofs
below. If one fails for a reason inside `editor`, fix it there; if outside, write `## Blocked` naming it.

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
