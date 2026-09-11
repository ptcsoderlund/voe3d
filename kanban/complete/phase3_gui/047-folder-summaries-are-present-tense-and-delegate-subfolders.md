# 047 — Folder summaries become present-tense maps, and `render` delegates two subfolders

claimed-by: claude-opus-5
blocked-by: -
status: review
decision: *A folder summary is a present-tense map, and it delegates its subfolders* (ADR-0120) — a folder summary describes the folder as it is now, never how it got there; a crowded subfolder takes its own summary and the parent keeps one pointer line, as `render/vulkan/` already does.

## Goal

`render/render.md` and `dev/dev.md` are both under 120 lines and hold no card
history. Two new summaries exist: `render/tests/tests.md` and
`render/shaders/shaders.md`. `sh tools/hot.sh` at the planning root reports no
`OVER`.

> **Amended in session, 2026-09-11.** There are **three** new summaries, not two:
> the principal lifted the prohibition below and `render/src/src.md` exists as
> well. See *What must not change* and the Notes.

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
  > **Lifted for `render/src/` by the principal, in session, 2026-09-11**, after
  > the card was worked and the ceiling was reported unreachable without it.
  > `render/src/src.md` now exists. `render/include/` is untouched and the
  > `include/render/device.h` entry stays on `render.md` where the card put it.
  > **ADR-0120 still says the opposite and is the tech lead's to amend** — this
  > line records the decision, it does not make it.
  >
  > **Amended, 2026-09-11, tech lead: ADR-0133.** The prohibition is withdrawn and
  > replaced by what it was protecting — a folder page answers what the folder is
  > and what its public surface is, on its own, and may delegate the rest when they
  > crowd it. `render/include/` stays on the parent page by that rule. The tree and
  > the record agree; nothing in this card needs redoing.

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

**Done.** Four folder summaries where there was one, and `hot.sh` says *all hot
files under their ceilings*.

| | lines |
|---|---|
| `render/render.md` — the preamble, the public surface, four pointers | 68 / 120 |
| `render/src/src.md` — 17 entries | 84 / 120 |
| `render/tests/tests.md` — 6 entries | 48 / 120 |
| `render/shaders/shaders.md` — 3 entries | 34 / 120 |
| `dev/dev.md` | 115 / 120 |

- Each new summary has a preamble modelled on `render/vulkan/vulkan.md` and the
  moved entries verbatim under it, their path prefixes dropped as `vulkan.md` has
  none. `render.md` keeps one pointer line each, in the shape of the `vulkan/` one.
- The `include/render/device.h` entry is one present-tense list, 42 lines down to
  39; `dev/dev.md`'s `src/main.c` entry 34 down to 32. Every *its header says…*
  clause survives, checked phrase by phrase against the old text — 34 in
  `device.h`, 23 in `main.c`.
- `grep -in 'card [0-9]'` returns nothing across all five files. All 11 in
  `render.md` and all 11 in `dev.md` are folded; one of `dev.md`'s spanned a line
  break, which is why the card's count said otherwise.

**Verified on Linux**, the only platform booted:

- `cmake -P check.cmake` **green** both before and after the `src/` split — every
  step ok, 39 tests, analyser clean over 106 files. Nothing here reached it, as the
  card expected. Not checked on Windows; nothing in this card is platform-specific,
  as it edits no code.
- `sh tools/hot.sh` at the planning root: **no `OVER`**.
- `render/render.md` read end to end. It answers on its own what the folder is for;
  what each file does now lives one click down, which is the amendment's cost.

**Two findings, neither touched — this card is `.md` only:**

- **`dev/src/interface.h` is stale and wants a card.** Its header still says the
  panel is pushed down the page *"with a SPACER… card 041 adds anchored children,
  and when it lands the spacer goes"*. Card 041 is in `complete/phase3_gui/`, and
  `dev/src/interface.c:42` says outright: *"It **was** a 54 mm spacer… now that it
  is the root's top padding"*. `dev/dev.md` repeated the claim; that clause now
  reads *"why its panel is pushed down the page to clear the readout rather than
  anchored where it wants to be"*, which is true of the code. The header itself is
  still wrong.
- `render/render.md` line 5 is unwrapped at ~120 columns where the file wraps at
  80, inside the preamble the card protects. `dev/dev.md` lines 10 and 12 are short
  for the same reason. Cosmetic.

**One reading, taken openly.** The card says the `tests/` block moves *verbatim*
and also that the sweep covers 11 references in `render.md` — the eleventh is
`card 018`, inside that block. Both cannot hold, so I folded it in `tests/tests.md`
(*"which two claims are not in it and where they live instead"*): the card's own
count includes it, and ADR-0120 says a folder summary describes the folder as it
is now, which `tests/tests.md` is.

**This impeaches no card.** Nothing outside `.md` was edited and no marker was
left in code. `dev/dev.md` was also normalised back to LF — the tree's CRLF noise
had left it mixed — so its diff is exactly the 32/34 lines this card changed.
