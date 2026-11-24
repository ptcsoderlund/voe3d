# 0519 — A trunk joint is a flare from an ellipse on the trunk to the branch
date: 2026-11-24
by: planner

## Decision
How 117 builds 0518. Mesh only: the skeleton is unchanged, so a branch still starts on the trunk's axis.
A trunk joint is the joint of an order-1 limb (a branch); twigs keep today's start ring and halfway collar.
Names: `r` the branch's base radius, `d` its first segment's unit direction, `t` its trunk segment's,
θ the angle between them, `R` the trunk's radius there.

1. **Where.** J0 is where the branch's straight line leaves the trunk's round surface; `outside` the length
   of its first segment beyond J0. The collar length is L = min(max(1.25 r / sin θ, 1.5 r), outside / 2),
   so the branch's own ring at E = J0 + L·d clears the trunk. The joint's centre J is on the trunk's surface
   L back along −t from J0, at J0's angle about the trunk; so J→E leaves at θ/2 from the trunk, exactly,
   and the branch bends out to θ at E. A branch whose first segment never leaves the trunk keeps today's
   joint.
2. **The base ring** lies on the trunk: an ellipse about J in the trunk's tangent plane, half-axes
   A = min(max(1.5 r / sin θ, 2.25 r), 4 r) along `t` and B = 1.5 r around, pushed radially onto the trunk
   as that level draws it (its facets, from the trunk's ring frame and side count at that level). Vertex k
   sits at the direction of the branch's ring offset o_k seen along the trunk's normal, so it pairs with
   vertex k of every branch ring and the tube does not twist.
3. **The flare.** Each collar ring's vertex k lies on the straight chord from base vertex k to the
   branch's ring vertex at E (E + r·o_k): at shares 1/3 and 2/3 at levels 0–1, 1/2 at levels 2–3. A chord
   leaving the trunk outward never re-enters it, so no flare vertex is inside the trunk. Beyond E the tube
   is today's; the ring at the branch's start and its halfway collar are gone.
4. **Light and bark.** One weight, w = 1 − smoothstep(share), blends each vertex's normal (the trunk's
   radial normal there, then o_k, renormalised) and its uv (the trunk's there, then the branch's own). The
   trunk's u at a point is J's u plus the trunk's turns × the signed angle about the trunk from J / 2π, so it
   never wraps across the ring; its v is the trunk's metres along its axis there. The base ring therefore
   has the trunk's normal and uv exactly: no crease, no break.
5. **Far levels** build the same joint with their own sides and collar rings, from the same J, E, A, B,
   which come from the skeleton alone; only the snap onto the trunk follows the level's facets. No new
   species key; every species changes at its trunk joints.

## Reasoning
- Chords, not a curved fillet: a curve through the plain tube's inside could dip into the trunk and bring
  the crease back; a chord from a surface point outward provably stays out.
- J set back by L makes "leaves nearer the trunk's direction" exact (θ/2) for every angle, 5° to 120°,
  without touching the skeleton that wind and chopping will take.
- 1.25 r / sin θ is where the low side of a ring of radius r clears the trunk by about r/4; the 1.5 r and
  4 r bounds keep a steep branch's collar visible and a shallow one's from running down the trunk.
- One weight for normal and uv keeps light and bark changing together, as bark does on a real collar.
- No setting: 0518 wants every species to change, and one shape fits spruce and birch at their angles.

## Replaces
Nothing. Carries out 0518; replaces 0515 point 2's halfway collar ring for trunk joints.
