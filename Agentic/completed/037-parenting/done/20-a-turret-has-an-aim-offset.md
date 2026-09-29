# 20 — A turret has an aim offset
folder: examples
decisions: 0168, 0272, 0251, 0271

## Change
Bug 02. A model whose barrel is not along its own −Z aims away from the pointer; the Tank / Turret
component gains an offset in degrees that says where the barrel points. Agents never edit
`examples/tank_game/main.scene` (0251); its uncommitted change stays uncommitted. Read the header
comments of `examples/tank_game/Code/tank_turret.h` and `Code/tank_turret_system.c`, the comments
of `yaw_toward` and `turn_toward` in the latter, and `Code/Code.md`.

- `Code/tank_turret.h`: a second described field `aim`, `float`, `FLOAT32`, after `turn`:
  degrees about the turret's own up from its −Z to where its barrel points; default 0, so a
  turret that already aims right is unchanged. The header says what it is, its default, and
  that 180 fits a model facing +Z (as `tank_head.glb` does). Being described, the Inspector
  shows it and the scene saves it; nothing else to add for that.
- `Code/tank_turret_system.c`:
  - `tank_turret_register`: the default row sets `aim` to 0.
  - `turn_toward` (and its caller): the forward it turns toward the aim is the turret's −Z
    turned by `aim` about its own +Y, then by the world rotation — not bare −Z. The rest
    (world rotation found, turned about world +Y, made local under the hull by
    `voe_scene_transform_local`) is unchanged, so the offset holds on a turning hull.
  - The file header and `turn_toward`'s comment say the facing direction is the offset forward.
- `Code/Code.md`: the `tank_turret.h` entry names the aim offset (default 0).
- `examples/tank_game/tank_game.md`: the `Code` entry says hull and turret, not only the hull.

## Done when
`(for f in examples/tank_game/Code/*.c; do clang -std=c23 -fsyntax-only
-DVOE_BASE_DESCRIPTIONS=1 $(for d in */include; do printf -- "-I%s " $d; done) "$f" || exit 1;
done)` exits 0; `bash ~/.claude/skills/checks/scripts/checks.sh --all` prints `FINDINGS: 0`;
`d=$(mktemp -d) && build/debug/editor/voe_editor examples/tank_game --capture "$d/t.png" && test
-s "$d/t.png"` exits 0. The human's, on `examples/tank_game` with `tank_head` under `tank_body`:
the Inspector shows Aim at 0 on `tank_head`'s Tank / Turret; set 180, save, reopen, it is 180;
Play, the barrel points at the pointer, and still does while driving the hull round with A/D.
