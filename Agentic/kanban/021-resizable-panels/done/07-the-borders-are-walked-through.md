# 07 — The borders are walked through
folder: editor
decisions: 0168, 0220, 0226, 0227
read: feature.md

## Change
No product code by default: this is the walk that proves the feature, and whatever it shows to be wrong.
Card 05's `editor/src/settings.c` finding is already fixed and committed; card 06 trimmed the last
finding outside `editor`.

The human walks `feature.md`'s seven steps at a running `voe_editor` on two projects and reports what does
not hold. A failing step is fixed here, in at most one of `editor/src/resize.c`, `dock.c`, `topbar.c`,
`settings.c` and `main.c`; read the header of the one you touch and no other file. A pointer whose shape
never changes on this compositor while `wayland-info` lists `wp_cursor_shape_manager_v1`, or a fault in
the shape itself, is `platform`'s: the card is blocked, naming it.

## Done when
The coder: `bash ~/.claude/skills/checks/scripts/checks.sh --all` exits 0.

The human, at a running `voe_editor`, sees every step of `feature.md` hold: the pointer turns to a
left-right arrow over the Scene list's and the Inspector's borders and an up-down arrow over the top bar's;
each drag resizes its panel as it moves; the side panels stop at a usable width and the bar at its content;
the two side panels dragged together leave the views room; a double-click puts one size back and leaves the
others; after a restart, and on another project, the sizes are as left.
