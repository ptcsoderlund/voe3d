# 21 — The capsule's level shows every kind of collision
folder: examples/capsule
decisions: 0168, 0249, 0253, 0250, 0246, 0177
read: feature.md

## Change
Feature step 1: the level, by hand in `examples/capsule/main.scene`, the format as it is today
(`authoring/include/authoring/scene_write.h` for the spelling; positions may carry decimals).
Every static piece is a cube shape with a box collider of size 1, scaled to its size; ids stay
unique, names say what each is. Heights are to the top of the floor at y 0.

- Two floor slabs 0.1 thick with a gap of 2 m between them across X, the capsule's side the larger.
- Four walls 3 m high round the larger slab.
- Two ramps, each a slab 4 m long, 2 m wide and 0.2 thick with its low edge on the floor: a
  gentle one tilted 15°, a steep one tilted 55°.
- A low ledge 0.25 m high, 2 m by 2 m (under the body's 0.3 m step).
- Five coins: cube shapes scaled 0.4, a yellow colour, 0.6 m up, each a Sphere collider size 1
  with `trigger = true` and a `coin` row; one on the ledge, one past the gentle ramp.
- The Player keeps its shape and `player` and `keyboard_input`, gains a Capsule collider
  (1, 2, 1), a kinematic body at its defaults, and `jump_height`/`gravity` at the defaults; stands
  at y 1. Camera and Sun as now.
- `examples/capsule/capsule.md` — what the level holds and what each piece shows.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder examples/capsule` prints `FINDINGS: 0`.
2. In `p=$(mktemp -d)`, `cp -r examples/capsule/. $p`, `rm -rf $p/Build`:
   `build/debug/editor/voe_editor --capture $p/shot.png $p 2>$p/err` exits 0, `$p/err` is empty,
   and `$p/shot.png` shows the level (0177: the coder looks at it).
3. `bash ~/.claude/skills/checks/scripts/checks.sh --all` prints `FINDINGS: 0`.
4. The human's: `feature.md`'s `## How to test`, steps 1–15, in the editor on `examples/capsule/`
   (the look of the lines, walking, jumping, coins and smoothness are seen, not
   checked by a program).
