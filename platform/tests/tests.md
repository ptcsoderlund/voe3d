# tests

One plain C program per `platform` module, found by the build, each checking
that module's promises from outside. None of them needs a window or a display.

- `clock.c` — that the clock moves and never goes backwards. Its header says
  why nothing in it measures a duration against a duration.
- `file.c` — that bytes round-trip, a shorter file replaces a longer one, a failed read or
  write fails as it says, no `.partial` outlives a write, and a non-ASCII name round-trips.
- `folder.c` — that listings are byte-sorted with folder and hidden right, failures are
  UNAVAILABLE or REFUSED, settings honours `XDG_CONFIG_HOME`, and non-ASCII names list exactly.
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
  with its name byte for byte; that the program's own path is an absolute file named for this test.
- `library.c` — that a library no system has opens as NULL with a report naming it.
- `sound.c` — that NULL is accepted, and with a device that room stays within the queue and
  writing that much silence succeeds; skipped, and said, without one.
- `arguments.c` — that a hand-made argv of three strings, one non-ASCII, comes back with count 3,
  the same bytes and a NULL after the last. Linux only by construction.
- `process.c` — that `cmake -E true` ends with 0 and `-E false` with non-zero, that a running
  `-E sleep 30` is ended within five seconds and zeroed, that a missing program is a false start,
  and on Linux that an output file holds both streams and is appended to.
