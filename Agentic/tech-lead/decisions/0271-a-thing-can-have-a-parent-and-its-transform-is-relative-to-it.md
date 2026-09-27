# 0271 — A thing can have a parent, and its transform is relative to it
date: 2026-09-27
by: tech-lead

## Decision
Any thing in 3D space may have one parent. Its transform (position, rotation, scale) is then
relative to that parent, so it moves, turns and scales with it. The Inspector shows and edits the
relative values. A parent may have any number of children, nested to any depth, and never a loop.
The Scene list shows the tree. Dragging a thing onto another thing in the list makes it a child,
and dragging it out to the top makes it a root. In both cases it keeps its place in the world.
Deleting a parent deletes its children. A game's code can parent and unparent while playing. The
camera and the sun may be children like anything else. 0222 still holds: a transform is the only
thing that places anything in 3D space. The only change is that a transform can now be relative
to another transform.

## Reasoning
The tank needs a turret on the hull and a barrel on the turret. The same parenting later puts
lights on shells and a camera on a player. Alternatives: fixed offsets in game code (every game
rewrites parenting badly); grouping without relative transforms (it does not turn a turret).

## Replaces
nothing. It amends 0222.
