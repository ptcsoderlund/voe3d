# 09 — The LODs panel shows Swap distance in metres
folder: editor
after: 01, 02, 04
decisions: 0168, 0500, 0501

## Change
"LODs of <file>" (`editor/src/levels_ask.h`, `.c`) keeps the × box as the input under its own
"Swap" header and adds a "Swap distance" column in metres (0500 point 3). Card 04 renamed game's
call to `voe_game_models_levels_summarise` (`game/include/game/models.h`; read its comment), so
the editor builds again with this card. The bound and metre functions are card 02's
(`3d/include/3d/model_levels.h`), the lens card 01's `VOE_SCENE_CAMERA_FOV_Y`
(`scene/include/scene/camera_component.h`).

- `editor/src/levels_ask.h`: the struct's `triangles`, `known`, `own`, `fresh` become one
  `voe_3d_model_levels_summary summary`; add `row_distances[VOE_ASSETS_LEVELS]` sized for the
  longest metres text and "—". The header comment: the row is name, share, swap, swap distance,
  triangles; Swap distance is metres at which that LOD swaps in for the model at scale 1 through a
  new camera's lens, after the cap and the 1.25 spread, following the typed swaps as they change;
  "—" with no fresh levels file or past the file's LODs; the headers are "Triangles kept", "Swap",
  "Swap distance", "Triangles".
- `editor/src/levels_ask.c`:
  - `voe_editor_levels_ask_open` (line ~119) fills `summary` by the game call.
  - `draw_header_row` (~line 154): "Swap" over the × box column, then "Swap distance", then
    "Triangles"; each column one fixed width as now.
  - The LOD row: after the × box, a label with the distance — the bounds from
    `voe_3d_model_levels_bounds(summary.radius, summary.errors, the shown swaps, summary.count,
    true, ...)` worked once per draw, each LOD's
    `voe_3d_model_level_metres(summary.radius, bound, VOE_SCENE_CAMERA_FOV_Y)` written as metres
    with one decimal; "—" when `!summary.fresh` or the LOD is at or past `summary.count`.
  - Every other use of the four old fields reads `summary`'s.
  - The swap line under the rows says Swap scales the distance the LOD's error allows and Swap
    distance is where it lands, each at least 1.25× the one before.
- `editor/src/src.md`: the `levels_ask` entry names the Swap distance column.

## Meaning
- Holds: the × box stays as the input under its own "Swap" header; "Swap distance" shows metres after
  spreading, for the model at scale 1 through a scene camera's default lens; a LOD with no fresh file
  shows "—" (0500 pt 3).
- Believed (observed): the "Swap distance" header sits over the × box today and no distance is shown.

## Done when
`grep -q voe_3d_model_level_metres editor/src/levels_ask.c` and
`! grep -rq voe_game_models_levels_triangles editor` exit 0, and the editor builds.
