# 43 — The game loader's source entry is one sentence
folder: game/src
after: none
decisions: 0168

## Change
`game/src/src.md`: the entry for `models.c` is 321 characters, cap 300. Cut
it to one sentence under 270 characters: a path joined onto the folder,
stamped, read and loaded, a `.glb` with its levels file, and a cooked
landscape loaded, each failure counted. Details that may be dropped (rewound
scratch; the `Cache/levels` path; the levels made with the `.parts`'s count;
the dot and water when needed; a landscape's millimetres as metres with
layers and paint, never watched; each failure a stderr line) must be in
`game/src/models.c`'s header comment (14 lines); add any it lacks.

Comments and index only; no code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --structure` prints no
FINDING line naming `game/src/src.md`.
