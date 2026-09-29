# 17 — The tank game proves parenting
folder: examples
decisions: 0168, 0272, 0251, 0281, 0223
read: feature.md

## Change
Needs cards 14–16. The last card of 037. The turret is already in the tree (committed with the
blocked card 13): `examples/tank_game/Code/tank_turret.h`, `Code/tank_turret_system.c`,
`Code/project.c` running it after the hull, and its `Code/Code.md` entries. Read those four
files' headers and `Code/Code.md`; change nothing unless a check below fails inside `examples`.
If one fails outside `examples`, block again naming the folder and finding.

`examples/tank_game/main.scene` has an uncommitted change from the earlier try; leave it
uncommitted unless the sponsor asks.

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
