# 05 — The top bar has a Ship button
folder: editor
decisions: 0168, 0225, 0264
read: feature.md

## Change
Read `editor/src/topbar.h`, `topbar.c`, `interface.h`, `interface.c` and `session.h`.

- `editor/src/topbar.h` and `topbar.c` — `ship_button` between Refresh and Preferences;
  `voe_editor_topbar_draw` takes a `ship` label after `refresh`; `voe_editor_topbar_clicks_read`
  answers `VOE_EDITOR_COMMAND_SHIP` for it. The header's order and "six buttons" become seven.
- `editor/src/interface.c` — polls `voe_editor_session_ship_poll` where the play poll is, once a
  frame before any root is built, and hands `voe_editor_session_ship_label` to the bar.
- `editor/src/interface.h` — a paragraph after the Refresh button's: the Ship button adds two
  nodes and ten elements (border, fill, the eight letters of "Shipping");
  `VOE_EDITOR_INTERFACE_NODES` 1179, `VOE_EDITOR_INTERFACE_ELEMENTS` 22386; the loop paragraph
  names the ship poll beside the play poll.
- `editor/src/src.md` — the `topbar.h`, `topbar.c` and `interface.c` entries name Ship.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder editor` prints `FINDINGS: 0`.
2. `cmake --build --preset debug --target voe_editor`, then in `p=$(mktemp -d)` with
   `cp -r examples/coin_game/. $p` and `rm -rf $p/Build`:
   `build/debug/editor/voe_editor --capture $p/shot.png $p 2>$p/err` exits 0 (the bar with Ship
   fits its budget).
3. `bash ~/.claude/skills/checks/scripts/checks.sh --all` prints `FINDINGS: 0`.
4. The human's: `feature.md`'s `## How to test`, steps 1 to 7, in the editor on Linux.
