# 01 — Middle click does not reset the fly speed

## Seen
"The movement speed is not resetting when I do middle mouse button. Which I want it to do when I hold RMB."
In an editor view, pressing the middle mouse button while holding the right mouse button leaves the fly
speed as the wheel set it.

## Expected
While the right mouse button is held (flying), a press of the middle mouse button puts the fly speed back to
its starting value, the speed a new editor view flies at. Middle click without the right button held does
what it did before.

## How to reproduce
1. Open the editor on a scene with a terrain.
2. Hold the right mouse button in a view and turn the wheel up until flying is clearly fast.
3. Still holding the right mouse button, press the middle mouse button.
4. Fly with WASD: the speed is still the fast one, not the starting speed.
