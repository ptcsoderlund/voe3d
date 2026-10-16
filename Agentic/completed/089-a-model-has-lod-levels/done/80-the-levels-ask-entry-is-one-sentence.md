# 80 — The levels ask entry is one sentence
folder: editor/src
after: none
decisions: 0168

## Change
`editor/src/src.md`: the entry `levels_ask.h` is 350 characters, over the
300-character cap.

Cut it to one sentence under 270 characters: the anchored panel a model's
LODs ask, its count, strength and a row per LOD with share, swap and
triangles read from the `.parts`, with Make and Cancel; Make saves into the
`.parts` and says whether a reload is enough. Details that may be dropped
(LODs 1 to 6; Gentle, Medium or Strong; lit choices; only swaps changed on a
fresh levels file) are already in `editor/src/levels_ask.h`'s header comment
(42 lines); add only what it lacks.

Comments and index only; no code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --structure` prints no
FINDING line naming `editor/src/src.md`.
