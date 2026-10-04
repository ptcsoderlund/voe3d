# 055 — The editor says what it is doing while it starts

## What
When I start the editor, its window is never white. From the moment it opens it
shows the theme's background, with one line of text in the middle saying what
it is doing, such as "Starting — preparing shaders…". When it is ready, the
normal editor replaces that screen. A start that takes a moment no longer looks
frozen.

The editor also writes to its log how long each step of the start took, from
launch to the first normal frame, so I can see where a slow start went. It
should be clear from the log whether the time went to preparing shaders or to
something before the editor's own code even ran (the system scanning a new
program, for example).

The game window works the same way when Play opens it.

## Why
After a code change the first start shows a white window for a long time, which
looks like a hang. Saying what is happening fixes that, and the timings tell us
whether there is a real cost worth removing.

## How to test
1. Change any line of engine code, rebuild, and start the editor on Windows.
   The window opens dark (the theme's background, not white) and shows the
   "preparing shaders" line until the editor appears.
2. Close it and start it again. It starts faster, and the starting line shows
   only briefly or not at all. The window is never white.
3. Open the log from both starts. Each lists the start's steps with how long
   each took, and adds up to roughly how long the start felt. The first start
   shows where the extra time went.
4. Switch to the Near white theme, close, and start again. The starting screen
   is in the light theme's background and its text is readable.
5. Press Play. The game window opens dark with its own starting line, never
   white, then the game appears.
6. Do steps 1–3 on Linux. Same behaviour, nothing broken.
