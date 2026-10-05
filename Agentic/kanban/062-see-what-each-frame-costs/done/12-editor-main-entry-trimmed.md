# 12 — editor's main.c entry within the cap
folder: editor/src
after: none
decisions: 0168

## Change
`editor/src/src.md`: the `main.c` entry is 346 characters; the entry cap is
300. Cut it to one sentence under 300: opens the project, window and device,
runs the loop until a close goes ahead or the picture is written, taking the
frame breakdown's timings each frame. The steps it drops (arena, font, themes,
splash line, shape upload, start log) belong in the header comment of
`editor/src/main.c`; read that header and add any dropped point missing there.
No code changes.

## Done when
`checks.sh --folder editor/src` reports no finding on `src.md`'s `main.c`
entry.
