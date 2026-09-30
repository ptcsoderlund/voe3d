# 043 — Entities without a transform

## What
Add entity makes an entity with just a name and no transform. The transform becomes an ordinary component:
I can add it from Add component, and remove it when nothing else on the entity needs it. When I add a
component that needs a place in the world, such as a shape, a model, a camera or a light, the entity gets a
transform along with it. An entity without a transform is only data, such as game state my code reads.
It shows in the Scene list but in no scene view. It can have children, so I can group things under it,
and any entity with children can be collapsed and expanded in the Scene list. See 0300.

## Why
Game state is written as ECS, so it is entities, and those have no place in the world. With a hundred
lamps coming (048), I want them under one entity I can fold away.

## How to test
1. Open `examples/tank_game`. Press Add entity. A new row appears in the Scene list. The Inspector shows
   its name and no transform. Nothing new appears in either scene view.
2. On that entity, choose Add component → Transform. The Inspector shows a transform at the origin, and
   it can be removed again.
3. On another new entity, add a shape. A transform comes with it and the shape shows at the origin. The
   transform offers no Remove while the shape is there. Remove the shape, and the transform can be removed.
4. Make a bare entity named "Lamps". Drag three things with shapes onto it in the Scene list. None of them
   moves in the scene view. Collapse "Lamps": its rows fold away. Expand it again: they come back.
5. Collapse "Lamps", save, close the editor and open the project again. "Lamps" is still collapsed.
6. Undo and redo each step above. Each one goes back and forth cleanly.
7. Give a bare entity one of the tank game's own components that needs no transform and press
   Play. The game plays as before.
8. Open a scene saved before this change. Everything is where it was.
