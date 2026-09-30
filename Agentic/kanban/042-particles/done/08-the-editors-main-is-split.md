# 08 — The editor's main is split
folder: editor
after: none
decisions: 0168

## Change
`editor/src/main.c` is 803 lines, one `main`; card 09 changes it. Split it by
function, moving code only, no behaviour change.

- Read `editor/src/main.c`. Move the per-frame keyboard and command acts
  (from "WHICH EDGE MEANT WHICH COMMAND IS ANSWERED ONCE" through Escape's
  order and the browser's Escape) into a function in a new
  `editor/src/frame_commands.h` and `.c`, named for what it does, taking what
  it reads and writes as parameters or one struct of pointers. If that
  stretch does not lift out cleanly, take the next self-contained stretch of
  the loop that does; the goal is `main.c` under 700 lines.
- Each new file has its header comment: what the part does, when main
  calls it, and which of main.c's header paragraphs moved with it (move
  them, do not copy them).
- `editor/src/src.md` (or the folder's index that lists `main.c`): an entry
  for the new file; the `main.c` entry updated.

## Done when
`test $(wc -l < editor/src/main.c) -lt 700` exits 0, and the editor builds in
the folder's checks.
