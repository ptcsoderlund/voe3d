# 15 — The sliders take effect and are remembered
folder: editor
decisions: 0168, 0177, 0194, 0197
read: feature.md

## Change
`editor/src/interface.c`, in the block that reads Preferences' result:
- `VOE_EDITOR_PREFERENCES_ADJUST`: `voe_editor_themes_adjust(themes, themes->chosen, result.contrast,
  result.separation)` — the chosen entry's palette is derived again where it stands, so the next frame draws
  with it and nothing needs setting on the context.
- `VOE_EDITOR_PREFERENCES_RESET`: `voe_editor_themes_reset(themes, themes->chosen)`.
- After the branches, while Preferences is showing and `!result.sliding`: `if
  (!voe_editor_themes_scalars_write(themes)) voe_editor_notice_set(&session->notice, "the slider values
  could not be remembered");` — the write happens when a drag ends and on Reset, and does nothing when
  there is nothing unwritten.
- The comment above the block says the sliders move the theme in force as they are dragged and are
  remembered per theme when the drag ends (ADR-0197).

`editor/editor.md`: the Preferences paragraph says it also shows contrast and surface separation for the
theme in force, that dragging one changes the whole editor as it moves, that what is set is remembered per
theme in `<settings>/voe3d/theme_scalars` and not in the theme file, and that Reset puts the theme's own
values back. The themes paragraph says a theme file authors one `hue=` and that a file still saying
`accent=` is refused with its line named (ADR-0194).

## Done when
`checks.sh --all` exits 0, and the human, running `voe_editor`, sees every step of `feature.md`'s
`## How to test` hold: the selected row, a held button and a dragged number box inverted and grey only in
Near black and Near white; an `amber.theme` with `hue="#D4A02B"` turning the whole editor amber with the
same marks; that file with `accent=` instead drawing on and showing a notice naming the file and the line;
both sliders moving text and surfaces as they are dragged and staying readable at either end; the theme and
both sliders as they were left after a restart with `amber.theme` unchanged on disk; and Near black showing
its own slider values, which Reset puts back.

## Blocked
The `## Change` is done and `checks.sh --folder editor` is `FINDINGS: 0`, but `checks.sh --all` exits 1
on `ui/include/ui/ui.md: does not list `slider.h``, a defect left by card 11 (a4e63f6) in another
folder, so this card's `## Done when` cannot be reached from `editor`. One `- `slider.h` — sentence `
entry added to `ui/include/ui/ui.md` in a `ui` card unblocks it; the human walk through
`feature.md`'s `## How to test` was not seen either, since it needs a person at the running editor.
