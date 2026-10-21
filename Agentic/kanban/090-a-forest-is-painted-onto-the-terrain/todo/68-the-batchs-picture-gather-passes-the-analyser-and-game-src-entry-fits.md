# 68 — The batch's picture gather passes the analyser, and the game/src entry fits the cap
folder: game/src
after: none
decisions: 0168

## Change
Two whole-suite findings in this folder.

`game/src/models_batch.c`, `pictures_gather` (about line 453): clang's
static analyser reports a null dereference at the write
`batch->pictures[at++] = ...` (about line 471). Its path: the first loop sums
`model.image_count` into the `uint32_t total`, which it may assume wrapped to
0, so the list is left NULL; the second loop then still finds a `.glb` with
pictures and writes into it. Fix it in that function only: the inner loop's
condition is also bounded by `at < total`, so no picture is written past the
list the function made. The `at == total` assert after the loops stays. No
`__clang_analyzer__` suppression. This was tried while planning and leaves the
file clean. The function's one-line comment may add that a write never
passes the counted list. Behaviour is unchanged; the existing `game/` tests
keep passing.

`game/src/src.md`: the entry `models_materials.c` is 336 characters, cap 300
(the whole line, bullet and name included). Shorten it to one sentence of at
most 300. Any point it drops that the header comment of
`game/src/models_materials.c` does not already make goes into that header,
kept under its 60-line cap (measure with
`bash ~/.claude/skills/checks/scripts/checks.sh --header-lines game/src/models_materials.c`).

## Done when
From the repo root, both print nothing:
- `clang --analyze -Xanalyzer -analyzer-output=text -DVOE_BASE_DESCRIPTIONS=1 -Igame/src -Igame/include -Iapp/include -Irender/include -Iplatform/include -Ibase/include -Imath/include -Iassets/include -I3d/include -Iphysics/include -Iscene/include -Iecs/include -Iui/include -Itext/include -Iaudio/include -std=c23 -o /dev/null game/src/models_batch.c 2>&1 | grep -F 'warning:'`
- `bash ~/.claude/skills/checks/scripts/checks.sh --structure 2>&1 | grep -F 'FINDING game/src/src.md'`
