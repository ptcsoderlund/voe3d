# tests

One plain C program per module, found by the build. The quad has no test of its
own; what it promises is checked where a sprite is drawn.

- `sheet.c` — the four corners of a four-by-two sheet, a short last row, and that
  framing a material touches nothing else in it. Its header says why the sheet is
  not square. Needs no graphics card.
