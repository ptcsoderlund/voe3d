# 0259 — A game draws a project's interface through a fourth entry point
date: 2026-09-26
by: planner

## Decision
For 029, the seam through which a project draws menus and a HUD over its game (0186 milestone 5:
"a HUD drawn with `ui`"):

1. **`game` gains the edges `text` and `ui`** (its row: base math ecs scene physics platform
   render text ui 3d app). Not `theme`: a game parses no project text (0236), so it draws with
   `voe_ui_theme_default_inputs` derived, in Oxanium (0185), which is 0194's "a cooked game's GUI
   draws with a theme".
2. **A fourth entry point in `game/project.h`:**
   `bool voe_game_project_interface(const voe_game_project_frame *frame)`. The frame carries the
   world, the window, a `ui` context whose frame the game has begun with the pointer set, and the
   surface's size in millimetres. The project lays out, calls `voe_ui_frame_end` itself (a
   widget answers only after it) and reads its buttons. It returns false to end the run: the
   game's own Quit. A project with no interface, and `cmake/game.cmake`'s no-code stub, end the
   frame and return true.
3. **Once a frame, after the steps.** Clicks and keys are per frame; rules stay in the fixed
   step's systems. Then the frame draws the world, clears depth and draws the records over it
   with `voe_render_element_transform` of the surface size.
4. **The surface is `VOE_GAME_SURFACE_HIGH` = 135 mm tall**, as wide as the window's aspect,
   the pointer's pixels divided by pixels per millimetre (the editor's ADR-0104 rule). No
   keyboard is handed to `ui`: a game has no text fields yet; Enter and Escape are the project's
   to read from `platform`.
5. **The entry point is handed in as a function pointer** to what calls it; `run.c` stays the
   one file naming a project's entry points (0245, 0257), and `cmake/exports.cmake` skips a
   member that names the new one.

## Reasoning
A per-frame entry point is the smallest seam that lets a project draw and hear clicks; the
steps run zero to four times a frame and would lose or double a click. Handing the project a
begun frame keeps pointer arithmetic in the engine and every widget the project's. Rejected: the
engine drawing fixed menus (every game would want other screens); a `theme` edge and a theme file
(a game reads no text); separate scenes for menus (the sponsor asked for panels over the level).

## Replaces
nothing. Amends 0234's "no quit key": the game still has none, but a project may end the run.
