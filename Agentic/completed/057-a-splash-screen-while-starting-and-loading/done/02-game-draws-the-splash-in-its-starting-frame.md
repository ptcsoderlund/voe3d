# 02 — game draws the splash in its starting frame
folder: game
after: 01
decisions: 0168, 0345, 0356

## Change
The starting frame draws a splash picture when handed one, and 055's plain screen when not.

- `game/include/game/starting.h`: `voe_game_starting_frame` and `voe_game_starting_prepare` each
  gain `const voe_app_picture *splash` (app/picture.h) after `frame_arena`. The header's
  colour paragraph becomes the two layouts: NULL is today's GROUND panel with the line centred;
  a splash is its top-left texel stretched over the whole surface (the edge colour), the
  picture anchored centred at the largest size that fits without changing its aspect, and a
  GROUND panel holding the line in normal text, anchored at the bottom middle with its lower
  edge 8 mm above the surface's (0356). It redraws from the window's size every frame, so a
  resize keeps it whole and centred. The picture is the caller's and outlives the frame.
  The usage example passes NULL.
- `game/src/starting.c`: the splash layout, using `voe_ui_image` (ui/widgets.h) for the two
  picture nodes, with a sheet inside the top-left texel for the edge colour, and anchored
  containers (ui/layout.h `voe_ui_anchor`) for the centred picture and the bottom box. Sizes
  are in the surface's millimetres (`voe_game_interface_surface`). The NULL path is unchanged.
- `game/src/run.c`: pass NULL for now (card 03 hands it the game's splash).
- `game/tests/starting.c`: existing cases pass NULL; new `splash_fits_a_wide_window` and
  `splash_fits_a_tall_window` — a 4×2 texture made with `voe_render_texture_create` whose
  top-left texel is one colour and the rest another, wrapped in a `voe_app_picture`; on a
  surface much wider (then much taller) than 2:1, the read-back pixel in the margin beside
  (then above) the picture has the edge colour, the window's centre has the picture's colour,
  and the bottom middle, about 10 mm up, has the theme's ground colour.
- `game/include/game/game.md`, `game/src/src.md`, `game/tests/tests.md`: the `starting` entries
  say splash or plain.

The editor still calls the old signature; card 05 changes it. Do not touch `editor/`.

## Done when
`game/tests/starting.c`'s cases pass, the two new ones among them.
