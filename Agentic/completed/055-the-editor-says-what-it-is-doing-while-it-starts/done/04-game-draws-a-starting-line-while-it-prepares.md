# 04 — Game draws a starting line while the device prepares
folder: game
after: 02, 03
decisions: 0168, 0259, 0345

## Change
- `game/include/game/starting.h` (new):
  - `[[nodiscard]] bool voe_game_starting_frame(voe_app *app, voe_ui_context *ui, voe_base_arena *frame_arena, const char *line)`
    — one frame: polls the window (skipping `voe_app_frame_open`, so an unfocused
    window does not wait a quarter second per frame, ADR-0215), takes its size
    (the app's settings size when headless), lays out a ui frame over
    `voe_game_interface_surface(size)` with a GROUND panel filling it and `line`
    as one label centred, opens the draw, one NULL-camera window pass of the
    context's element records, closes the draw. A size with no area draws
    nothing. False when the window is closing or the draw failed.
  - `[[nodiscard]] bool voe_game_starting_prepare(voe_app *app, voe_ui_context *ui, voe_base_arena *frame_arena, const char *line)`
    — a starting frame, then `voe_render_device_prepare` once, repeated until
    PREPARED; false on a closing window or FAILED.
  The header says why (never a blank window while pipelines build, 0345), that
  the colours are the context's theme in force so a light theme reads, and that
  the caller's arena is rewound by the caller.
- `game/src/starting.c` (new): both. For how element records reach a pass, read
  `game/src/frame.c`'s interface draw and do the same.
- `game/include/game/interface.h`, `game/src/interface.c`:
  `voe_game_interface_context` takes and returns non-const, so the run can hand
  its context to the starting frame; its callers are all in `game` and compile
  as they are.
- `game/src/run.c` and `game/include/game/run.h`: after the interface is made,
  `voe_game_starting_prepare` with the line "Starting — preparing shaders…"; then
  the rest of the order as now. A `voe_app_start_log` times "window and device",
  "interface", "preparing shaders", "world and scene", "models and sound" and
  "first frame" (after the first `voe_game_frame`), written to stderr only
  (path NULL) as program "game". A false starting prepare ends the run as a
  closed window does. `run.h`'s THE ORDER paragraph gains both.
- If Oxanium lacks the em dash or the ellipsis (the font from
  `voe_game_interface_new` reports its glyphs; check with text's lookup), use
  "-" and "..." instead, here and in card 05's line.
- `game/include/game/game.md` and `game/src/src.md`: entries for starting.h and
  starting.c.
- `game/tests/starting.c` (new, headless app as `game/tests/interface.c` makes
  one): `starting_frame_draws_line` — after one starting frame, the window
  target read back has the theme's ground colour at a corner and a pixel unlike
  it in the middle row; `starting_prepare_prepares` — after the prepare loop the
  next `voe_render_device_prepare` answers PREPARED. List it in
  `game/tests/tests.md`.

## Done when
The two cases in `game/tests/starting.c` pass.
