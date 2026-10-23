# 0460 — A kind's base size is thousandths in the landscape, and a stroke in undo
date: 2026-10-22
by: planner

## Decision
For 090 bug 10, carrying out 0469.

1. **In the file:** `[Foliage]` gains `size<k>=` beside `kind<k>=`, a whole number of
   thousandths from 100 to 10000 (0.1× to 10×), written only when kind k is not 1×; a missing
   key is 1×. Anything else is MALFORMED.
2. **In the struct:** `voe_assets_landscape` gains `uint16_t kind_sizes[VOE_ASSETS_LANDSCAPE_KINDS]`
   in thousandths, **0 read as 1×**, so a flat or hand-built grid stays valid; one call,
   `voe_assets_landscape_kind_size`, turns it into a factor, and nothing else reads the array
   for arithmetic. A kind added starts at 0.
3. **Placed:** a copy's placed scale is its kind's base size × its own scale. Changing a kind's
   base size places that kind's copies again in the store and raises its change count, so the
   bounce relights as for any foliage change.
4. **In the game:** `voe_game_landscape` gains `const uint16_t *kind_sizes` last, kind_count of
   them, NULL every kind 1×; the editor cooks them beside the kinds.
5. **In the editor:** each kind row has a base-size slider over exactly point 1's range, 0.1× to
   10×, figure `2.00×`, typed or dragged. It resizes live while held; one undo step per drag or
   typed value, pushed when the slider lets go, carried as a stroke (landscape path, kind,
   before, after) and applied through the editor's model store; the project is marked edited.

## Reasoning
- Thousandths, as a copy's scale is: the text is exact and short, and read back equals written.
- The file's range equal to the slider's: `voe_ui_slider_action` reports a clamp as a change,
  so a stored value outside the slider would turn merely showing the row into an edit.
- 0 as 1×: as NULL paint is all layer 0, a struct built in code needs no setting it may forget.
- A stroke rather than a new step kind: strokes already ride beside the scene text for a
  landscape's edits and write back through the store; a material step writes a file at once,
  which a landscape is not (it is saved with the project).

## Replaces
nothing. Carries out 0469 points 1 and 3.
