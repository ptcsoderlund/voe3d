# editor

The program a person opens to author a scene, runs with `--capture` to
write one frame to a PNG with no window, or with `--check` to read a
project's files strictly. A leaf like `dev`: it names what it
needs and nothing names it (ADR-0121). It links `audio` so a project library
that plays sounds loads. A gap in an engine folder is a card there, never a
reach-around from here.

- `src` — the implementation: the loop, the project and its session, undo,
  files changed on disk taken in or asked about (0506), a project's Docs, the
  themes, the top bar, the browser, Preferences, the dock and the four panels;
  each file is listed on `src/src.md`.
