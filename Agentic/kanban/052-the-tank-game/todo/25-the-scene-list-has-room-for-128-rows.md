# 25 — The Scene list, the dropdown and undo have room for 128 authored entities
folder: editor/src
after: 24
decisions: 0168, 0204, 0337

## Change
0337 point 3, the editor's half. Read `editor/src/scene.h`, `editor/src/interface.h`,
`editor/src/scene_list.c` (only to count what one row draws), `editor/src/undo.h`,
`editor/src/prefabs.h` and `editor/src/entities.h`.

- `scene.h`: `VOE_EDITOR_SCENE_ROWS` is defined as `VOE_GAME_WORLD_AUTHORED` (include
  `game/world.h`), so the list and the identity table are one number by construction. Its comment
  drops "thirty-two" and "the two this file builds", and says the interface's budget is counted for
  it. `project.c`'s `static_assert` stays as it is.
- `interface.h`: every budget paragraph that counts Scene list rows (32) or dropdown rows (33) counts
  them at 128 and 129: the row buttons' second records, the rim wrappers, the dropdown's rows and
  their names' characters, and whatever one Scene list row draws (its button, label, fold button,
  file name) that the loose 512 held at 32 rows, counted from `scene_list.c` for the 96 rows more.
  `VOE_EDITOR_INTERFACE_NODES` and `VOE_EDITOR_INTERFACE_ELEMENTS` are the new sums, each
  paragraph's running total carried through to them.
- `undo.h`: `VOE_EDITOR_UNDO_TEXT` is 64 KiB; the CONSTRAINTS paragraph's sizes follow (four
  megabytes a line, eight in all), and says a full world of 128 at about 300 bytes an entity fits.
- `prefabs.h`: the constraint "32 identities make it moot" names the authored room, no number.
- `entities.h`: the CONSTRAINTS line about `VOE_EDITOR_SCENE_ROWS` rows keeping a scan cheap still
  holds at 128; change it only if it names 32.

## Done when
`grep -q 'define VOE_EDITOR_SCENE_ROWS VOE_GAME_WORLD_AUTHORED' editor/src/scene.h` exits 0, and
`build/debug/editor/voe_editor examples/tank_game --capture build/debug/rows.png --frames 4`
exits 0 with the PNG written.
