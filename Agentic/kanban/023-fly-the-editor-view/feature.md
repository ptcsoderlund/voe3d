# 023 — Fly the editor view

## What
Holding the right mouse button in a scene view lets me fly its view around, the way Unreal and Unity do.

- Only the scene view under the pointer when I press the right button flies. The other view doesn't move.
- While the button is held, moving the mouse turns the view: left and right to look around, up and down to look
  up and down. The pointer is hidden and doesn't move. When I let go, it shows again where it was.
- While the button is held, W moves forward in the direction I'm looking, S moves back, A moves left and D moves
  right. E goes straight up in the world and Q straight down, whichever way I'm looking. Keys held together
  combine, so W and D together move diagonally.
- Movement is a steady speed that doesn't depend on the frame rate. Holding Shift moves about three times as fast.
- While the button is held, those keys only move the view. They don't type into a field or trigger a shortcut.
- This moves the editor's own view only. The scene's Camera entity, its transform and the undo history are not
  touched, and the scene is not marked as changed.
- Where a view was left is not remembered after the editor is closed.

## Why
Getting around a level by keyboard and mouse is the most basic thing an editor view has to do. Nothing else in
the editor moves the view this way yet.

## How to test
1. Open the editor with a scene that has a few shapes in it.
2. Put the pointer over the top scene view, then hold the right mouse button and move the mouse. The view turns to
   follow the mouse, the pointer disappears, and the bottom view stays still.
3. Keep the button held and press W, then S, A and D, one at a time. The view moves forward, back, left and right
   relative to where it's looking. Look down at the floor and press W: the view moves towards the floor.
4. Still holding the button, press E and then Q. The view goes straight up and then straight down, even while
   looking at an angle.
5. Hold W and D together. The view moves diagonally.
6. Hold Shift with W. The view moves about three times as fast.
7. Let go of the right mouse button. The pointer comes back where it was, and pressing W, A, S, D, Q or E no
   longer moves the view.
8. Do the same in the bottom view. Only the bottom view moves.
9. Click the scene's Camera. Its transform in the Inspector is unchanged. Undo does nothing to the view, and the
   scene isn't marked as changed.
10. Click a number field in the Inspector so it's being edited, then put the pointer over a scene view, hold the
    right button and press W. The view moves, and no "w" is typed into the field.
