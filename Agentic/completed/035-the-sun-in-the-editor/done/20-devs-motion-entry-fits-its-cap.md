# 20 — Dev's motion entry fits its cap
folder: dev/src
decisions: 0168

## Change
Index only; no code. `checks.sh --all` finds the `motion.h` entry in `dev/src/src.md` at
313 characters, over the 300 cap (card 09 lengthened it).

- `dev/src/src.md`: shorten the `motion.h` entry below 300 characters, keeping its points:
  the flight, the orbit at a second, the pose a flight is seen from, `voe_dev_sun_pose` (the
  sun's transform turned along its lap), and that the header says why the cube's spin is not
  here. Change no other entry.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder dev/src` prints `FINDINGS: 0`.
