# 065 — F frames the selection

## What
In the editor, pressing F with something selected moves the scene view under the pointer so the selection is
in front of it, as in other game engines. The view keeps its angle and glides there in about a quarter second.
It ends looking at the selection's middle, close enough that the selection fills about two thirds of the
picture. That size comes from the meshes of the selected item and its children. A light, a camera or an empty
item has no size of its own, so the view stops a fixed short distance from it. After that the view orbits the
selection. The other view does not move. With nothing selected, F does nothing. F is like every other
shortcut: it does nothing while typing in a field or while flying a view.

## Why
Finding the item you just picked in the Scene list means orbiting and zooming by hand.

## How to test
1. Open the tank game's scene. Select the tank in the Scene list, point at the top view and press F. The view
   glides, without jumping, to the tank, which fills about two thirds of the picture, seen from the same
   angle as before. The bottom view has not moved.
2. Drag to orbit. The view turns around the tank.
3. Select something small and far away, point at the bottom view and press F. The bottom view frames it the
   same way, and the top view stays on the tank.
4. Select something large, for example the ground. Press F: all of it, or nearly all, is in the picture.
5. Select a light, then the camera. Press F on each: the view stops close to it, and its marker is in the
   middle of the picture.
6. Select an item with children, for example a parent with a model under it. Press F: the parent and its
   children are in the picture.
7. Clear the selection and press F. Nothing moves.
8. Click into a name or number field in the Inspector and type an F. It goes into the field and no view moves.
   Fly a view and press F: nothing is framed.
9. Close and reopen the editor. The views open as before, on the scene's camera.
