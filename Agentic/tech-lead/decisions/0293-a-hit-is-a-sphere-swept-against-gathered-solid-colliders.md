# 0293 — A hit is a sphere swept against gathered solid colliders
date: 2026-09-29
by: planner

## Decision
For 041, 0249 layer 1's ray and sweep, beside 0253's overlap:

1. **One query, the sweep**: a sphere of `radius` from a double `from` along a float `motion`,
   answered with the first solid collider it touches. A ray is the sweep with radius 0; there is
   no second function.
2. **The caller gathers, then sweeps many.** `voe_physics_obstacles_gather` writes every solid
   collider's world shape, entity and bounding radius into storage the caller hands it, once a
   step; each sweep reads that array. The array is the caller's data, not a cache beside the
   collider, so a query still only reads (0249 rule 1). A trigger is not gathered: a sweep
   passes through it.
3. **A hit** is the entity, the fraction `t` of `motion` (0..1) at first touch, the point on the
   surface touched (double) and the surface's unit normal there, facing the sweep. A sweep that
   starts overlapping hits at `t` 0, at `from`, with the normal against `motion` (+Y when
   `motion` is nought). `ignore` names one entity passed over.
4. **Exact against every kind**, in float about `from` (0250): a sphere as a ray against a
   sphere of the summed radius; a capsule as a ray against the capsule of the summed radius
   (cylinder, then cap spheres); a box as a ray against the box rounded by `radius` — the slab
   test on the box grown by the radius, then, where the entry is outside the box on two or three
   axes, the ray against those edges as capsules. Nothing moves in steps, so nothing thin is
   tunnelled.
5. **The only broad phase is the bounding radius**: a segment farther from an obstacle's centre
   than its bounding radius plus the sweep's is passed over before the exact test.
6. **`game` gives colliders the room of drawn things** (`VOE_GAME_WORLD_MAX_DRAWN`), because
   spawned things now collide; bodies keep the authored room.

## Reasoning
A shell covers half a metre a step, so overlap in substeps (0253 point 4) would take tens of
tests a shell; a swept test is one and cannot miss. A gathered array keeps hundreds of sweeps a
step from composing every collider's world place hundreds of times, without the hidden cache
0249 forbids. Rejected: a separate ray function (the sweep at radius 0 is the same code); a box
grown with sharp corners (hits up to 0.7 radius early at a corner); a grid (tens of colliders do
not need one; 0253's header already names where one would go).

## Replaces
nothing. Fills in 0249 layer 1; 0253 point 3's "ray and sweep wait for a caller" is met.
