# 09 — The selected entity is outlined in every view
folder: editor
decisions: 0168, 0177, 0194, 0196, 0203
read: feature.md

## Change
Every view's pass is told what to outline and in what colour. All of it is `editor/src/main.c`.

- `EDITOR_CAPACITIES` gains the three transient numbers, in place of the sentence saying they stay nought:
  `.transient_vertices = VOE_3D_OUTLINE_VERTICES * VOE_EDITOR_VIEWS`,
  `.transient_indices = VOE_3D_OUTLINE_INDICES * VOE_EDITOR_VIEWS` and
  `.transient_geometries = VOE_EDITOR_VIEWS` — one outline per view's pass, every view in the room there is for
  views and not the two in use, the way `passes` and `targets` are already sized. The comment above says the
  outline is what they are for (0203).
- A `#define VOE_EDITOR_OUTLINE_MILLIMETRES 0.4f` beside `WHEEL_MILLIMETRES`, with the same kind of comment:
  how thick the selection's outline is drawn is this program's to choose, in the surface's millimetres like
  everything else it sizes, and `pixels_per_millimetre` is what turns it into the pixels `3d` wants — so it
  stays the same thickness on a screen of any density (ADR-0180).
- A static function beside `world_light`, `outline_colour`, taking a `const voe_ui_theme *` and answering the
  lighter of `palette->inverse` and `palette->inverse_ink` as a `voe_math_float3` — lighter by relative
  luminance, `0.2126 r + 0.7152 g + 0.0722 b`, on the linear numbers the palette already holds (ui/theme.h).
  Its comment is 0203's paragraph in two sentences: a scene view's background is the engine's near-black clear
  colour whatever the theme is, so the dark half of a light theme's inverse pair would be an outline nobody can
  see; both roles carry the theme's one hue and neither is a colour of its own, so taking the lighter keeps
  0194's rule and keeps the outline visible in every theme.
- In the per-view pass, the `voe_3d_frame` literal handed to `voe_3d_draw_system_run` gains an `.outlined`:
  `.entity` is `voe_editor_scene_selected(&scene)`, `.geometries` the store card 08 made, `.material`
  `shapes.outline` (card 04), `.colour` `outline_colour(&voe_editor_themes_chosen(&themes)->palette)`,
  `.pixels` `VOE_EDITOR_OUTLINE_MILLIMETRES * pixels_per_millimetre` and `.size`
  `{ (int)view->width, (int)view->height }`. Nothing else about the pass changes: the same camera, the same
  light, the same call.
- The file header's paragraph "A FRAME IS A PASS PER VIEW" gains a sentence: each view's pass also outlines
  whatever is selected, in the theme's own lightness, drawn after everything else in the pass so it shows
  through what stands in front of it (0203, 3d/draw_system.h).

`editor/editor.md`'s paragraph about the views says the selected entity is drawn with a thin outline round its
silhouette in both views, in the theme's own lightness and never a colour of its own, and that it shows even
when something is in front of it.

## Done when
`checks.sh editor` exits 0, and `voe_editor --capture <path>.png --size 1280x720` still writes a picture. At a
running `voe_editor`: selecting a shape — by its row in `Scene` or by clicking it — draws a thin outline round
it in both views; moving the camera so another shape stands in front of it leaves the outline showing; changing
its kind, colour or position moves the outline with it; and nothing selected draws no outline anywhere.
