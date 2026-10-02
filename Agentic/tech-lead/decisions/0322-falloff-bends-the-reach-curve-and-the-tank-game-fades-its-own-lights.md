# 0322 — Falloff bends the reach curve, and the tank game fades its own lights
date: 2026-10-02
by: planner

## Decision
For 048 bug 01, carrying out 0321:
1. **The curve.** A point light d metres off lights by `saturate(1 − (d/range)^(2/falloff))²`. At
   falloff 1 that is 0320 point 4's `saturate(1 − (d/range)²)²`, so the default is today's look.
   Below 1 the exponent rises: an even pool whose rim still eases to nought. Above 1 it falls: a
   bright core that dims quickly. The light still ends at `range`; binning is unchanged.
2. **The values.** `falloff` is a FLOAT32 on `voe_scene_point_light` after `range`, default 1. The
   drain refuses one that is not finite or lies outside 0.25 to 4 (exponents 8 to 0.5), named as
   `VOE_SCENE_POINT_LIGHT_FALLOFF_LEAST` and `_MOST`. `render` carries it as authored in the
   record's spare word and asserts at pass begin on one not finite or not above nought.
3. **The Inspector.** A plain number box, as `range` is: the field is described, so the editor
   needs no code. A drag past the bounds is refused by the drain like any bad value.
4. **Files already saved.** The scene reader's own rule holds: a missing `falloff` reads the default
   row's 1, and the `flash` and `flash_when_made` keys name no field and are ignored with a warning,
   dropped by the next save. The tank game's saved files are rewritten so they open with no warning.
5. **The system.** With nothing to count down, `voe_scene_point_light_system_run(world)` takes no
   seconds, as the sun's does, and only drains the replaces.
6. **The tank game's flashes.** One component of its own, "Tank / Light fade" (`peak`, `seconds`,
   `left`), and its system: each step it sets the entity's point light intensity to
   `peak × left / seconds` through the replace, then counts `left` down. `tank_light_fade_start`
   restarts it; the gun and the enemy call it per shot. A wreck's prefab writes `left` as its
   `seconds`, so it fades from full when spawned.

## Reasoning
An exponent on the reach keeps one formula, one multiply and a pow in the shader, and makes the
default exactly 0320's. A power of the inner term (`(1 − q²)^k`) was the other choice: low k gives a
hard rim, which the bug rules out. The bounds keep the two ends meaningful; past them the pool is a
ring or a dot. Reusing the reader's missing-field rule means no migration code. One fade component
serves the shots and the wrecks, so the three flashes are one piece of game code.

## Replaces
0320 point 4's curve becomes point 1's at falloff 1.
