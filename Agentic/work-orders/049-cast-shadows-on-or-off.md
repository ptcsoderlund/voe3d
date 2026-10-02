# 049 — Cast shadows on or off

## What
The directional light and every shape and model have a "Cast shadows" checkbox in the Inspector. A new light's is off and a new mesh's is on (0316).
With the light's box off, the light still lights things but nothing it lights casts a shadow. With a mesh's box
off, that mesh is still lit and still darkened by other things' shadows, but casts no shadow of its own. Prefabs
keep the setting, and the game looks the same as the editor. See 0301.

## Why
Some things, such as a flat decal, a light's glass or a distant backdrop, should not throw a shadow even under a
sun that does.

## How to test
1. Open `examples/tank_game`. Select the sun: the Inspector shows Cast shadows, ticked. Untick it. Every shadow
   in both scene views disappears and the level stays lit. Tick it again: the shadows come back.
2. Select a house. Untick its Cast shadows. The house's shadow on the ground disappears, while the tank's shadow
   still falls across the house when the tank drives next to it.
3. Undo and redo the change: the shadow goes and comes back each time.
4. Save, close and reopen the project. Both settings are as you left them.
5. Untick Cast shadows on a part inside a prefab and save the prefab. Every copy of it in the level loses that
   part's shadow.
6. Play. The shadows match what the editor showed.
7. Open a scene saved before this change. Everything casts shadows as before.
8. Start a new scene. Its light's Cast shadows is unticked and nothing casts a shadow. Add a light: its box is
   unticked too. Add a shape: its box is ticked.
