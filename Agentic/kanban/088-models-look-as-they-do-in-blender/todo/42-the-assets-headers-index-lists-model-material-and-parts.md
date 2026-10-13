# 42 — The assets headers' index lists model_material.h and parts.h
folder: assets/include/assets
after: none
decisions: 0168

## Change
`assets/include/assets/assets.md` lacks entries for two public headers this
feature added. Read the header comments of
`assets/include/assets/model_material.h` and `assets/include/assets/parts.h`,
then add one entry each to `assets.md`, one sentence under 300 characters,
in the shape of the entries already there:

- `model_material.h` — a glTF material as a `.material` file's values, and
  the ORM picture Import textures writes for it; opens no file.
- `parts.h` — a `.parts` file's text to the `.material` each glTF material
  of a `.glb` wears, and back.

No header changes; no code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder assets/include/assets` prints no FINDING naming
`assets/include/assets/assets.md`.
