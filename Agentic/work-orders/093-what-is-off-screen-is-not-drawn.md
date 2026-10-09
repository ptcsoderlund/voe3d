# 093 — What is off screen is not drawn

## What
A view draws only what it can see. Things behind the camera or outside the edges of the picture cost
nothing in the camera's pass. The sun's shadows draw only the casters that can throw a shadow into the
view. A thing out of sight still throws its shadow into the picture, and nothing pops in at the edges
while flying or turning. The frame breakdown shows, for each view, how many things it drew and how many
it skipped. Editor views, Play and the shipped game all do this.

## Why
The hill's terrain, forest and grass put far more in the world than is ever on screen at once. Drawing
only what is seen is the cheapest win before them (0375 leaves indirect and GPU culling for later).

## How to test
1. Open a scene with many things in it, such as the hill. Note the drawn and skipped counts and the
   camera pass's time in the frame breakdown.
2. Turn to look at the sky, then at an empty corner. The drawn count falls and the skipped count rises,
   and the camera pass gets cheaper.
3. Fly and turn quickly past things at the edge of the picture. Nothing pops in or flickers at the edges.
4. With the sun low, stand with a tall thing just behind me or just off the side of the picture. Its
   shadow still lies on the ground in view.
5. Open the second scene view and select the scene camera. Each picture shows its own counts.
6. Press Play, and ship the project: the same counts behave the same way, and the picture is unchanged.
