# tests

One plain C program per `platform` module, found by the build, each checking
that module's promises from outside. None of them needs a window or a display.

- `clock.c` — that the clock moves and never goes backwards. Its header says
  why nothing in it measures a duration against a duration.
- `file.c` — that the bytes written come back byte for byte on both sides of the API, that a shorter
  file replaces a longer one, that a missing path or a folder fails a read as UNAVAILABLE, that no
  `.partial` sibling outlives a successful write, and that a write onto a folder is REFUSED.
- `folder.c` — that a listing is sorted by byte order with folder and hidden answered correctly,
  that a missing folder fails as UNAVAILABLE, that creating over an existing name is REFUSED, and
  that the settings folder honours `XDG_CONFIG_HOME` on Linux.
- `input.c` — that a poll drains the mouse's motion, wheel and typed text and keeps held keys and
  the pointer, that losing focus releases every key and losing the pointer every button, and that a
  code point past capacity or a control code types nothing, and that the pointer's shape survives
  a poll and both losses.
- `keymap.c` — a hand-written keymap text checked key by key, and a second copied from a real
  compositor's own spelling, for both keysym spellings, the levels, AltGr, aliases, dead keys and
  the two refusals.
- `scale.c` — that 120 is the identity, that 150 turns 1536×864 into 1920×1080
  and 100.5 into 125.625, that a length at 180 rounds 1 up to 2, and that zero
  stays zero.
- `path.c` — join, parent and name on ordinary paths, roots and trailing
  separators, for the platform it runs on; that resolving "." to an absolute
  path is idempotent, and that resolving a made-up name is NULL.
