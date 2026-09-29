# 0291 — The game window is a `[window]` section of the project file
date: 2026-09-29
by: planner

## Decision
For 039, filling in what the feature and 0268 milestone 6 leave to the planner:

1. **`project.voe3d` gains an optional `[window]` section** after `[project]`: `width` and
   `height`, whole numbers 160 to 16384, and `fullscreen`, `true` or `false`. A missing section
   or key is its default, 1280, 720 and `false`, so every existing project still opens. Any other
   value, key or section is refused and reported with its line. The writer always writes the
   section.
2. **Fullscreen takes the screen and ignores the size.** `platform`'s `voe_platform_window_new`
   takes a `fullscreen` flag: on Wayland `xdg_toplevel_set_fullscreen` on no named output before
   the first commit, the compositor's configure giving the size; on Windows a `WS_POPUP` over the
   primary monitor's rectangle. `app`'s settings carry the flag through. There is no switch
   while running; Alt+F4 or the editor's Stop closes it (0234).
3. **The game is handed the window by its generated `main.c`**: `voe_game_run(title, window)`
   with a `voe_game_window` of width, height and fullscreen, written by the editor's game tree
   from the project's settings. The game never reads project text (0236). Play and Ship share it.
4. **The editor's Project panel** is a top-bar button, Project, left of Preferences, opening an
   anchored panel in Preferences' place: width and height number boxes (rounded, clamped to
   point 1's range), a Windowed / Fullscreen choice, and Close. A change is written to
   `project.voe3d` at once; it is not a scene edit, so it marks nothing unsaved and is not undone.
   An untitled project keeps the settings in memory and writes them with its first save.
5. **The tank game keeps the level's width by its lens.** The width the scene's camera frames at
   16:9 (the old 1280×720 and the editor's 480×270 preview) is the level's width. A project
   system, before the move, keeps the camera's authored `fov_y` in a runtime-only row on the
   camera and each step submits the whole lens: the authored `fov_y` at 16:9 or wider (a wider
   window sees more water at the sides), otherwise widened so the horizontal field stays the one
   at 16:9, capped at 3.0 rad. Headless, nothing changes. The sponsor's scene is not edited.

## Reasoning
A section of its own keeps the project's one file and its reader's shape (0164, 0236), and
defaults keep old projects working. Opening fullscreen rather than switching after the first
configure avoids a windowed frame on Wayland and needs no toggle nothing asks for (rule 10).
Numbers in `main.c` are the cook's way into the game already. Writing the file at once matches
the editor settings (0220): a setting is not part of the scene's undo line. A lens that widens
only when narrower than the framing is the least that keeps the width and shows more when wide;
a runtime row follows the coin game's camera state (0261) and leaves the sponsor's scene alone.
Rejected: keys in `[project]` (mixes the file's identity with the game's settings); a global
editor setting (the size is the game's, not the person's); moving the camera up instead of
widening the lens (changes the angle the sponsor framed).

## Replaces
nothing. Ends 0234's fixed 1280×720 window.
