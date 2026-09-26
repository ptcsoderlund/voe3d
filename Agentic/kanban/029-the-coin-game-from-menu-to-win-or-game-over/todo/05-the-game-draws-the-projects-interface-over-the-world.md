# 05 — The game draws the project's interface over the world
folder: game
decisions: 0168, 0259, 0234, 0245, 0257

## Change
Read `game/include/game/interface.h` (card 02), `game/include/game/frame.h`,
`game/include/game/run.h`, `render/include/render/device.h` (`voe_render_frame_clear_depth`,
the element section).

- `game/include/game/frame.h`, `game/src/frame.c` — `VOE_GAME_CAPACITIES` gains
  `.elements = VOE_GAME_INTERFACE_ELEMENTS` (its comment: the interface's records, no longer
  "no interface"). `voe_game_frame` gains a last parameter `const voe_ui_context *ui`: inside the
  window pass, after the world, when `ui` is not NULL and has records, depth cleared, the records
  submitted and drawn in one command with `voe_render_element_transform` of
  `voe_game_interface_surface(size)`. NULL draws none. Header: the order gains the interface.
- `game/include/game/run.h`, `game/src/run.c` — after the device opens, `voe_game_interface_new`
  (NULL is a refusal: a line and 1); each frame, after the steps,
  `voe_game_interface_run(..., voe_game_project_interface)` on the scratch arena; false leaves
  the loop and the run returns 0; then `voe_game_frame` with the interface's context. The
  interface destroyed on every path out. Header: the order; "no quit key" becomes: no quit key,
  but the project's interface may end the run (0259); four entry points linked.
- `game/tests/frame.c` — calls pass NULL; one more case draws a frame with a context holding a
  label (made as `tests/interface.c` does) and comes back true.
- `game/include/game/game.md`, `game/src/src.md` — the frame and run entries.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder game` prints `FINDINGS: 0`.
2. `cmake --build --preset debug --target voe_editor`, then in `p=$(mktemp -d)` with
   `cp -r examples/capsule/. $p` and `rm -rf $p/Build`:
   `build/debug/editor/voe_editor --capture $p/shot.png $p 2>$p/err` exits 0 and
   `cmake -S $p/Build/game -B $p/Build/debug -G Ninja && cmake --build $p/Build/debug --target
   game` exits 0 (all four entry points link).
3. The human's: Play `examples/capsule/` in the editor; it walks, jumps and follows as before.
