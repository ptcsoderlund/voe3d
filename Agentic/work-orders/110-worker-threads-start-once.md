# 110 — Worker threads start once

## What
The editor, Play and a shipped build start their worker threads once, when they open, and keep them
until they close. Loading models and materials, watching files for changes and compressing textures
all run on those same threads instead of starting new ones each time. Under CLion's debugger on
Windows the editor no longer stutters from thread starts, and nothing gets slower (0497).

## Why
Every load and every once-a-second file check starts and stops threads today. That costs a little
every time, and under a debugger on Windows each start pauses the whole editor, which makes debugging
a big project painful.

## How to test
1. Open the hill in the editor under CLion's debugger (Shift+F9), maximized. Once the splash is gone,
   the debugger's console shows no new "thread created" or "thread exited" lines while I fly around
   the forest for a minute.
2. Still under the debugger: drop a new model into Assets, change a model file on disk, and import a
   texture so it gets compressed. Each one loads and shows up as before, and still no threads come
   and go.
3. Flying through the forest under the debugger does not freeze. (The window may still be slower at
   4K than without a debugger.)
4. Without the debugger, open the hill from cold: it takes about as long as before, or less.
5. Press Play, then stop: no threads come and go. Close the editor: it closes at once and cleanly.
6. Ship the hill and run the build: it opens and runs as before.
