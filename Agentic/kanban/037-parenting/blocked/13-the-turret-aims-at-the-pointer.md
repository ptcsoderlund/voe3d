# 13 — The turret aims at the pointer, and the tank game proves parenting
folder: examples
decisions: 0168, 0272, 0251, 0281, 0223
read: feature.md

## Change
Needs every card before it. The last card of 037.

- `examples/tank_game/Code/tank_turret.h`: the `tank_turret` component, key `tank_turret_key`,
  described: `turn` in degrees a second (default 180); menu `Tank / Turret`; its register and
  `void tank_turret_system_run(voe_ecs_world *, voe_platform_window *, double seconds)`.
- `Code/tank_turret_system.c`: each step, when the window's pointer is `over`: the world's first
  camera, at `voe_scene_transform_world`, with its lens, through `voe_3d_view` at the window's
  aspect (`voe_platform_window_size`), then `voe_3d_pick_ray` through the pointer
  (`3d/include/3d/pick.h`, `3d/include/3d/projection.h`, `platform/include/platform/input.h`);
  where the ray crosses the level plane through the turret's world position is the aim. The
  turret's world rotation is turned about +Y toward facing the aim with its −Z, by at most
  `turn`·seconds, and written back as its row with `voe_scene_transform_local` (it is the hull's
  child). A ray parallel to the plane or pointing away aims nowhere. Header points: aim in world,
  written relative (0271); why the plane; a barrel under it follows with no code.
- `Code/project.c`: registers the turret; runs it after the hull, before the move.
- `Code/Code.md`: its two entries; `project.c`'s entry names both systems.

For the human, `## How to test` in `feature.md`, all six steps, in the editor on
`examples/tank_game`, with a barrel `.glb` imported beside `tank_body` (hull) and `tank_head`
(turret). Before step 6 add `Tank / Hull` to the hull and `Tank / Turret` to the turret with
Add component, and point the scene's camera down at the tank. Commit the barrel and the scene
only if the sponsor asks.

## Done when
`(for f in examples/tank_game/Code/*.c; do clang -std=c23 -fsyntax-only
-DVOE_BASE_DESCRIPTIONS=1 $(for d in */include; do printf -- "-I%s " $d; done) "$f" || exit 1;
done)` exits 0; `bash ~/.claude/skills/checks/scripts/checks.sh --all` prints `FINDINGS: 0`;
`d=$(mktemp -d) && build/debug/editor/voe_editor examples/tank_game --capture "$d/t.png" && test
-s "$d/t.png"` exits 0. The six steps above are the human's.

## Blocked
The change is in and passes the syntax check, `checks.sh --folder examples` (FINDINGS: 0) and the editor capture, but `checks.sh --all` prints FINDINGS: 5: `.md` entries over the 300-character cap in `3d/tests/tests.md` (`pick.c`, `outline.c`), `editor/src/src.md` (`interface.c`, `scene.h`) and `scene/tests/tests.md` (`transform.c`), left by cards 03, 04 and 11. Those folders are outside this card's; a card per folder that shortens those entries unblocks it.
