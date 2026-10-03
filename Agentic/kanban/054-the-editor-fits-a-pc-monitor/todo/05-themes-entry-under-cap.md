# 05 — The `themes.h` index entry fits the cap
folder: editor/src
after: none
decisions: 0168

## Change
Only `editor/src/src.md` and, if a point is lost, the header comment of
`editor/src/themes.h`. No code changes.

The `themes.h` entry in `editor/src/src.md` is 305 characters; the cap is 300.
Shorten it to one sentence under 300 characters that still says: the two
built-in themes then one per `*.theme` file, the chosen one remembered and
re-read, each derived at the editor's base text size and spacing. Drop detail
(paths, the "two scalars and text scale") rather than meaning. Whatever the
entry drops must already be in the header of `themes.h`; the settings paths
are there, so check only that the header says each theme carries its scalars
and text scale at the editor's base text size and spacing (card 02), and add
that point to the header if it is missing.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder editor/src` prints no
finding naming `themes.h`.
