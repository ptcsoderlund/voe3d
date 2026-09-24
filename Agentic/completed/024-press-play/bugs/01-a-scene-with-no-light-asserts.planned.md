# 01 — A scene with no light asserts

## Seen
"We should not force (assert) one or more light sources on start. Devs might wanna do a 2d game where everything
is lit without lights. Or they might want to do their own shading." Drawing a frame of a scene with no light hits
the assert in `voe_3d_draw_system_frame`; `game/include/game/frame.h` records it as a limit.

## Expected
A scene with no light opens, shows in the editor's views and plays without an assert, and every surface draws in
its own material colour, unshaded (0238). Adding a light brings shading back. More than one light is not part of
this bug.

## How to reproduce
1. Open a project in the editor.
2. Delete the scene's Light entity.
3. Press Play.
4. The game hits the assert instead of showing the scene unlit.
