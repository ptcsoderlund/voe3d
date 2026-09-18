# tests

One plain C program per `platform` module, found by the build, each checking
that module's promises from outside. None of them needs a window or a display.

- `clock.c` — that the clock moves and never goes backwards. Its header says
  why nothing in it measures a duration against a duration.
- `file.c` — that the bytes written come back byte for byte on both sides of the
  API, that a shorter file replaces a longer one, that an empty file reads as
  zero bytes, that a missing path or a folder fails a read as UNAVAILABLE, that
  no `.partial` sibling outlives a successful write, and that writing to a path
  that is itself a folder fails the rename as REFUSED without touching the
  folder. Reads and writes with stdio as the oracle on purpose.
- `folder.c` — that a listing is sorted by byte order with folder and hidden
  answered correctly, that an empty folder lists zero, that a missing folder
  fails a listing as UNAVAILABLE, that creating over an existing name is REFUSED
  and under a missing parent UNAVAILABLE, and that the settings folder honours
  `XDG_CONFIG_HOME` on Linux.
- `input.c` — that a poll drains the mouse's motion, wheel and typed text and
  keeps held keys and the pointer, that losing focus releases every key and its
  typed text and nothing else, that losing the pointer releases every button and
  keeps its last position, that a code point past capacity is dropped whole, and
  that a control code point types nothing.
- `keymap.c` — a hand-written keymap text checked key by key: an ordinary key,
  one with a type statement beside its `symbols[Group1]`, a digit and its
  shifted punctuation, a key with one level, a dead key that types nothing, a
  `U`-named code point, an alias, and that text with no `xkb_symbols` block is
  refused. A second text, copied from a real compositor's own spelling, checks
  `0x` keysym values, a Group index written `1`, an unindexed `type=`, AltGr as
  the keysym `0xfe03` on two keys, the `0x0100xxxx` Unicode form, a key with no
  resolved code, and that a keymap resolving to nothing is refused too.
- `path.c` — join, parent and name on ordinary paths, roots and trailing
  separators, for the platform it runs on; that resolving "." to an absolute
  path is idempotent, and that resolving a made-up name is NULL.
