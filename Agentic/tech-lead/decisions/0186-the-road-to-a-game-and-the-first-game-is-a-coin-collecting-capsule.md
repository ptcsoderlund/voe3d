# 0186 — The road to a game, and the first game is a capsule that jumps and collects coins
date: 2026-09-19
by: tech-lead

## Decision
Work after 009 moves toward one concrete game, made in the editor: **a third-person capsule that
runs and jumps through one level and collects coins (cylinders squashed flat by their scale);
collecting them all completes the level.** Work comes in six milestones, in this order, and each
one ends with something the sponsor can do in the editor that they could not do before:

1. **Build a level by hand.** Add, delete and duplicate entities, and add and remove their
   components. Built-in capsule and cylinder shapes next to the cube, each with a colour of its
   own. Inspector values typed as well as dragged. Move things in the view rather than only in
   the Inspector. Undo. The engine has no "coin": a developer makes one from a cylinder, a colour
   and a squashed scale, and the same goes for every other thing in the game.
2. **Press Play.** The editor builds the project's game and starts it as a program of its own
   (0187). At first the game only shows the scene through a game camera.
3. **Game logic.** The project's own C code runs every frame, and input is read as named actions.
   The capsule moves with the keys and a third-person camera follows it.
4. **Collision.** Simple shapes collide, a character stands on floors, walks against walls and
   jumps, and a trigger reports when the capsule touches a coin.
5. **A game.** A HUD (coins left) drawn with `ui`, a sound when a coin is taken, a "level
   complete" state and restarting the level. Prefabs and scene switching come in only if the
   coin game needs them.
6. **Ship.** Export the game as a standalone program for Linux and Windows with no editor in it.

Out of the road until a game needs them: parent/child transforms, skeletal animation, lights
beyond the one sun, music, and plugins (see `ideas.md`).

## Reasoning
A small, specific target decides what to build and in what order, and keeps anything else out.
The coin game uses every milestone and nothing more. Alternatives: a road with no target game,
which grows every feature to a general size; a top-down arena or a physics puzzle room, which the
sponsor did not choose.

## Replaces
nothing
