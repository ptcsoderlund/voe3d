# 003 Render to a texture, and save it — acceptance

## Start it

1. Build everything and run the checks once:

       cmake --preset debug
       cmake -P check.cmake

2. Build the two programs you will look at:

       cmake --build --preset debug --target voe_dev
       cmake --build --preset debug --target voe_editor

## Try this

1. **A second camera's view shows on a surface in the world** — run the dev program:

       ./build/debug/dev/voe_dev

   Expected: among the cubes stands a flat screen carrying a small picture of the same world,
   seen from somewhere your own camera is not. Orbit around with the mouse, or Tab to fly: the
   picture on the screen keeps showing its own fixed viewpoint while your view of the room
   changes, and the turning cube inside the picture keeps turning. The screen does not appear
   inside its own picture, and it does not darken as the sun crosses it.

2. **A picture is saved as a PNG file** — from the editor, with a desktop or without:

       ./build/debug/editor/voe_editor --capture ~/editor.png --size 1280x720

   Expected: the command finishes and says nothing. Open `~/editor.png` in any ordinary image
   viewer. It shows the editor as you would see it on screen — the Scene list on the left, two
   stacked scene views in the middle, the Inspector on the right — at exactly 1280 by 720, with
   the colours right.

3. **The two are independent** — the dev program draws into a texture and saves nothing; the
   editor's capture saves a picture that was never in the scene. Expected: each works without
   the other, as you just saw.

4. **The editor draws with no desktop at all** — over a plain terminal with no graphical session
   (for example an SSH session, or with `WAYLAND_DISPLAY` unset):

       WAYLAND_DISPLAY= ./build/debug/editor/voe_editor --capture ~/headless.png --size 1280x720

   Expected: no window ever appears, the command exits, and `~/headless.png` is a picture of the
   editor. Name a directory that does not exist and it refuses with a plain message instead,
   writing nothing.

5. **A program that draws nothing is unaffected** — nothing you run needs graphics it did not
   already need. Expected: `cmake -P check.cmake` exits zero, as in Start it.

6. **A picture saved with no display looks the same as one drawn on screen** — compare the
   `~/headless.png` from step 4 against what the editor shows when you run it normally with a
   desktop:

       ./build/debug/editor/voe_editor

   Expected: the same frame, panel for panel.

7. **The checks pass** — `cmake -P check.cmake` exits zero, as in Start it.

## Results
