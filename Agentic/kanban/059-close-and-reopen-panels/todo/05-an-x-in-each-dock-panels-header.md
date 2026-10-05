# 05 — An × in each dock panel's header closes it
folder: editor
after: 04
decisions: 0168, 0351, 0363

## Change
0363 point 2 for the four dock leaves.

- `editor/src/dock.h`: `voe_editor_dock_closes`, one `voe_ui_node` per docked closable, recorded by the walk;
  `voe_editor_dock_walk` gains `voe_editor_dock_closes *closes`;
  `voe_editor_closable voe_editor_dock_closes_read(const voe_ui_context *ui, const voe_editor_dock_closes
  *closes)` — the × that fired this frame, COUNT for none, in the window after `voe_ui_frame_end`.
- `editor/src/dock_walk.c`: every laid closable leaf's panel begins with a header row outside its scroll area
  (the bottom view's above its picture, the picture keeping the rest): `voe_editor_closable_name` at the left,
  an × (U+00D7) button at the right; a leaf not drawn leaves its node zeroed. The top view has no header.
- `editor/src/panels.h` and `panels.c`: `[[nodiscard]] bool voe_editor_panels_toggle(voe_editor_closable
  which, voe_editor_dock_root *root, const voe_editor_topbar *bar)` — flips a docked panel's flag and
  remembers through `voe_editor_panels_remember`; asserts on PROJECT and ERRORS (card 07 adds them); false
  when the write fails.
- `editor/src/interface.h` and `interface.c`: `roots` becomes non-const; each root's walk gets a
  `voe_editor_dock_closes` of the frame's; after the frame ends a fired × is toggled on that root, a failed
  write put in the session's notice through `notice.h` as `main.c`'s resize remember does ("editor_settings").
  The budget: a paragraph counting the four header rows (the row, the name label, the × button and its label;
  their border, fill and characters, names at most "Bottom view") and the three numbers raised by it.
- `editor/src/main.c`: nothing unless the non-const `roots` needs it.
- `editor/src/src.md`: `dock_walk.c`, `panels.h`, `interface.c` entries name the header and the toggle.

## Done when
- The folder builds.
- `grep -q voe_editor_panels_toggle editor/src/interface.c` exits 0.
- `d=$(mktemp -d) && XDG_CONFIG_HOME=$d build/debug/editor/voe_editor --capture $d/a.png && test -s $d/a.png`
  exits 0 (the budget holds with every header drawn).
- The human, in the editor: the × on the Inspector closes it and the views take its room; restarted, it is
  still closed (How to test 2, 6 for the Inspector; 4 for the bottom view; 8).
