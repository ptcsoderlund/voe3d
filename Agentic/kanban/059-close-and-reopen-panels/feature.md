# 059 — Close and reopen panels

## What
I can close any editor panel I am not using, except the top bar and the top scene view: the Project panel, Scene
list, Assets, Inspector, Errors and the bottom scene view. Each has an × in its header. The top bar has a
**Panels** menu that lists those panels with a tick beside each open one, and clicking one opens or closes it.
When a panel closes, the panel beside it takes the space. When I open it again it comes back where it was, at
the size it had. The editor remembers which panels I had open, in every project, like it remembers their sizes.
If a build fails, the Errors panel opens by itself so I see why (0351).

## Why
More room for the scene views and the panels I actually use, without the cost of docking yet.

## How to test
1. Open the Panels menu in the top bar. Every closable panel is listed with a tick; the top bar and the top view
   are not listed.
2. Close the Inspector with the × in its header. The scene views grow into its space, and the Panels menu shows
   no tick beside Inspector.
3. Make the Scene list wider, then close it from the Panels menu. Reopen it from the menu: it comes back on the
   same side, at the width it had.
4. Close the bottom scene view. The top view fills the space. Reopen it: the two views share the space as before.
5. Close every panel the menu lists. Only the top bar and the top view remain, and nothing is drawn where the
   panels were. Reopen them one by one: each comes back in its place.
6. Close the Inspector and Assets, close the editor and start it again, in another project too: they are still
   closed, and the other panels are as I left them.
7. Close the Errors panel, then press Play on a project with a mistake in its code. The Errors panel opens by
   itself and shows the error.
8. Close a panel and drag the borders of the panels that remain: they resize as before.
