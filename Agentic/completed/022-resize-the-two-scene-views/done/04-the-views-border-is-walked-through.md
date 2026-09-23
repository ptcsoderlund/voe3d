# 04 — The views' border is walked through
folder: editor
decisions: 0168, 0226, 0227, 0228, 0229
read: feature.md

## Change
No product code by default: this is the walk that proves the feature, and whatever it shows to be wrong.

The views are stacked (0229): read `feature.md`'s "left"/"right" and "wider"/"narrower" of the views as
"top"/"bottom" and "taller"/"shorter", and "drag it right" as "drag it down".

The human walks `feature.md`'s eight steps at a running `voe_editor` on two projects and reports what does
not hold. A failing step is fixed here, in at most one of `editor/src/resize.c`, `dock.c`, `settings.c`
and `main.c`; read the header of the one you touch and no other file.

## Done when
The coder: `bash ~/.claude/skills/checks/scripts/checks.sh --all` exits 0.

The human, at a running `voe_editor`, sees every step of `feature.md` hold: the pointer turns to an up-down
arrow over the seam between the views; dragging it resizes both views as it moves and both keep drawing;
each view stops at a usable minimum; at about 70/30 the share holds as the window is resized and as the
Scene list's or Inspector's border is dragged; a double-click on it gives half and half and leaves the side
panels; after a restart, and on another project, the share is as left.
