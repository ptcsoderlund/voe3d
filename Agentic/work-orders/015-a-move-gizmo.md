# 015 — A move gizmo

## What
The selected entity shows a move gizmo in both scene views: three arrows along the world's X, Y and Z axes and
a small square for each of the three planes between them, at the entity's position. Dragging an arrow with the
left button moves the entity along that axis only; dragging a square moves it in that plane only. It moves live,
following the pointer, and the Inspector's position follows. One drag is one change: it marks the project unsaved
and one Ctrl+Z (014) puts it back. The gizmo is drawn on top of everything and stays the same size on screen
however far away the entity is. The arrows tell themselves apart by where they point and by their labels, not
by colour alone, in keeping with decision 0194; a handle under the pointer or being dragged is marked the way
0194 marks any hovered or pressed control. A left click that is not on a handle still selects (013). Only world
axes for now; following the entity's own rotation is a later toggle.

## Why
Laying out a level by typing positions is slow; dragging a thing where you want it is how a level editor works.

## How to test
1. Select a cube. Both views show three arrows and three plane squares at its centre, drawn over the cube and
   over anything in front of it.
2. Drag the X arrow. The cube slides along X only, following the pointer; Y and Z in the Inspector do not change.
   Do the same for Y and Z.
3. Drag the square between X and Z. The cube moves across the floor plane and its Y stays the same.
4. Rotate the cube in the Inspector, then drag the X arrow. It still moves along the world's X, not its own.
5. Orbit the camera close and far. The gizmo stays about the same size on screen.
6. Hover over an arrow, then press on it. It is clearly marked in each state, in the theme's own lightness.
7. After a drag, the project is marked unsaved. Ctrl+Z puts the cube back where the drag began, in one step.
8. Click on empty space away from the handles. The selection clears and the gizmo goes away. Click another shape.
   The gizmo moves to it.
9. Middle-drag starting on a handle. The camera orbits; the entity does not move.
