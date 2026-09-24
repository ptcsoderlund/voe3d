# 024 — Press Play

## What
The editor's top bar has a Play button. Pressing it starts the game as a program of its own, in its
own 1280×720 window, showing the scene through the scene's camera: what the camera's corner picture
shows, filling the game's window. The game shows the scene exactly as it is in the editor at that
moment, unsaved changes included. Play does not save and does not change whether the project is
marked unsaved.

While the game runs, the button reads Stop. Pressing Stop closes the game. Closing the game's window
also stops it, and the button reads Play again. The editor stays open and works normally the whole
time: you can select, move, fly the views and undo while the game is open. Changes made while the
game runs don't reach it. To see them, stop and press Play again.

The game has no code of the developer's yet and no quit key: Escape does nothing special. If the
build fails, nothing starts and the button stays Play. Showing the build's errors comes later.

## Why
Milestone 2 of 0186: the first time the level made in the editor runs as a game. Decisions 0187,
0188 and 0234.

## How to test
1. Open the editor and open a project with a few shapes in different colours. Move one shape so the
   project is marked unsaved. Don't save.
2. Select the camera and look at its corner picture. Press Play. Within about five seconds a new
   1280×720 window opens and shows what the corner picture shows, moved shape included. The top
   bar's button now reads Stop.
3. Check the project is still marked unsaved and the file on disk hasn't changed.
4. Press Escape in the game window. Nothing happens.
5. In the editor, select a shape, move it, fly a view and undo. Everything works while the game stays
   open, and the game still shows the scene as it was when you pressed Play.
6. Press Stop. The game window closes and the button reads Play.
7. Press Play again. The game opens and now shows the move from step 5.
8. Close the game's window with its close button (or Alt+F4). The button reads Play again and the
   editor is unchanged.
9. Move or turn the camera, press Play, and check the game looks where the camera now looks.
