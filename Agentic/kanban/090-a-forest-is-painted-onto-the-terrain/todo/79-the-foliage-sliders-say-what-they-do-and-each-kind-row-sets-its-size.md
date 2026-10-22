# 79 — The foliage sliders say what they do, and each kind row sets its size
folder: editor
after: 78
decisions: 0168, 0379, 0442, 0452, 0459, 0460

## Change
0459 points 1 and 3, 0460 point 5: the Foliage section's sliders are renamed and each kind
row gets a base-size slider whose change is live and one undo step.

- `editor/src/inspector_foliage.c`, `editor/src/inspector_foliage.h` —
  - The four sliders read Density, **Size ±%**, **Turn ±°** and **Tilt with slope %**; the
    numbers, ranges, defaults and figures do not change.
  - Draw: each kind row is followed by its base-size slider, drawn as the section's other
    sliders, over VOE_ASSETS_LANDSCAPE_KIND_SIZE_MIN–_MAX / 1000, its value
    `voe_assets_landscape_kind_size` of the store's grid, its figure `%.2f×`.
  - Read: a kind slider `changed` records the ask in the sculpt state (kind, value rounded to
    thousandths); `held` on any kind slider is recorded too. Forget sets the kind sliders to
    VOE_UI_NODE_NONE.
  - Header: the slider names and what each does (Size ±% from the kind's base size, Turn ±° a
    random spin, Tilt with slope % from upright to square to the ground); the kind row's
    base-size slider and that it resizes that kind's copies already painted, live, as one undo
    step.
- `editor/src/sculpt.h` — beside the Foliage nodes: `kind_sliders` and their figures, one per
  kind; the ask (`size_asked`, `size_kind`, `size_to`, `size_held`) and the open edit
  (`sizing`, its kind, its size before and its landscape path). Header: the constraints line
  names the kind sliders' range; a sentence that a size edit is settled each frame, brush or
  none.
- `editor/src/sculpt.c` (486 lines; if it passes ~550, the settle goes in
  `editor/src/sculpt_sizes.c` with its header, declared in `sculpt.h`) — `voe_editor_sculpt_start`
  clears the new fields. At the top of `voe_editor_sculpt_read`, before it looks at the brush:
  an ask opens an edit when none is (path: the selection's landscape, found as a press finds
  it; before: its kind's stored size) and sets the size through
  `voe_editor_models_kind_size_set`; an open edit with no kind slider held is closed: when its
  size changed, `voe_editor_stroke_new_kind_size` handed to the undo line as a sculpt stroke
  is, and the project marked edited. A selection with no loaded landscape drops the ask. The
  comment above `_read` says so.
- `editor/src/src.md` — the `inspector_foliage.*`, `sculpt.*` (and new `sculpt_sizes.c`)
  entries follow; each ≤ 300 characters.

## Done when
The editor builds; `grep -n 'Tilt with slope' editor/src/inspector_foliage.c` and
`grep -n 'voe_editor_stroke_new_kind_size' editor/src/sculpt*.c` each print a line;
`grep -c '"Lean' editor/src/inspector_foliage.c` prints 0.

The human's, on lumberhill (bug 10's How to test):
1. Pick a thing that wears the landscape. Under Foliage: Size ±%, Turn ±°, Tilt with slope %.
2. Every kind row shows 1.00×; the forest looks as before.
3. Set the pine's base size to 2× (drag, or click and type 2): every pine painted doubles in
   place, same spot and heading; other kinds unchanged.
4. Size ±% at 0, scatter pines: they match the 2× ones.
5. Size ±% at 50, scatter: new pines from about 1× to 3×.
6. Tilt with slope % at 100 on a steep bit: square to the slope; at 0 upright.
7. Save, close, reopen: the pine still 2×, the forest the same.
8. Ctrl+Z the size change: the pines go back to 1×.
