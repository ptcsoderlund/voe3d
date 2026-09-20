# 013 — Click to select in a scene view

## What
A left click in either scene view selects the frontmost entity under the pointer, exactly as clicking its row
in `Scene` does: the row is marked and the Inspector shows it. A click on empty space clears the selection.
The selected entity is marked in both views with a thin outline around its silhouette, in the theme's own
lightness and never a colour of its own (decision 0194), drawn on top so it shows even when something is in
front. Picking works for every drawn entity: the built-in shapes and imported models alike. A middle-button
drag still orbits the camera and never selects. While the file browser or Preferences shows, a click in a view
does nothing.

## Why
Finding a thing in a level by its name in a list is slow; pointing at it is how a person expects to pick it.

## How to test
1. Open a project with a few shapes, some behind others from a view's angle. Left-click one in the top view.
   Its row in `Scene` is marked, the Inspector shows it, and it has an outline in both views.
2. Click a shape that is partly behind another, on the part you can see. The one in front of the pointer is
   the one selected, never the one behind it.
3. Click empty space in a view. Nothing is selected, no outline shows, and the Inspector is empty.
4. Select a row in `Scene`. The same entity gets the outline in both views.
5. Select a shape, then place the camera so another shape is in front of it. Its outline still shows through.
6. Middle-drag in a view starting over a shape. The camera orbits and the selection does not change.
7. Change the selected shape's kind, colour or position. The outline follows it.
8. Switch to Near white and to another theme. The outline stays clearly visible and is grey or the theme's
   own hue, never a colour of its own.
9. Open the file browser and click in a view behind it. Nothing is selected.
