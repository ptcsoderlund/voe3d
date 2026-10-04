# 057 — A splash screen while starting and loading

## What
When the editor or a game is busy starting or loading a scene, it shows the splash screen instead of a plain
background. The splash is the VOE3D image from `engine_assets/Engine images/splashscreen.png`, centred and
scaled to fit the window without stretching. Any room around it is filled with the image's own dark edge
colour. Across the bottom middle is a box in the theme's background colour, holding the status line, such as
"Starting - preparing shaders...", in the theme's text colour. When the work is done, the editor or the game
replaces the splash. There is no minimum time, so a quick load only flashes it.

The editor always shows the engine's splash. A game shows its project's own `Assets/splashscreen.png` if
there is one, and the engine's otherwise. That PNG goes, unchanged, into the shipped game's folder beside the
program. If no splash can be read, the plain starting screen from 055 shows instead.

The splash shows at:
- editor start, and when the editor makes a new scene or opens one;
- game start, from Play and in a shipped game;
- a game's change from one scene to another, once games can do that.

## Why
One recognisable screen wherever the engine is busy, easy for a game to replace with its own (0346).

## How to test
1. Change any line of engine code, rebuild, and start the editor. From the moment the window opens it shows
   the splash, fitted and centred, never white and never stretched. The "preparing shaders" line is in a box
   at the bottom middle, in the theme's colours. Then the editor appears.
2. Resize the window while the splash shows, if it stays long enough to try: the image stays whole and
   centred. Try a very wide and a very tall window.
3. Switch to the Near white theme, close, and start again. The image is the same, but the bottom box is light
   with dark readable text.
4. Open another scene, then make a new one. Each time the splash shows while it loads, even if only for a
   flash.
5. Open `examples/tank_game` and press Play. The game window opens on the engine's splash, with its own line
   in the bottom box, then the game appears.
6. Put any other PNG in `examples/tank_game/Assets/` named `splashscreen.png` and press Play again. The game
   shows that image. The editor still shows the engine's. Remove the file afterwards.
7. With your own `splashscreen.png` in place, Ship the game. The shipped folder has the PNG beside the
   program, and starting it shows your image. Without the file, a ship shows the engine's.
8. Rename the engine's copy of the splash so it cannot be found, rebuild, and start the editor: it shows 055's
   plain themed screen with its line, nothing breaks. Restore it.
