# 09 — The audio headers have their map
folder: audio/include/audio
decisions: 0168, 0265

## Change
The folder exists; it lacks its `<folder>.md` (0120, 0133). Read `audio/include/audio/mixer.h`'s
header comment and `game/include/game/game.md` for the shape.

- New `audio/include/audio/audio.md` — title `# audio`, a one-line opening saying these are
  the public headers, one entry each, and one entry for `mixer.h`: play by path, mix, pump,
  and that its header says why everything is float, why a play overlaps and why a broken file
  is silence plus one line. One sentence, under 300 characters.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder audio/include/audio` prints `FINDINGS: 0`.
