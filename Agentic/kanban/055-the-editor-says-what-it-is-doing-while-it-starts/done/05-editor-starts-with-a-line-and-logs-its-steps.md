# 05 — The editor shows its starting line and logs the start's steps
folder: editor
after: 02, 03, 04
decisions: 0168, 0178, 0339, 0345

## Change
- `editor/src/main.c`: a `voe_app_start_log` begun first thing in `main`. Steps,
  each marked where that part ends in the existing order: "project" (startup.h's
  choice), "window and device" (`voe_app_new` or `_headless`), "font, themes and
  interface", then `voe_game_starting_prepare` (game/starting.h) on the
  interface's context with the line "Starting — preparing shaders…" (or the ASCII
  fallback card 04 chose), marked "preparing shaders", then "scene and shapes"
  (everything up to the loop), and "first frame" after the first
  `voe_app_draw_close` that drew. The chosen theme (themes.h) must already be set
  on the context before the first starting frame, so Near white starts light. A
  false starting prepare ends the program as a closed window does, cleaning up as
  that path does. Then `voe_app_start_log_write` once with program "editor" and
  the path `start.log` joined onto `voe_platform_folder_settings` (stderr only,
  path NULL, when `--capture` or when there is no settings folder); a failed write
  is its stderr line and nothing more. The header gains a paragraph: the start
  shows a line while render prepares (0345) and its steps are logged, where.
- `editor/src/src.md`: the `main.c` entry names the starting line and the log.

## Done when
`voe_editor --capture <scratch>/shot.png` (build tree's editor program, as
`editor/src/capture.h` describes) exits 0 and its stderr has one line per step
above, "before main" first, and a total line. 

Human, on Linux (0339 drops the feature's Windows steps):
1. Change a line of engine code, rebuild, start the editor: the window opens in
   the theme's background with the "preparing shaders" line until the editor
   appears; never white.
2. Start again: faster; the line shows briefly or not at all.
3. `start.log` in the voe3d settings folder holds both starts, each step timed,
   the total near the felt time; the first shows where the time went.
4. Choose Near white, close, start: the starting screen is light, its text
   readable.
5. Press Play: the game window opens dark with its own starting line, then the
   game.
