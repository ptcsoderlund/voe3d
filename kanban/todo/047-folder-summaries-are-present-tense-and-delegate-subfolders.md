# 047 — Folder summaries become present-tense maps, and `render` delegates two subfolders

claimed-by: -
blocked-by: -
status: todo
decision: *A folder summary is a present-tense map, and it delegates its subfolders* (ADR-0120) — a folder summary describes the folder as it is now, never how it got there; a crowded subfolder takes its own summary and the parent keeps one pointer line, as `render/vulkan/` already does.

## Goal

`render/render.md` and `dev/dev.md` are both under 120 lines and hold no card
history. Two new summaries exist: `render/tests/tests.md` and
`render/shaders/shaders.md`. `sh tools/hot.sh` at the planning root reports no
`OVER`.

## Scope

**1. Delegate two subfolders of `render`.**

- Create `render/tests/tests.md`. Move into it, verbatim, the six `tests/…` entries
  currently at `render/render.md` lines 135–171, and give it a short preamble
  saying what the folder is for and what its reader is doing there — model it on
  `render/vulkan/vulkan.md`, which is the existing precedent.
- Create `render/shaders/shaders.md` the same way from the three `shaders/…`
  entries at lines 172–193.
- In `render/render.md`, replace each moved block with one line naming the
  subfolder and pointing at its summary, in the shape of the existing
  `` - `vulkan/` — … See `vulkan/vulkan.md`. `` line.

**2. Fold the card-keyed changelog into present tense — two files.**

- `render/render.md`, the `include/render/device.h` entry (lines 22–63). Lines
  31–63 are past-tense clauses of the form *"Card NNN added X; its header says…"*.
  Rewrite the entry so it describes the header as it stands today, in one
  present-tense list, with no card numbers in it. **Nothing described may be
  dropped** — every *its header says…* clause names something the header still
  explains, and those survive; only the *Card NNN added* framing goes.
- `dev/dev.md`, the `src/main.c` entry, which carries the same pattern.
- Then sweep both files for any remaining `Card NNN` reference and fold it the same
  way. There are 11 in `render.md`.

## What must not change

- **No code, no headers, no shaders, no tests.** This card touches `.md` files only.
- The *"its header says…"* house style stays — it is used across every folder
  summary and is not what this card removes.
- The preamble of `render/render.md` (lines 1–20) is correct and stays as it is.
- `render/vulkan/vulkan.md` and its one-line reference are untouched; they are the
  pattern being copied.
- No other folder summary is edited. `3d`, `ui`, `text` and `scene` name no cards.
- `kanban/complete/` is where this history lives and it is append-only. Nothing is
  copied into it and nothing is deleted from it.
- No subfolder summary for `render/src/` or `render/include/`. ADR-0120 rules those
  out: a reader opening `render` needs those entries on the page they opened.

## Verify

- Linux: `cmake -P check.cmake` green. Nothing here should be able to affect it;
  if it does, that is the finding.
- `grep -rn 'Card [0-9]' render/render.md dev/dev.md` returns nothing.
- From the planning root, `sh tools/hot.sh` reports no `OVER` and `render/render.md`
  is comfortably under 120, not at 119.
- Read `render/render.md` end to end afterwards and confirm it still answers, on its
  own, what the folder is for and what each file in `src/` and `include/` does.

## Done looks like

A coder opening `render/` gets one page that describes the folder as it is today,
with `tests/` and `shaders/` one line each behind their own summaries. Nothing a
header explains has stopped being mentioned; only the history of which card added
it has gone, and that is in `kanban/complete/`.

## Notes
