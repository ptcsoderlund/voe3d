# 51 — Every brush is sized in a Brush section
folder: editor
after: none
decisions: 0168, 0452

## Change
0452: Radius (m) and Softness leave the Sculpt section for a new Brush section, drawn after
Foliage while any brush is chosen. The numbers stay `sculpt->radius` and `sculpt->softness`;
`sculpt.c`'s strokes, the foliage brush it builds and the view's ring do not change.

- `editor/src/sculpt.h` — `VOE_EDITOR_SCULPT_SLIDERS` and `sliders[]`/`figures[]` give way to
  the Sculpt section's one `strength_slider` and `strength_figure`, and the Brush section's
  `brush_sliders[VOE_EDITOR_SCULPT_BRUSH_SLIDERS]` (2: Radius, Softness) and `brush_figures`,
  commented as the other sections' nodes are. Header: the constraints line says the Sculpt,
  Brush and Foliage sliders clamp; names inspector_brush.h among the sections that set it.
- `editor/src/sculpt.c` — `voe_editor_sculpt_start` sets the new nodes to VOE_UI_NODE_NONE in
  place of the old loop.
- `editor/src/inspector_sculpt.c`, `editor/src/inspector_sculpt.h` — the section draws, forgets
  and reads Strength alone (its name, range, figure as now). Header: Radius and Softness are
  the Brush section's; PAINT strokes with Strength.
- `editor/src/inspector_brush.h`, `editor/src/inspector_brush.c` — new, shaped as
  `inspector_sculpt.*` (forget, draw, read; nodes in the sculpt state):
  - `void voe_editor_inspector_brush_forget(voe_editor_sculpt *)`.
  - `void voe_editor_inspector_brush_draw(voe_ui_context *, voe_editor_sculpt *)`: nothing
    while `!sculpt->chosen`; else a "Brush" heading as Sculpt's, then Radius (m) and Softness
    sliders with figures exactly as the Sculpt section drew them.
  - `void voe_editor_inspector_brush_read(const voe_ui_context *, voe_editor_sculpt *)`: each
    drawn slider's value taken into `radius` or `softness`.
  - Header points: 0452 points 1–4; the usage block as inspector_sculpt.h's.
- `editor/src/inspector_foliage.h` — Radius and Softness are the Brush section's, not Sculpt's.
- `editor/src/inspector.c` — in the block that draws Sculpt, Layers and Foliage on a thing
  that wears a landscape, the Brush section after Foliage; header line 6 names it.
- `editor/src/inspector.h` — `voe_editor_inspector_draw`'s comment: the Brush section after
  Foliage while a brush is chosen, then LOD.
- `editor/src/interface.c` — forget it beside the other three when the frame opens.
- `editor/src/interface_read.c` — read it right after the Foliage read.
- `editor/src/inspector_level.h` — if its header says the LOD section comes after Foliage, it
  says after Brush or Foliage.
- `editor/src/src.md` — entries for `inspector_brush.h` and `.c`; `inspector_sculpt.h` names
  Strength alone; `inspector_foliage.h` and `inspector_level.h` as their headers now say;
  `sculpt.h` unchanged unless its header's change makes it untrue. Each entry ≤ 300 characters
  (the whole `- `name` — ...` line), each header touched under its 60-line cap (measure with
  `bash ~/.claude/skills/checks/scripts/checks.sh --header-lines <file>`).

## Done when
The editor builds; `grep -c 'Radius (m)' editor/src/inspector_sculpt.c` prints 0;
`grep -n 'Radius (m)' editor/src/inspector_brush.c` prints a line;
`grep -rn 'VOE_EDITOR_SCULPT_SLIDERS' editor/src` prints nothing.

The human's: open a landscape in the Inspector. With no brush chosen there is no Brush section.
Choose Raise: Brush appears below Foliage with Radius (m) and Softness, Sculpt shows Strength
alone, and Raise has not moved. Pick a foliage kind and Scatter: Radius (m) in Brush sizes the
ring and the scatter.
