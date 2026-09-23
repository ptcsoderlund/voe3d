# 04 — `sprite` names no camera forward
folder: sprite
decisions: 0168, 0222

## Change
`voe_scene_camera_forward` is gone (card 01); a camera's facing is its transform's rotation.

- `sprite/include/sprite/quad.h` — the "does not billboard" paragraph says the facing rotation is built from the
  camera's transform at the call site, in game code, instead of naming `voe_scene_camera_forward`. Comment only.

## Done when
`grep -rn "camera_forward" sprite` prints nothing, and the Checks line of `CLAUDE.md` with `{folder}` = `sprite`
exits 0.
