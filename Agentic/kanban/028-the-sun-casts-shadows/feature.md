# 028 — The sun casts shadows

## What
When a scene has a light, everything casts a shadow from it onto everything else, in both scene
views in the editor and in the game that Play starts. Turning or moving the light moves the
shadows at once. When the capsule jumps, its shadow stays on the ground below, so I can see
where it will land. Shadows are sharp close to the camera and still there far away, and they
stay still on the ground when the camera moves. A scene with no light is unlit and has no
shadows, as today. See decision 0252.

## Why
In a jumping game you need the shadow to judge where you land, and real shadows are the first
step toward dynamic lighting.

## How to test
1. Open `examples/capsule/` (with 027's level). The capsule, walls, ramps and ledge cast
   shadows on the floor in both scene views.
2. Select the light and turn it in the Inspector. All shadows swing round as you drag.
3. Stand the capsule under the ledge's edge in the editor: the ledge's shadow falls on the
   capsule, not only on the floor.
4. Press Play. Shadows are there in the game too. Jump: the capsule's shadow stays on the ground
   below it, and shrinks toward it as the capsule lands.
5. Walk around and let the camera follow. Shadows stay put on the ground; their edges do not
   shimmer or crawl.
6. In the editor, fly the view far out so the whole level is small on screen. Distant things
   still have shadows. Fly close to the capsule: its shadow edge is crisp.
7. Move every entity 100 km along X, as in 027 step 16. Shadows look the same as near the
   middle, in the editor and in the game.
8. Delete the light. Everything draws unlit with no shadows. Undo, and shadows are back.
9. The game runs as smoothly as it did before shadows.
