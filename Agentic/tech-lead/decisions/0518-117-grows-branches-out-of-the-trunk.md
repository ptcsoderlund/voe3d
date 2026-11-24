# 0518 — 117 grows branches out of the trunk
date: 2026-11-24
by: tech-lead

## Decision
Joins feature 117, and 117 is not accepted without it. Where a branch leaves the trunk, it grows out of it
rather than being pushed into it. The branch's base swells into the trunk's surface, stretched further along
the trunk than across it, like the collar of a real branch. The first stretch of the branch leaves closer to
the trunk's direction and bends out to its angle, so it does not leave at full angle from the first
centimetre. Near the joint, the shading and the bark follow the trunk's surface, so the light and the bark
do not break where the branch goes in. Branch and trunk stay separate geometry: nothing is welded, and the
swelling hides where they cross. Only joints on the trunk get this. Branches on branches and twigs keep
today's joint, to save vertices. Every existing species changes at its trunk joints; that is the point, and
no setting brings back today's joint. The far levels keep the joint's look as far as their fewer sides allow,
and a swap stays within 0514's measured errors. How the swelling is shaped and sized, and whether any of it
becomes a species setting, is the planner's choice. Amends 0515 point 2's collar, "widened to the parent's
surface radius", and the halfway collar ring built from it, for trunk joints.

## Reasoning
The sponsor, testing 117: branches "are just slapped on the trunk, looks like they are glued to the trunk
instead of growing from it". That undercuts the reason 0510 grows trees from settings ("our branches would
look good in the trunk connection"). Today a branch starts on the trunk's centre line, its one wider ring
usually still inside the trunk, at full angle, with a crease in the light and a break in the bark. What reads
as glued is that crease and the sudden angle, not the two pieces of geometry. Alternatives: weld each branch
into the trunk as one skin, which far levels with 3-sided tubes cannot take, and which the swelling and
blended light already hide; weld near and swell far, kept for later if this still looks glued up close; every
joint, which the sponsor turned down to save vertices on branches. Inside 117 like 0516: a grown tree with good
joints is what 117 is for.

## Replaces
nothing
