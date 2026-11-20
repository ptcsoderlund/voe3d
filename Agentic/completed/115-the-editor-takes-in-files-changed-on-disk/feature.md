# 115 — The editor takes in files changed on disk

## What
While a project is open, I or an agent can change any of its `[Section]` text files outside the editor —
`project.voe3d`, the scene, prefabs, materials, landscapes, parts — and the editor shows the change within a
second or two, without reopening. If I have no unsaved changes to that file, it just loads, and Undo takes it
back. If I do, the editor asks about that file alone: load from disk, or keep mine. A changed file that is
wrong — a key that section does not take, a value of the wrong kind or out of range — is not loaded: the
editor keeps what it shows and tells me the file, the line, the key and why. The editor's own saves never
ask and never reload. (0505, 0506)

## Why
So an agent in the project folder and I in the editor can work on one project at once, and neither of us
erases the other's work.

## How to test
1. Open a project with a sun. In a text editor, change the sun's colour and strength in the scene file and
   save. Within a second or two the view and the Inspector show the new light. Ctrl+Z puts the old light
   back.
2. Change a material's colour in its `.material` file. Every surface wearing it changes in the view.
3. Change a prefab's file, a landscape's file and the game window size in `project.voe3d`, one at a time.
   Each shows in the editor without reopening.
4. Move a light in the editor and do not save. Change the scene file on disk. A question names the scene
   file: load from disk, or keep mine. Choose keep mine: my moved light stays, and Save writes it.
   Repeat and choose load from disk: the file's version shows.
5. With unsaved changes to both the scene and a landscape, change both files on disk. I am asked about each
   file on its own.
6. Write `texture=foo.png` into a light's section in the scene file and save. The editor keeps showing the
   scene as it was and tells me the file, the line, `texture` and why. Fix the line and save: it loads.
7. Edit in the editor and Save, many times. No question and no reload ever comes from my own saves.
