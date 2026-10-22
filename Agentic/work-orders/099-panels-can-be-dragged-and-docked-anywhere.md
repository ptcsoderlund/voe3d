# 099 — Panels can be dragged and docked anywhere

## What
Decision 0439. "I want the frame panel to be movable. I would like to be able to drag it around and place it
where i want. No matter which panel." Any panel with a header (Frame, Scene list, Inspector, Assets and the
rest) can be dragged by its header. Dropping it on an edge of another panel or of the scene views docks it
there, and while I drag an outline shows where it will go. Dropping it anywhere else leaves it floating over the
editor, where I can move it by its header and resize it by its edges. Double-clicking a floating panel's header
puts it back where it was docked. The top bar stays put; the scene views take whatever room is left. My layout is
remembered after a restart. Everything stays inside the editor window, and there are no tabs.

## Why
The fixed layout puts panels where the editor chose, not where I work; the Frame panel in particular sits in my way.

## How to test
1. Drag the Frame panel by its header to the middle of a scene view and let go: it floats there over the view.
   Drag it around by its header; resize it by an edge. It cannot be dragged outside the window.
2. Drag it onto the right edge of the Inspector: an outline shows it will dock there. Let go: it docks, and the
   Inspector gives up room for it.
3. Drag the Scene list onto the left edge of the editor, the Assets panel under the Inspector, and the Inspector
   onto the bottom of the scene views: each docks where the outline showed, and the views fill what is left.
4. Drag a docked panel's border: it resizes as borders do today.
5. Double-click the floating Frame panel's header: it goes back to where it was last docked.
6. Close a moved panel with its × and reopen it from the Panels menu: it comes back where I put it.
7. Close and reopen the editor, and open another project: the layout is as I left it.
8. Delete the editor settings file and start: the layout is today's.
