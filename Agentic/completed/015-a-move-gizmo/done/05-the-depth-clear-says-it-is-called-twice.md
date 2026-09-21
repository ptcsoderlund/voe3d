# 05 — The depth clear says it is called twice
folder: render
decisions: 0168, 0205

## Change
`render/include/render/device.h` alone, and nothing under `render/src/`. `voe_render_frame_clear_depth`'s
header comment says "See voe_3d_draw_system_run, which is the one caller, and which calls this once between
the world and the overlay". Card 04 made that sentence false: the same caller now clears a second time,
between the selection outline and the move gizmo, so that the gizmo stands in front of the outline too.

Say instead, in the same voice and the same number of sentences, that `voe_3d_draw_system_run` is still the
one caller and that it clears once between the world and the overlay and again before the gizmo it draws
last — each clear being one more group in front of everything before it. Change no other sentence, no
signature and no code: a reader of this header must not have to open `3d` to learn that a layer costs one
call and that there may be more than two of them.

## Done when
`checks.sh --folder render` exits 0; `git diff --stat` names `render/include/render/device.h` and no other
file outside `Agentic/`; `git diff -U0` shows comment lines only, no line without a leading `//`; and
`grep -c gizmo render/include/render/device.h` is 1 or more.
