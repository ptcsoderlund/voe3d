# 02 — A game hands its project a ui frame
folder: game
decisions: 0168, 0259, 0185, 0194

## Change
Read `game/include/game/project.h`, `ui/include/ui/layout.h`, `ui/include/ui/widgets.h`,
`ui/include/ui/theme.h`, `text/include/text/font.h`, `platform/include/platform/input.h`,
`render/include/render/device.h` (element section only).

- `game/CMakeLists.txt` — DEPENDS gains `ui text`.
- `game/include/game/project.h` — new type `voe_game_project_frame` { `voe_ecs_world *world`,
  `voe_platform_window *window` (NULL headless), `voe_ui_context *ui`, `voe_math_float2 size` }
  and the fourth declaration `bool voe_game_project_interface(const voe_game_project_frame
  *frame)`. Header points: four entry points; the interface runs once a frame after the steps;
  the frame is begun with the pointer set, the project ends it and reads its buttons; false ends
  the run; a project with no interface ends the frame and returns true.
- `game/include/game/interface.h`, `game/src/interface.c` (new) — the game's side of it:
  - `VOE_GAME_SURFACE_HIGH` 135.0f; `VOE_GAME_INTERFACE_NODES` and
    `VOE_GAME_INTERFACE_ELEMENTS` (say why: a glyph is an element; room for a few panels of
    short labels, e.g. 256 and 4096).
  - `voe_game_interface` (font, derived theme, context), made by
    `voe_game_interface *voe_game_interface_new(voe_render_device *device, voe_base_arena
    *arena)`: Oxanium font, `voe_ui_theme_default_inputs` derived, context with the two
    capacities, font and theme set. NULL, with a stderr line, when the font is refused.
    `void voe_game_interface_destroy(voe_game_interface *)` releases the font.
  - `voe_math_float2 voe_game_interface_surface(voe_platform_size size)` — millimetres, 135 tall,
    width by aspect; asserts on a size with no area.
  - `bool voe_game_interface_run(voe_game_interface *, voe_base_arena *frame_arena,
    voe_ecs_world *, voe_platform_window *, voe_platform_size, bool (*interface)(const
    voe_game_project_frame *))` — begins the ui frame, sets the pointer (window pixels over
    pixels per mm, left button down; NULL window is a pointer not over), calls `interface`,
    returns its answer.
  - `const voe_ui_context *voe_game_interface_context(const voe_game_interface *)` for the draw.
- `game/tests/interface.c` (new) — on a headless device (as `tests/frame.c` opens one; skip
  without Vulkan): the surface of 1280×720 is 240×135; a run with a stand-in interface that lays
  out a panel with a label and a button, ends the frame and returns false returns false and
  leaves element records; one returning true returns true.
- `game/include/game/game.md`, `game/src/src.md`, `game/tests/tests.md` — the entries for
  `interface`; `project.h`'s entry says four entry points.
- `run.c`, `frame.c` unchanged: card 05 wires them.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder game` prints `FINDINGS: 0`.
2. `ctest --test-dir build/debug -R "^game/interface"` passes.
