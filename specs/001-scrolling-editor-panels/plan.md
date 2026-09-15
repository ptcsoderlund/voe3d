# 001 Scrolling editor panels — plan

The design is decided in ADR-0153 and is built bottom-up in four tasks. `platform` reports the
wheel in notches; `ui` learns to clip a container per axis and to take a clamped scroll offset in
layout; `ui` then gains a scroll area widget that remembers its offset, draws a scrollbar over its
content and takes a scroll length from the pointer; finally the editor makes every non-picture dock
leaf a scroll area, makes the inspector's field rows fill and wrap, and turns the wheel into
millimetres. Wrapping itself (`wrap`, the X-then-Y layout pass) was already built before this
feature and is only used here.

## Decisions

- Overflow is opt-in: a container may wrap or clip, and a scroll area remembers its offset —
  zeroed means today, clip and offset in layout, remembered offset in a widget table, bar over the
  content, scroll by a length passed outward, wheel in notches in `platform`, the editor opts in.
  Project-wide: `decisions/0153-overflow-is-opt-in-a-container-may-wrap-or-clip-and-a-scroll-area-remembers-its-offset.md`.
- `voe_ui_scroll_by` and `voe_ui_scroll_reveal` (ADR-0153 point 9) are not built — they have no
  caller until gamepad focus exists (CLAUDE.md rule 10). The pointer's scroll goes through one
  internal function that `voe_ui_scroll_by` will later expose.
- A wrap is decided in `arrange`, so the `measure` that fed the wrapping container's ancestors
  has already run and every one of them keeps its pre-wrap size. Found in review, fixed in task 5
  by a corrective measure-only sweep after both passes, which re-clamps each container's offset and
  shifts its children by the difference. It does not change the X-then-Y order and resizes nothing,
  so ADR-0153 stands as written and no new record is needed; `layout.c`'s header carries the
  reasoning, as it does for the passes themselves.
- Windows' wheel is written and not verified — Linux first, ADR-0130; Windows problems arrive as
  bug reports.

## Folders

- `platform/` — changed — adds `voe_platform_wheel` and `voe_platform_input_wheel`.
- `ui/` — changed — adds `voe_ui_overflow_kind`, `voe_ui_overflow`, `voe_ui_container.overflow`,
  `voe_ui_container.scroll`, `voe_ui_node_visible`, `voe_ui_node_scroll`,
  `voe_ui_capacities.scrolls`, `voe_ui_scroll_axes`, `voe_ui_scroll_begin`,
  `voe_ui_pointer.scroll`. All additive; a zeroed value means today's behaviour.
- `editor/` — changed — internal only.

## Verification

- `cmake -P check.cmake` — exits 0 on Linux. Covers criterion 9, and every unit case in tasks 1–3.
- After task 2 and again after task 3, by hand on Linux: run the editor
  (`./build/debug/editor/voe_editor`); it looks as it did before the task. No editor code changed
  in either.
- Task 1's temporary wheel print in the editor, one notch towards you reads `y = 1` and a sideways
  tilt reads a non-zero `x` (output is in task 1's report). Covers criterion 7's sign on Linux,
  together with step 4 below.
- By hand on Linux after task 4, run the editor and select an entity with a transform:
  1. At the default window size, every number box is inside the Inspector column. — criterion 1
  2. Narrow the window until a vector field's boxes fold under its label; widen it and they return
     to one line. Save a screenshot of the folded state as
     `specs/001-scrolling-editor-panels/inspector-wraps.png`. — criterion 2
     (Both screenshots were captured before task 5 and show the phantom horizontal bar; task 5
     replaces them.)
  3. Narrow further until a single box is wider than the column: a horizontal bar appears, and
     scrolling sideways (a wheel tilt, or dragging the horizontal thumb) reaches the box. —
     criteria 3 and 7
  4. Make the window short enough that the inspector overflows: one notch towards you brings the
     content below up into view, the thumb drags it, a press on the track pages it by one column
     height, and dragging a number box still edits the value. Save a screenshot as
     `specs/001-scrolling-editor-panels/inspector-scrolls.png`. — criteria 4 and 7
  5. Scroll to the bottom, then select an entity with fewer components: the column shows its top
     content with no empty space above. — criterion 5
  6. The wheel over a scene view does nothing; the views still draw their picture and a
     middle-button drag still moves their camera. — criterion 6
  7. Make the window short enough that the Scene list has more entities than fit: it scrolls with
     the wheel and its scrollbar the same way. — criterion 8
