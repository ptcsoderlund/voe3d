# 14 — Preferences shows two sliders and Reset
folder: editor
decisions: 0168, 0177, 0194, 0196, 0197

## Change
`editor/src/preferences.h`:
- The struct gains `voe_ui_node contrast_slider`, `voe_ui_node separation_slider`,
  `voe_ui_node reset_button`, and `char contrast_text[16]`, `char separation_text[16]` — the two value
  labels' text, which must outlive the draw that made it (`ui/widgets.h`).
- `voe_editor_preferences_action` gains `VOE_EDITOR_PREFERENCES_ADJUST` and
  `VOE_EDITOR_PREFERENCES_RESET`; the result gains `float contrast`, `float separation` (the sliders' values
  this frame) and `bool sliding` (either slider is held — card 15 writes the file when it goes false).
- The header gains a paragraph: the two sliders belong to the theme in force, they move the whole editor as
  they are dragged, their range is `VOE_UI_THEME_SCALAR_MIN..MAX` so text stays readable, and Reset puts the
  theme's own two back (ADR-0197).

`editor/src/preferences.c`: under the scroll area of theme rows and above Close, a column holding, per
scalar, a row of a label ("Contrast", "Surface separation"), `voe_ui_slider(ui, "contrast", 0, value,
VOE_UI_THEME_SCALAR_MIN, VOE_UI_THEME_SCALAR_MAX, 60.0f)` and a label of the value formatted `%.2f` into the
struct's buffer; then a Reset button. `value` is the chosen entry's `contrast_strength` /
`surface_separation` (card 13). `voe_editor_preferences_clicks_read` reads both with
`voe_ui_slider_action`, fills `contrast`, `separation` and `sliding`, and answers ADJUST when either
`changed`, RESET when the button fired, and otherwise what it answers today; Choose and Close win over
ADJUST when both happened.

`editor/src/interface.h`: raise `VOE_EDITOR_INTERFACE_NODES` and `VOE_EDITOR_INTERFACE_ELEMENTS` by what
Preferences now adds, counted in its paragraph the way the browser's and the bar's are: the column, two
rows, two labels, two sliders (three nodes each), two value labels and Reset with its label.

## Done when
The folder's check passes (`checks.sh` for `editor`), and `voe_editor --capture /tmp/prefs.png --size
1280x720` still draws the editor with no refused frame on stderr (ADR-0177).
