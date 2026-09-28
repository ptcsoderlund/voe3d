# 037 — Parenting

## What
A thing can sit on another thing and move with it, as 0271 says. The Scene list shows things as a
tree. Dragging a thing onto another in the list makes it a child, and dragging it out to the top
makes it stand alone again. Either way it stays where it is in the world. A child's position,
rotation and scale in the Inspector are relative to its parent. Moving, turning or scaling a parent
carries its children along, in the editor and in the game. Deleting a parent deletes its children.
Undo, save and Play all keep the tree. A game's code can parent and unparent things while it plays.
In the tank game, the tank is a hull with a turret on it and a barrel on the turret. Driving it
turns the hull, and the mouse (a stick comes in 040) turns the turret, with the barrel following.

## Why
Milestone 4 of 0268. The tank is three rigid parts that turn separately but move together.

## How to test
Have the hull, turret and barrel `.glb` files imported.
1. Place a hull, a turret and a barrel. Drag the turret onto the hull in the Scene list, then the
   barrel onto the turret. The list shows them nested, and nothing moves in the view.
2. Move the hull with the gizmo. The turret and barrel go with it. Rotate the turret. The barrel
   turns with it, and the hull stays still.
3. Select the barrel. Its Inspector position is small numbers, relative to the turret.
4. Drag the barrel out to the top of the list. It stays where it was in the world. Undo, and it is
   the turret's child again.
5. Delete the hull. All three go. Undo. All three come back, still nested.
6. Save, reopen, Play. In the game, WASD drives the hull, and the turret and barrel ride on it. The
   turret turns toward the mouse pointer.
