# 03 — split main.c: the loop's pointer and view reads move out
folder: editor/src
after: none
decisions: 0168

## Change
`editor/src/main.c` is 806 lines; cards 05 and 06 add to its loop. Before them, move one run of
it into its own file, behaviour unchanged.

- `editor/src/frame_pointer.h` (new) and `editor/src/frame_pointer.c` (new) — the loop's run from
  the comment "THE RIGHT BUTTON FLIES THE VIEW IT WENT DOWN OVER" through the call to
  `voe_editor_pick_read`: the fly, `voe_editor_frame_commands_read`, the borders
  (`voe_editor_resize_frame` and the panels' remember), the middle drag, the gizmo, the Assets
  drag and the pick, in that order, comments moved with the code. Shape it as
  `editor/src/frame_commands.h` is: a struct `voe_editor_frame_pointer` of pointers to main.c's
  parts plus whatever of this run's state outlives a frame, filled once before the loop beside
  `commands`, and one call a frame, `voe_editor_frame_pointer_read`, taking the frame's values
  (pointer, buttons, motion, seconds and the like) and giving back what main.c reads after it
  (whether a view is flying, whether the borders took the pointer, `ui`'s keyboard). The header
  says what the run owns, its order and why the order matters (the comments already say it), and
  that none of it allocates. Keep the `pointer.over` / `pointer.down` clears where the flying and
  resize answers are read, whichever side they end on.
- `editor/src/main.c` — the run replaced by the struct's fill and the one call; its header's line
  on frame_commands.h gains frame_pointer.h's. Aim under ~720 lines.
- `editor/src/src.md` — entries for `frame_pointer.h` and `frame_pointer.c`; `main.c`'s entry
  unchanged unless it names the moved parts.

## Done when
- `cmake --build --preset debug --target voe_editor` exits 0 and `wc -l editor/src/main.c` is
  under 730.
- `./build/debug/editor/voe_editor --capture build/debug/065-split.png` from the tree root exits 0.
- Human: open the editor; fly a view (right button), orbit (middle drag), drag a gizmo arrow, click
  to pick and drag a panel border; each behaves as before.
