# 21 — The scene headers' index lists the picture row
folder: scene/include/scene
after: none
decisions: 0168

## Change
`scene/include/scene/scene.md` lacks entries for two headers card 16 added.

- Read the header comments of `scene/include/scene/picture_component.h` and
  `scene/include/scene/picture_system.h`.
- `scene/include/scene/scene.md`: one entry each, beside the `sky_component.h` and
  `sky_system.h` entries and in their style: what the picture row holds (exposure and tone)
  and what its system does. Each at most 300 characters. No other file changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder scene/include/scene` prints
`FINDINGS: 0`.
