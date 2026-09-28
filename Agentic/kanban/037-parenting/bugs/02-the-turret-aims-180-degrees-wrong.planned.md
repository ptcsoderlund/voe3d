# 02 — The turret aims 180 degrees wrong

## Seen
In Play, the tank head turns toward the mouse pointer, but the barrel points
straight away from it. The barrel is part of `tank_head.glb`, and the model faces
the other way from what the turret assumes. There is no way to correct it.

## Expected
The Tank / Turret component has an aim offset in degrees, shown in the
Inspector and saved with the scene. It defaults to 0, so a turret that already
aims right is unchanged. The turret turns so that its forward, turned by the
offset about up, faces the pointer. With the offset at 180 on `tank_head`, the
barrel points at the pointer. The offset works the same when the head rides on
a hull that is turning.

## How to reproduce
1. Open `examples/tank_game` with `tank_head` parented under `tank_body`.
2. Press Play and move the mouse around the tank.
3. The barrel points away from the pointer. The Inspector has no field to fix
   it.
