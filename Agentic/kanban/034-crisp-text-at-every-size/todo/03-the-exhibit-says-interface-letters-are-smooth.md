# 03 — The exhibit says interface letters are smooth
folder: dev
decisions: 0168, 0269

## Change
After card 02 the dev program's comments about letters are wrong: interface text (the element
path) now has edges covered across one pixel; world text keeps its hard cut. Comments only.

- `dev/src/elements.c`: group 5 of the header ("a hard cut… Nothing here is antialiased") says
  the edge of a large letter is sharp and a small letter's edge is smooth (0269); group 6 and the
  paragraph above `TITLE_EM` say a small stem fades to partial coverage rather than breaking up,
  where the sheet runs out of room; "the hard edge of the field is plainly a hard edge" becomes
  the large line looking sharp.
- `dev/src/text.c`: the paragraph "AND THERE IS NO ANTIALIASING ANYWHERE IN THE PICTURE" says
  world letters, textures and silhouettes are still hard and intended, and the heads-up line,
  drawn as elements, is the one smooth-edged thing (0269). Keep what it says about the edges being
  in the right place.

## Done when
- `grep -ci 'nothing here is antialiased\|no antialiasing anywhere' dev/src/elements.c dev/src/text.c`
  prints 0 for both.
- `bash ~/.claude/skills/checks/scripts/checks.sh --all` passes.
- The human's, from `feature.md`'s `## How to test`: steps 1–6 — the editor at about 800×600,
  the text slider at 50% and 200%, maximised against before, the coin game's Play, and a project in
  `Spel_åäö` or `Игра` showing the missing-glyph box for what the font lacks.
