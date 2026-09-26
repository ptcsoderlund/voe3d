# 03 — Ship builds the release game and installs it, a step at a time
folder: editor
decisions: 0168, 0237, 0240, 0242, 0264

## Change
The folder exists. Read `editor/src/refresh.h`, `refresh.c` and `game_tree.h`; Ship is
Refresh's shape with two more steps.

- New `editor/src/ship.h` and `ship.c`:
  - `voe_editor_ship_stage`: IDLE, CONFIGURING (when `Build/release/` has no cache yet),
    BUILDING, CLEARING (`voe_editor_game_tree_ship_clear`), INSTALLING
    (`voe_editor_game_tree_install`).
  - `voe_editor_ship_result`: RUNNING, SHIPPED, FAILED.
  - `voe_editor_ship`: stage, process, its own arena (NULL while idle), folder copied at the
    start. Zeroed is idle.
  - `void voe_editor_ship_start(voe_editor_ship *, const voe_editor_project *, voe_editor_notice *why)`
    — asserts idle; untitled refuses with a notice to save once; otherwise writes the game tree
    (the scene as it is now), empties `Build/build.log` and starts the RELEASE configure or,
    when configured, the build, every step writing to the log. A refused write or start: `why`
    says so and it is idle again.
  - `voe_editor_ship_result voe_editor_ship_poll(voe_editor_ship *, voe_editor_notice *told)` —
    never blocks, asserts not idle; a step that ended 0 starts the next; INSTALLING ending 0 is
    SHIPPED with `told` set to "Shipped to <the shipped folder>"; a step ending
    non-zero prints one `voe_editor: …` stderr line naming the step and the log and is FAILED.
    Idle again after SHIPPED or FAILED, arena destroyed.
  - `void voe_editor_ship_end(voe_editor_ship *)` — ends what runs, idle; idle is a no-op.
  - `const char *voe_editor_ship_label(const voe_editor_ship *)` — "Ship" idle, "Shipping"
    otherwise.
  - The header says: why a failed build leaves the old folder (CLEARING comes after the build),
    that nothing saves the project (0188), the shipped folder's path is
    `voe_editor_game_tree_shipped`, and a caller ends a ship before its own exit.
- `editor/src/src.md` — entries for `ship.h` and `ship.c`.

Nothing calls it yet; card 04 does.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder editor` prints `FINDINGS: 0`.
