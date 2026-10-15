# Make levels changes nothing I can see

No matter which strength I choose, I always get 30k triangles at far distance.

Found while looking: Make levels writes a new `Cache/levels/<path>.levels`, but the model already loaded in
the editor keeps the levels it was loaded with. `voe_game_models_watch` reloads a model only when its `.glb`
stamp changes (`game/src/models.c`, `load_changed`), and `main.c` does nothing after the prepare. So every
strength shows what was loaded when the project opened, until the project is opened again.

Make levels should show the new levels at once, without reopening the project.
