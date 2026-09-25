# 05 — Split the scene reader by what it reads
folder: authoring
decisions: 0168

## Change
`authoring/src/scene_read.c` is 1146 lines and card 06 changes it. Split it by function, no
behaviour change, before that card runs.

- `authoring/src/field_read.h` + `authoring/src/field_read.c` (new) — one field's value read from
  the text: the cursor and its blanks, tokens, brackets and their depth ceiling, the number
  spellings (`unsigned_of`, `signed_of`, `float_of`, `floats_of`, the stores), `element_of`,
  `spelling`, the CHAR reads, `held_element`, `shape_value`, `elements_of` and `field_value`, with
  whatever struct they share with the passes (`struct cursor`, `struct site`, the part of
  `struct reader` they touch) declared in the `.h`. Only what `scene_read.c` calls is non-static.
  Header points, moved from `scene_read.c`'s: nothing recurses (rule 14, the explicit stack and
  BRACKET_DEPTH_MAX), numbers are read in the "C" locale after the spelling is checked by hand.
- `authoring/src/scene_read.c` — keeps the two passes: classifying sections, `type_by_name`,
  `row_size`, `field_by_name`, `read_section`, keeping sections, patching, `create` and
  `voe_authoring_scene_read`; its header keeps what is about the passes.
- `authoring/src/src.md` — entries for `field_read.h` and `.c`; `scene_read.c`'s entry narrowed.

Each resulting `.c` under ~700 lines. No public header changes.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder authoring` prints `FINDINGS: 0`.
2. `ctest --test-dir build/debug -R '^authoring/'` passes with no test file edited.
3. `wc -l authoring/src/scene_read.c authoring/src/field_read.c` shows each under 800.

## Blocked
The split is done (`field_read.h`/`.c` 628 lines, `scene_read.c` 523, `src.md` updated), but `authoring`
does not build at HEAD: card 01's `VOE_BASE_FIELD_DOUBLE3` is unhandled in the switches of `scene_cook.c`,
`scene_write.c` and the reader's `spelling()` (now in `field_read.c`), and with `-Wswitch` off the same seven
`scene_read`/`scene_cook`/`scene_write` test failures occur before and after the split (card 03's double
position). Card 06 handles both; run it on top of this split, then re-check this card's `## Done when`.
