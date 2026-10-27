# 01 — The draw shader sheds its capture and sky entries
folder: render
after: none
decisions: 0168

## Change
`render/shaders/draw.slang` is 804 lines and card 03 adds to it. Move code only; no behaviour
changes, every entry point keeps its name and its SPIR-V.

- `render/shaders/draw_capture.slangh` — new part: the capture pass's two entry points (about
  lines 660–750 of `draw.slang`) and whatever only they use. Its header: what the capture pass
  draws, that it is a part of `draw.slang` included at its end after everything it calls, never
  compiled alone (ADR-0308).
- `render/shaders/draw_sky.slangh` — new part, the same for the sky's two full-screen entry
  points (about lines 751–804).
- `render/shaders/draw.slang` — the moved code gone, the two parts included at the end, its
  header's list of entry points says where the capture and sky ones now live.
- `render/shaders/shaders.md` — an entry for each new part; the `draw.slang` entry says its
  capture and sky entries are in them.

Parts in `render/shaders/` are found by the build's glob; no build file changes.

## Done when
`render` builds, `wc -l < render/shaders/draw.slang` prints under 700, and the existing render
tests pass (the folder's checks run them).
