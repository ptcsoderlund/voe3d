# 072 — `ui` lays out X then Y, and a container may wrap

claimed-by: claude-opus-5 (session a10ca260)
status: review
blocked-by: -
decision: *Overflow is opt-in: a container may wrap or clip, and a scroll area remembers its offset* (ADR-0153) points 1–4.

## Goal

A row or column told to `wrap` puts children that do not fit onto further lines, as flexbox's
`flex-wrap: wrap` does. To make that possible, layout resolves every width in the tree before any
height. A container that does not ask to wrap lays out exactly as it does today, rectangle for
rectangle.

## Scope

**1. `ui/include/ui/layout.h`: one field.**

```c
typedef struct {
	...
	voe_ui_anchor anchor;
	bool wrap;          // nought: one line, as today
} voe_ui_container;
```

**2. `ui/src/layout.c`: two axis passes in `voe_ui_frame_end`.**

Measure X bottom-up and arrange X top-down for the whole tree, then measure Y bottom-up and
arrange Y top-down. Every rule that exists today (natural, fixed, grow, the four `along` values,
`across` with FILL and the fixed-beats-FILL rule, padding, gap, anchored children, degenerate
SPREAD and EVENLY) keeps its current result. Anchored children never take part in lines.

**3. Wrapping, along the flow.** For a container with `wrap` set:

- **Breaking.** Walk in-flow children in call order, adding each child's natural length along
  the flow and a `gap` before every child but a line's first. When a child would take the line past
  the container's inner length, it starts a new line. **The first child on a line never breaks**:
  a child longer than the whole line has that line to itself, at its full size, sticking out.
- **Lines break when the container's length along the flow is known.** In a row that is the X
  arrange; in a column it is the Y arrange.
- **Each line is laid out as today's single run**, against the inner length: grow children share
  what is left *of that line*, and `along` distributes within that line.
- **Across the flow**, a line's thickness is its thickest child's natural size across. Lines stack
  from the start of the axis with `gap` between them. **Space left over across the container,
  after the lines and their gaps, is shared equally between the lines**, each line growing by the
  same amount; `across` then places each child within its line, FILL stretching to the line's
  thickness. With one line this is exactly today's layout.
- **A natural length along the flow is one line long**, so such a container never wraps. Not an
  error.
- **A wrapping row's height is its lines**: its Y measure is the sum of line thicknesses and gaps,
  plus padding. That is why X runs first.
- **A wrapping column breaks after its width is settled.** Its X measure is one column, and the
  extra columns it breaks into during the Y arrange sit to the right of its rectangle, outside it.

**4. `voe_ui_node_measured` of a wrapping container** is its content after wrapping: the longest
line along the flow, and every line and the gaps between them across it, plus padding. For every
other node it is unchanged.

**5. The header**, `ui/include/ui/layout.h`:
- *NOTHING IS LAID OUT UNTIL voe_ui_frame_end*: say it measures and arranges X for the whole tree,
  then Y, and that nothing may need a height to know a width.
- *OVERFLOW IS NOT SHRUNK*: one sentence — a container may wrap instead; see `wrap`.
- A `wrap` paragraph: along the flow; the first child on a line never breaks; grow and `along` are
  per line; space across shared between lines; natural length never wraps; **a wrapping column does
  not widen as it wraps, and its extra columns overflow its rectangle to the right** (ADR-0153
  point 3).
- `voe_ui_node_measured`: its wrapped meaning.

**6. `ui/tests/layout.c`** — the new cases, each asserting exact rectangles in millimetres.

- **Breaking**: a row fixed at 50 along, gap 2, five boxes of 20 → lines of two, two and one;
  measured along 42, across three box heights and two gaps.
- **A child too long**: a 70 box among 20s in a 50 row is alone on its line, 70 wide, rect's max
  past the container's.
- **Grow per line**: a line holding a fixed 20 and a grow child in a 50 row gives the grow child 28.
- **Along per line**: CENTER centres each line on its own.
- **Across**: children of different heights — each line as thick as its tallest; FILL stretches
  to the line; a row fixed tall with two lines gives each line half the spare height.
- **One line is today**: a tree with `wrap` whose children fit gives every rectangle identical to
  the same tree without `wrap`.
- **Natural never wraps**: a natural-length wrapping row holding five 20s is one line.
- **X before Y**: a column fixed at 50 wide with FILL across, holding a wrapping row of five 20s
  and a box beneath it — the row breaks into three lines, and the box sits under the third.
- **A wrapping column**: fixed 50 tall, five boxes 20 tall — three columns, the second and third
  to the right of the column's own rectangle; measured across is three widths and two gaps.
- **Anchored children**: an anchored child of a wrapping row is placed as today and breaks nothing.
- **Nesting**: a wrapping row inside a wrapping row.

## What must not change

- **Every existing test in `ui/tests/layout.c` and `ui/tests/widgets.c` passes unedited.** A test
  that needs an edit is a finding for Notes, not a fix.
- No other public call or field in `ui`. No clipping — card 073. No widget.
- No change outside `ui/`, except `ui/ui.md` if its description of layout names a single pass.
- ADR-0095's vocabulary: no shrink, no margin, no `wrap-reverse`, no line-alignment setting.

## Verify

- Linux: `cmake -P check.cmake` green; `ctest -R ui` passes.
- `git diff --stat -- ui/tests` shows additions to `layout.c` only.
- The editor runs and looks as it did before the card (no editor code changed).
- From the planning root, `bash tools/hot.sh` reports no new `OVER`.

## Done looks like

`.wrap = true` on a row of fixed length folds its children into lines, a single too-long child
gets a line of its own, and nothing that did not ask to wrap moved by a millimetre.

## Notes

**Done 2026-09-13, verified on Linux (Fedora 44, clang, RTX 4070). Not checked on Windows.**

- `voe_ui_frame_end` now measures and arranges X for the whole tree, then Y. Each child records
  which `line` of its parent's run it is on (`src/context.h`), and a container that does not wrap
  keeps every child on line nought: one run, the same arithmetic as before.
- **A question came up, and the principal answered it when asked.** In a column that breaks into
  several columns, does a FILL child stretch to its column's thickness (point 3), or does "nothing may
  need a height to know a width" (point 5) win? **Point 5 wins.** A wrapping column's widths are set
  in the X pass as one column. The break in the Y pass only *moves* each child, and everything inside
  it, to its column. It never resizes one. So a FILL child keeps its one-column width. This is written
  in `layout.h`, in `arrange_across` in `layout.c`, and pinned by the second half of `wrap_column`.
  **ADR-0153 point 3 may want that sentence.**
- One line is exactly today's layout, bit for bit. A single line's thickness is the container's inner
  size across, not its thickest child's. That keeps FILL and CENTER unchanged when a child overflows
  across. `wrap_one_line_is_today` checks this with zero tolerance.
- "A natural length never wraps" is exact. A run whose arranged length is at least its measured length
  is never broken, so adding up fractional lengths a second time cannot break it by rounding.
- Padding is still added in one place and subtracted in one: `measured_set`, and
  `inner_min`/`inner_size`.
- A wrapping column moves each child's subtree, so a node under several nested wrapping columns is
  moved once per column. That is the one pass that is not strictly linear. The file header says so.

**Verified:**
- `cmake -P check.cmake` exits 0: all standalone configures, 47 tests, analyser on 129 files.
- `ctest -R ui`: 2/2 pass, and every existing `layout.c`/`widgets.c` case passes without edits.
- `git diff --stat -- ui/tests`: `layout.c` only, 528 insertions, 0 deletions.
- Probe: with breaking disabled, `wrap_breaks` fails (height 6, expected 22). Reverted, and it passes.
- `bash tools/hot.sh`: all hot files under their ceilings (`ui.md` 79/120, this card 144/150).
- Editor: started on the real GPU and ran 6 s with nothing on stderr. Nothing captures a frame, so
  **whether it looks the same was not checked by eye.** That is left for review.

**Files touched:** `ui/include/ui/layout.h`, `ui/src/context.h`, `ui/src/layout.c`,
`ui/tests/layout.c`, `ui/ui.md`. Nothing outside `ui/`. No DEVIATION or BLOCKED markers.
