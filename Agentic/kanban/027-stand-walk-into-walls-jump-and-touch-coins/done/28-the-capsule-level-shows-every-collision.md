# 28 — The capsule's level shows every kind of collision, proven
folder: examples/capsule
decisions: 0168, 0249, 0253, 0250, 0246, 0177
read: feature.md

## Change
Card 24's change is committed (99dcdab): the level in `examples/capsule/main.scene` and its
account in `examples/capsule/capsule.md`. It blocked only on two findings outside
`examples/capsule`, which cards 25 and 26 fix. Edit nothing unless a proof below fails; if one
does, the fix is in those same two files, to these points:

- Every static piece a `voe_3d_shape` of kind 1 (cube) with a `voe_physics_collider` of kind 1
  (box), size [1, 1, 1], scaled by its transform; unique ids, names saying what each is.
- Two floor slabs 0.1 thick, a 2 m gap across X; four 3 m walls round the larger; a gentle 15°
  and a steep 55° ramp, each 4 x 2 x 0.2 with its low edge on the floor; a ledge 0.25 m high,
  2 x 2.
- Five yellow coins, cubes scaled 0.4, 0.6 m up, each a sphere collider (kind 2) size
  [1, 1, 1] with `trigger = true` and a `coin` section; one on the ledge, one past the gentle
  ramp.
- The Player with a capsule collider (kind 3) size [1, 2, 1] and a `voe_physics_body` with
  every field at its default (`physics/include/physics/body_component.h`), standing at y 1.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder examples/capsule` prints `FINDINGS: 0`.
2. In `p=$(mktemp -d)`, `cp -r examples/capsule/. $p`, `rm -rf $p/Build`:
   `build/debug/editor/voe_editor --capture $p/shot.png $p 2>$p/err` exits 0, `$p/err` is empty,
   and `$p/shot.png` shows the level (0177: the coder looks at it).
3. `bash ~/.claude/skills/checks/scripts/checks.sh --all` prints `FINDINGS: 0`.
4. The human's: `feature.md`'s `## How to test`, steps 1–15, in the editor on `examples/capsule/`
   (the look of the lines, walking, jumping, coins and smoothness are seen, not
   checked by a program).
