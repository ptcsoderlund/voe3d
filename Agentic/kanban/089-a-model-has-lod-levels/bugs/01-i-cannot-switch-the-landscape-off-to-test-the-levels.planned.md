# 01 — I cannot switch the landscape off to test the levels

## Seen
"I cannot test 089 properly. I need a "Enabled" switch per entity in editor so i can turn them off/on. This way I
can disable landscape and test models properly for LOD." I also want to put two lights in the scene and jump
between them by switching one off and the other on.

## Expected
Decision 0427. Every entity's Inspector has an Enabled switch at the top, on by default. Off, the entity is as
if it were not there: not drawn, no shadow, no light or bounce, no collision, no hits, sound or particles, and
clicking where it was selects what is behind it. Its children go off with it. The scene list still shows it,
greyed, so I can select it there and switch it back on. The switch is saved with the scene, undo takes it back,
and Play and a shipped game see it as I left it.

## How to reproduce
1. Open the hill project with its terrain and some models.
2. Select the terrain and look for a way to switch it off without deleting it. There is none.

## How to test
1. Select the terrain, switch Enabled off: the ground is gone, and so are its shadows. The terrain is greyed in
   the scene list. The Frame panel's Triangles drops.
2. Place two lights. Switch one off: its light and shadows are gone. Switch them the other way: the scene is lit
   by the other one only.
3. Switch off a parent with children: the children are gone too. Switch it on: they are back.
4. Click where the switched-off terrain was: it is not selected. Select it in the scene list and switch it on.
5. Ctrl+Z undoes a switch; Ctrl+Y does it again.
6. Switch the terrain off, save, close, reopen: it is still off. Press Play: it is still off there, and nothing
   collides with it.
7. With the terrain off, fly away from a model and watch the Inspector's Level and the Frame panel's Triangles.
