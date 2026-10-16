# 04 — Make levels changes nothing I can see

## Seen
No matter which strength I choose, I always get 30k triangles at far distance.

Found while looking: Make levels writes a new `Cache/levels/<path>.levels`, but the model already loaded in
the editor keeps the levels it was loaded with. `voe_game_models_watch` reloads a model only when its `.glb`
stamp changes (`game/src/models.c`, `load_changed`), and `main.c` does nothing after the prepare. So every
strength shows what was loaded when the project opened, until the project is opened again.

## Expected
Decision 0431. Make levels shows the new levels at once, without reopening the project.

## How to reproduce
1. Place a model, fly far away and read the Frame panel's Triangles.
2. Make levels on it with another strength: the Triangles far do not change until the project is opened
   again.

## How to test
1. Make levels with Levels 4 and Strong: after the splash, the Inspector's "of" count and the far
   Triangles change without reopening.
2. Make levels with Levels 2 and Gentle: the Inspector says "of 2" and the far Triangles rise.
