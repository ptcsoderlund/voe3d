# 064 — The splash stays alive while it works

## What
While the splash screen is up, at start or while a scene opens, in the editor or a game, the window stays
alive. It redraws, it can be moved, and the OS never calls it not responding. Its status line shows progress,
for example "Preparing shaders 12/40", and moves along as the work goes on. Closing the window while the splash
is up quits at once. Shaders prepared once are remembered, so later starts get past "Preparing shaders" much
faster (0362).

## Why
The splash freezes until the work is done, which looks like a hang and cannot be closed.

## How to test
1. Start the editor for the first time after the build. While the splash shows "Preparing shaders", drag the
   window around and cover it with another window and uncover it. It keeps drawing, and the status line counts
   up. The OS never says the window is not responding.
2. Close the editor and start it again. "Preparing shaders" is over much sooner than in step 1.
3. Start the editor again and close its window while the splash is up. It quits within about a second, with no
   crash and no error. Start it once more: it starts normally.
4. Open the tank game's scene. While the splash is up for the load, drag the window. It keeps drawing, and the
   status line moves.
5. Ship the tank game and run it twice. The first time, the splash stays alive and shows progress while
   preparing shaders. The second start is faster. Close it during its splash once: it quits at once.
6. Once a scene is open, the editor and the game look and behave exactly as before.
