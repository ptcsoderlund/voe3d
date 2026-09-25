# tests

One plain C program per `platform` module, found by the build, each checking
that module's promises from outside. None of them needs a window or a display.

- `clock.c` — that the clock moves and never goes backwards. Its header says
  why nothing in it measures a duration against a duration.
- `file.c` — that the bytes written come back byte for byte on both sides of the API, that a shorter
  file replaces a longer one, that a missing path or a folder fails a read as UNAVAILABLE, that no
  `.partial` sibling outlives a successful write, that a write onto a folder is REFUSED, and
  that a non-ASCII name is written, found and read back.
- `folder.c` — that a listing is sorted by byte order with folder and hidden answered correctly,
  that a missing folder fails as UNAVAILABLE, that creating over an existing name is REFUSED, and
  that the settings folder honours `XDG_CONFIG_HOME` on Linux, and that non-ASCII names list byte for
  byte.
- `input.c` — a poll drains motion, wheel and typed text and keeps held keys and the pointer;
  focus loss releases keys, pointer loss buttons; an overflowing or control code point types
  nothing; the pointer's shape survives a poll and both losses.
- `keymap.c` — a hand-written keymap text checked key by key, and a second copied from a real
  compositor's own spelling, for both keysym spellings, the levels, AltGr, aliases, dead keys and
  the two refusals.
- `scale.c` — that 120 is the identity, that 150 turns 1536×864 into 1920×1080
  and 100.5 into 125.625, that a length at 180 rounds 1 up to 2, and that zero
  stays zero.
- `path.c` — join, parent and name on ordinary paths, roots and trailing
  separators, for the platform it runs on; that resolving "." to an absolute
  path is idempotent, that resolving a made-up name is NULL, and that a non-ASCII folder resolves
  with its name byte for byte.
- `library.c` — that a library no system has opens as NULL with a report naming it.
- `arguments.c` — that a hand-made argv of three strings, one non-ASCII, comes back with count 3,
  the same bytes and a NULL after the last. Linux only by construction.
- `process.c` — that `cmake -E true` ends with 0 and `-E false` with non-zero, that a running
  `-E sleep 30` is ended within five seconds and zeroed, that a missing program is a false start,
  and on Linux that an output file holds both streams and is appended to.
