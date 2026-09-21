# 014 — Undo and redo

## What
Ctrl+Z undoes the last change to the project and Ctrl+Shift+Z (and Ctrl+Y) redoes it. Every change that marks
the project unsaved can be undone: a dragged or typed number, a typed name, a colour picked, a shape's kind,
an entity added, duplicated or deleted, and a component added or removed. One drag, one typed commit or one
visit to the colour picker is one step, not one step per frame. Undoing a delete brings the entity back as it
was, with its name, id and every component. Selecting something is not a change and is not undone. Making a
new change after undoing throws away what could have been redone. The history survives Save; opening another
project or New starts an empty one. The shortcuts do nothing while a field holds the keyboard (there, typing
is undone by Escape) or while the file browser shows. Undoing still leaves the project marked unsaved.

## Why
The move gizmo that follows (015) makes it easy to knock a thing out of place by accident; today the only way
back is typing the old numbers or reopening the last save.

## How to test
1. Drag a cube's X position from 0 to about 3 in one drag. Press Ctrl+Z. It is back at 0 in one step. Press
   Ctrl+Shift+Z. It is at 3 again. Ctrl+Z, then Ctrl+Y does the same as Ctrl+Shift+Z.
2. Type a new name, pick a colour and change a shape's kind, then press Ctrl+Z three times. Each press undoes
   one of them, newest first, and the scene views and Inspector show it at once.
3. Delete an entity, press Ctrl+Z. It is back in `Scene` with its name and every component, where it was.
4. Add a Cylinder, duplicate it, add a component to the copy and remove one. Undo all four, one at a time, then
   redo all four.
5. Undo twice, then make a new change. Ctrl+Shift+Z does nothing.
6. Select different entities between changes. Ctrl+Z never changes the selection alone; it undoes the change.
7. Click into a number box and type, then press Ctrl+Z. The project's history is not touched.
8. Save, then Ctrl+Z. The last change before the save is undone, and the project is marked unsaved.
9. Open another project and press Ctrl+Z. Nothing happens.
