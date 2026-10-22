# 74 — The editor's device has room for foliage in the bounce sun maps
folder: editor
after: 72
decisions: 0168, 0450, 0451

## Change
Bug 04 in the editor's preview and views: card 72 draws every foliage group
not `rest` into each bounce sun map a frame relights, one object a group, so
the device needs that room per view and for the preview.

- `editor/src/view_passes.h` — `VOE_EDITOR_CAPACITIES`' `.objects`: the
  foliage term's per-view multiplier, now 1 + the cascades, grows by
  VOE_RENDER_BOUNCE_VOLUMES × VOE_RENDER_DIRECTIONAL_LIGHTS, still times
  (VOE_EDITOR_VIEWS + 1). The comment above the macro no longer says "in no
  other pass": it names the bounce sun maps (0450), captures and point
  shadows still none. The header's top comment is near its cap; the
  reasoning stays above the macro.

No call changes: `editor/src/view_passes.c` already gathers the foliage
before the shadows, and the bounce reads the store the frame hands it.

## Done when
The editor builds, and `grep -n "VOE_3D_FOLIAGE_GROUPS" -A4
editor/src/view_passes.h` shows VOE_RENDER_BOUNCE_VOLUMES in the foliage
term.

The human, in the editor (the bug's How to test):
1. In the lumberhill scene with `Hill.landscape` and its trees of `tree
   asset.glb`, set the Light's bounces to 1: leaves over the shaded forest
   floor take only a faint bounce; leaves at the sunlit edge over open grass
   a clear green one.
2. Erase the trees in one patch with the brush: the bounce there brightens
   to open-grass strength once the probes relight, with no hitch.
3. Bounces 0: the picture is as before this change.
