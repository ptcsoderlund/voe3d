# 019 — The scene's one camera

## What
Every scene has exactly one camera, on its own entity, listed in the Scene list. A new scene starts with it,
and opening a scene saved before this adds one, a little back from the centre and looking at it, and marks the
scene unsaved. In the view the camera is drawn as a small camera with lines showing what it sees. You select it
by clicking it or in the Scene list, move it with the move gizmo, and aim it by typing its rotation in the
Inspector, where its field of view can also be set. While it is selected, a small picture in a corner of the
view shows what the camera sees. It cannot be deleted or duplicated, its camera component cannot be removed,
and Add component never offers a camera. This is the camera Play will show the game through; there is no Play
yet.

## Why
Decision 0218. Play needs something to look through, and the sponsor wants to place it themselves.

## How to test
1. Make a new scene. The Scene list has a camera entity. The view shows it as a small camera with its lines.
2. Open a scene saved before this feature. It has a camera too and is marked unsaved.
3. Click the camera in the view. It is selected and a small picture in a corner shows what it sees.
4. Drag it with the move gizmo. The picture follows.
5. Type a rotation in the Inspector. The camera turns in the view and the picture turns with it. Change the
   field of view; the lines and the picture widen or narrow.
6. Try to delete it, duplicate it, and remove its camera component. None is possible.
7. Select any other entity and open Add component. No camera is offered.
8. Undo and redo the move and the rotation.
9. Save, close, reopen. The camera is where you left it, aimed the same way.
