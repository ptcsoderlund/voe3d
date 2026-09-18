# text

The public headers, one entry each; the fuller account of both of these stays on
`text/text.md`.

- `font.h` — the whole public surface: the font made once, a string turned into
  one mesh and one sheet, or one character's box, sheet rectangle and advance for
  a caller placing characters itself.
- `utf8.h` — one character out of a UTF-8 string, and how many bytes it was, for
  a caller that walks a string the way this folder does.
