# 78 — The 3d model levels entry is one sentence
folder: 3d/src
after: none
decisions: 0168

## Change
`3d/src/src.md`: the entry `model_levels.c` is 341 characters, over the
300-character cap.

Cut it to one sentence under 270 characters: a `.glb`'s levels file made
(level 0 baked, each level simplified to its share, or own levels baked and
measured), written and read back with stale fields refused, its swaps patched
in place, and each level's triangles. Details that may be dropped (the FNV-1a
hash; part by part until a level is not below the one before) belong in
`3d/src/model_levels.c`'s header comment (22 lines); add there only what it
lacks: the FNV-1a hash and the swaps patched in place at a fixed offset, if
not already said.

Comments and index only; no code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --structure` prints no
FINDING line naming `3d/src/src.md`.
