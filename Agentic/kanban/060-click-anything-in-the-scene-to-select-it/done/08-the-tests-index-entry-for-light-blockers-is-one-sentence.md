# 08 — The tests' index entry for light_blockers.c fits its cap
folder: 3d/tests
after: none
decisions: 0168

## Change
In `3d/tests/tests.md` the entry for `light_blockers.c` is 346 characters, cap
300. Shorten it to one sentence under 300: what the file proves about light
blockers (box follows transform, recorded per frame, lined in the editor's
colours). Read the header comment of `3d/tests/light_blockers.c`; any detail
the entry drops that the header does not already say (the 12 edges, black
ground, a Direct's shadow reading the fill, selected outlined, none when not
shown) goes into that header. Only those two files change.

## Done when
`checks.sh --folder 3d/tests` prints no finding for `tests.md` or
`light_blockers.c`.
