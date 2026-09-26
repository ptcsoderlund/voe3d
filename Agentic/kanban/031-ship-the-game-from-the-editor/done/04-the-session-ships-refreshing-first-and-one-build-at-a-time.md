# 04 — The session ships, refreshing first, one build at a time
folder: editor
decisions: 0168, 0240, 0242, 0264

## Change
Read `editor/src/session.h`, `session.c` and `ship.h`. Ship joins the session the way Play
does (0264 points 4 and 5).

- `editor/src/session.h` and `session.c`:
  - `VOE_EDITOR_COMMAND_SHIP`; the session gains `voe_editor_ship ship` and `bool ship_after`
    (the running refresh starts Ship once its code is in).
  - SHIP never arms. A ship running or `ship_after` set: nothing. Play configuring or building,
    or a refresh with `play_after`: the notice says to wait for that build. Any other running
    refresh: sets `ship_after`. Code in `Code/` (`voe_editor_game_tree_has_code`): starts a
    refresh with `ship_after`. Otherwise `voe_editor_ship_start` now. A ship or its refresh that
    starts hides `errors`.
  - PLAY that would start a build or refresh, and REFRESH, while a ship runs or `ship_after` is
    set: the notice says to wait for Ship; Play's Stop of a running game is unchanged. A due
    refresh waits (stays due) while a ship runs.
  - `voe_editor_session_step`: after a swap, `ship_after` starts the ship; a failed build, load
    or swap drops `ship_after` as it drops `play_after`.
  - `void voe_editor_session_ship_poll(voe_editor_session *)` — once a frame, a ship not idle is
    polled: SHIPPED sets the notice to what the poll told; FAILED shows `errors` from the build
    log.
  - `const char *voe_editor_session_ship_label(const voe_editor_session *)` — "Shipping" while
    a refresh with `ship_after` runs, else `voe_editor_ship_label`.
  - A CLOSE that goes ahead ends the ship; NEW and OPEN end it and drop `ship_after`.
  - The header gains a SHIP paragraph and the command count and list say seven.
- `editor/src/src.md` — the `session.h` and `.c` entries mention Ship.

Card 05 puts the button on the bar and calls the poll.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder editor` prints `FINDINGS: 0`.
