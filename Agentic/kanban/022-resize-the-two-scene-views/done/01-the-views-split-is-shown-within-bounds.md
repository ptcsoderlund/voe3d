# 01 — The views' split is shown within bounds and set by share
folder: editor
decisions: 0168, 0226, 0228, 0229

## Change
The dock can lay out, bound and write the share between the two scene views. Nothing drags it yet (card
03) and nothing reads it from a file yet (card 02).

- `editor/src/dock.h`
  - `voe_editor_dock_place`: `least`, `most` and `shown` are now filled for every split, not only a held
    one. For a FRACTION split they are the FIRST child's length in mm: `least` its need, `most` the
    divided length less the second child's need, `shown` `fraction` of the divided length clamped to
    `least..most` (`most` winning, never below nought). Say so in the struct's comment.
  - `void voe_editor_dock_split_set(voe_editor_dock_tree *tree, uint32_t node, const
    voe_editor_dock_arrangement *arrangement, float length)`: the one write of a split's edge. A held
    split stores `length` clamped to the place's `least..most` as its `length` (a held SECOND child's
    length being what its side keeps); a FRACTION split stores the clamped first child's length over the
    divided length as `fraction`, kept strictly between nought and one. Asserts a split in range.
  - `double voe_editor_dock_view_share(const voe_editor_dock_tree *tree)` and
    `void voe_editor_dock_view_share_set(voe_editor_dock_tree *tree, double share)`: the `fraction` of the
    split whose two children are both SCENE_VIEW leaves; 0.5 / nothing written when there is none. The
    setter asserts nought < share < one.
  - Header: the "held length is what a drag writes" paragraph also names the views' share (0228, 0229):
    a share, so it holds as the window changes; the "nothing is rearrangeable" paragraph no longer claims
    nothing is dragged or saved. `voe_editor_dock_default`'s comment: the views' share starts at a half.
- `editor/src/dock.c`
  - `arrange_node` (and whatever it shares with the walk): a FRACTION split's first child gets the
    clamped `shown` above, so each scene view keeps `VOE_EDITOR_DOCK_VIEW_ROOM` along the split; the
    walk keeps taking every child's size from the arrangement.
  - The three new calls. The comment on node 3 in `voe_editor_dock_default` says its `fraction` is the
    share a person drags.
- `editor/src/src.md` — the `dock.h` and `dock.c` entries name the views' share.

Read `dock.h`'s header and `dock.c`'s file header and the heads of the functions you change; no other file.

## Done when
The Checks line of `CLAUDE.md` with `{folder}` = `editor` exits 0. With `XDG_CONFIG_HOME` at an empty
scratch folder, `voe_editor <scratch>/p --capture <scratch>/a.png --size 1280x720` shows the two views
half and half, as before.
