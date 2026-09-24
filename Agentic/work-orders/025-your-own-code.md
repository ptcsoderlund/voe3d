# 025 — Your own code

## What
A project has C code of its own, written in any text editor. The code defines components and
systems the way the engine does (0239). The editor knows the project's components: you add them to
entities, edit their fields in the Inspector, undo, save, and they reach the game. A Refresh in the
editor picks up changed code without restarting it (0241). When you press Play, the project's
systems run every frame in the game, read the keyboard and mouse through the engine's API, and move
things. When the code doesn't compile, the editor shows the compiler's errors (0240). This is
milestone 3 of 0186: the capsule moves with the keys and the camera follows it. There is no
collision and no character controller yet; those are milestone 4.

The feature comes with an example project: a capsule on a floor, a camera, and C files with a
`keyboard_input` component and `keyboard_system`, a `player` component with a speed and a
`player_system` that moves the transform of every entity that has both `player` and
`keyboard_input`, and a `follow_camera` component (a target entity and a distance) with a system
that keeps the camera behind its target.

## Why
A game is its logic. Until the project's own code runs, the editor only builds scenes.

## How to test
1. Open the editor and open the example project. The capsule has none of the project's components
   yet.
2. Select the capsule and open Add component. The project's Keyboard Input and Player are listed
   beside the engine's own. Add both. The Inspector shows Player's speed, which you can drag and
   type. Undo removes Player and redo puts it back.
3. Select the camera, add Follow Camera, pick the capsule as its target and set a distance.
4. Press Play. In the game, WASD moves the capsule over the floor at the speed you set, and the
   camera stays behind it at that distance. Press Stop.
5. Without saving, change the speed and press Play again. The capsule now moves at the new speed.
6. Remove Keyboard Input from the capsule and press Play. The capsule doesn't move. Undo, and Play
   moves it again.
7. Save, close the editor, open it and the project again. Every component and value is as you left
   it.
8. Make a change without saving. Then, in a text editor, add a second field to Player (for example a
   run speed used while Shift is held) and use it in `player_system`. Press Refresh in the editor.
   Within a few seconds, without the editor restarting, the Inspector shows the new field, and your
   unsaved change is still there. Play uses the new code.
9. Put a mistake in one of the C files and press Refresh. The editor shows the compiler's errors and
   the components stay as they were. Press Play: the same errors show and no game starts. Fix the
   mistake, and Refresh and Play work again.
10. Rename Follow Camera in the code and press Refresh. The camera's Follow Camera is gone from the
    Inspector, and nothing else changes. Save, rename it back, and Refresh. It is back with its target
    and distance.
11. Open a project with no code of its own. Play works as it did before.
