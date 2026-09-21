# editor

The program a person opens to author a scene, and the same program run with
`--capture` to write one frame to a PNG with no window at all. It is a leaf
and it stays one, exactly as `dev` is: it names whatever it needs and nothing
names it (ADR-0121). No engine folder gains anything for the editor's sake —
a gap in one of them is a card in that folder, never a reach-around from here.

- `src` — the implementation: the loop, the project and its session, undo,
  the themes, the top bar, the browser, Preferences, the dock and the three
  panels; each file is listed on `src/src.md`.
