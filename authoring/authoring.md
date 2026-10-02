# authoring

A world turned into a scene file and back (ADR-0151; format ADR-0149, content
ADR-0150), a world cooked into C source for the game (ADR-0237), and a project
file read and written (ADR-0164). Authoring-time code:
components go through their field descriptions, which a game's build compiles
out (ADR-0145); only the reader creates rows (ADR-0152); `platform` owns files.

- `include` — the public headers, in `include/authoring/`; each is listed below by path.
- `src` — the implementation. See `src/src.md`.
- `tests` — one plain C program per module, found by the build. See `tests/tests.md`.
- `include/authoring/scene_write.h` — a world written as scene text, handed back
  in one `voe_authoring_text` out-struct. Its header says what is written and in
  what order; the file says how every field kind is spelled and what it refuses.
- `include/authoring/prefab.h` — one tree written as a `.prefab` file's scene text,
  and read back onto a placed root. Its header says what a prefab file is, why the
  root sits at the origin, and why a read may add rows.
- `include/authoring/scene_read.h` — scene text read into a world. Its header says
  why a load may create rows, that a file is validated whole first, what a missing
  field reads as, and what is kept rather than read; the file says what a world
  that runs out of room is left as.
- `include/authoring/scene_cook.h` — a world cooked into C source the game compiles in.
  Its header says what is cooked, why floats are hex and why the cooked function may
  create rows; the file says how every field kind is spelled and what it refuses.
- `include/authoring/project.h` — `project.voe3d` read and written. Its header
  says the scene key and the optional `[window]` section with its range and defaults,
  what it refuses, and why the writer cannot fail.
