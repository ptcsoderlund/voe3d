# 11 — dev's main.c entry within the cap
folder: dev/src
after: none
decisions: 0168

## Change
`dev/src/src.md`: the `main.c` entry is 311 characters; the entry cap is 300.
Cut it to one sentence under 300: the loop, frame by frame, until the window
closes, now including the frame breakdown. The detail it drops (the order of
the systems, what to try when the picture looks wrong) already lives in the
header comment of `dev/src/main.c`; read that header and, if any point the
entry drops is missing there, add it to the header. No code changes.

## Done when
`checks.sh --folder dev/src` reports no finding on `src.md`'s `main.c` entry.
