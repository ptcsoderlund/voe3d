# 038 — Prefabs and spawning in play

## What
A group of things can be saved as a prefab. Dragging a thing (with its children) from the Scene list
into the Assets panel makes a prefab file there. Dragging a prefab from the panel into a view places
a copy. Opening a prefab from the panel edits it in the views on its own, like a scene. When it is
saved, every placed copy in every scene takes the change, except for where each copy is placed. A
placed copy shows in the Scene list as a prefab, and its parts can still be selected. While the game
plays, its code can spawn a prefab by name at a place and remove anything, and the game does this
many times a second without stutter. In the tank game, the enemy tank is a prefab. Pressing fire
spawns a shell that flies forward and is removed after a while, and enemies are spawned by code.

## Why
Milestone 5 of 0268. Enemies, shells and wrecks come and go by the hundred while the game plays, and
the sponsor should build an enemy once and place it many times.

## How to test
1. In `examples/tank_game`, build a tank from parts. Drag the hull from the Scene list into the Assets
   panel. A prefab appears there.
2. Drag the prefab into a view three times. Three tanks stand there.
3. Open the prefab from the panel. The views show it alone. Give the turret a different model and
   save. Go back to the level. All three tanks and the original have the new turret, each still
   where it was placed.
4. Undo in the level, save, reopen. Everything is as it was left.
5. Press Play. Hold fire. Shells stream out of the barrel and vanish after a few seconds, with no
   stutter even after a minute. Enemy tanks appear from code while you play.
6. Stop. The level in the editor is unchanged: no leftover shells or enemies.
