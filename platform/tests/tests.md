# tests

One plain C program per module, found by the build, checking what the API
promises on the platform it runs on. None of them needs a window or a display.

- `input.c` — that a poll drains the mouse's motion, wheel and typed text and
  keeps held keys and the pointer, that losing focus releases every key and its
  typed text and nothing else, that losing the pointer releases every button and
  keeps its last position, that a code point past capacity is dropped whole, and
  that a control code point types nothing. Needs no window and no display.
- `keymap.c` — a hand-written keymap text checked key by key: an ordinary key,
  one with a type statement beside its `symbols[Group1]`, a digit and its
  shifted punctuation, a key with one level, a dead key that types nothing, a
  `U`-named code point, an alias, and that text with no `xkb_symbols` block is
  refused. Needs no window and no display.
- `file.c` — that the bytes written come back byte for byte on both sides of the
  API, that a shorter file replaces a longer one, that an empty file reads as
  zero bytes, that a missing path or a folder fails a read as UNAVAILABLE, that
  no `.partial` sibling outlives a successful write, and that writing to a path
  that is itself a folder fails the rename as REFUSED without touching the
  folder. Reads and writes with stdio as the oracle on purpose; needs no window
  and no display.
- `clock.c` — that the clock moves and never goes backwards. Its header says why
  nothing in it measures a duration against a duration.
- `folder.c` — that a listing is sorted by byte order with folder and hidden
  answered correctly, that an empty folder lists zero, that a missing folder
  fails a listing as UNAVAILABLE, that creating over an existing name is REFUSED
  and under a missing parent UNAVAILABLE, and that the settings folder honours
  `XDG_CONFIG_HOME` on Linux. Needs no window and no display.
- `path.c` — join, parent and name on ordinary paths, roots and trailing
  separators, for the platform it runs on; that resolving "." to an absolute
  path is idempotent, and that resolving a made-up name is NULL. Needs no window
  and no display.
