# 03 — Shells fly out the back of the barrel

## Seen
"The shells spawning does not respect the rotation offset for my barrel. So now they shoot 180
degrees wrong direction."

The barrel in the tank has its own rotation (it is turned relative to the turret/hull it sits on).
Pressing fire spawns shells that fly the opposite way to where the barrel points.

## Expected
A shell appears at the barrel's muzzle and flies the way the barrel points in the world, taking
into account the barrel's own rotation and every parent's rotation above it (turret, hull). Turning
the barrel's rotation in the Inspector turns where shells go by the same amount.

## How to reproduce
1. Open `examples/tank_game` in the editor.
2. Select the barrel; it has a rotation of its own (for example 180° about its up axis).
3. Press Play and hold fire.
4. Shells leave toward the back of the barrel instead of out of its end.
