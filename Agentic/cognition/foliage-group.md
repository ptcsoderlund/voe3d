# Foliage group

A landscape's foliage group, which replaces the foliage kind (0520): several glbs painted and grown as one. Until groups are built, today's kind is a one-glb group and the lines below about kinds hold for it.

## What a group is for
- A group exists only to spare repainting one surface with different foliage: "a group is only so i dont have to repaint same surface with different foliage kinds/instances"; see 0520. [intent · tech-lead · 2026-11-27]
- A group has one density, so glbs of very different size belong in different groups: "Super large and small flowers should maybe not be in the same group"; see 0520. [intent · tech-lead · 2026-11-27]
- Settings are per glb because a group holds few glbs and it is cheap: "I can imagine its not many glb's in a group so per glb should be fine"; see 0520. [intent · tech-lead · 2026-11-27]

## Its glbs
- Each glb's share, how often a copy picks it, is equal by default; the tech-lead's proposal, taken with the per-glb answer; see 0520. [assumed · tech-lead · 2026-11-27]
- A glb removed from a group leaves its spots to the group's other glbs, which may then stand in other spots; the tech-lead's call, not put to the sponsor. [assumed · tech-lead · 2026-11-27]
- Nothing in `.landscape` or the Foliage section stops two kinds of one landscape naming the same `.glb`, so a model path does not name a kind uniquely. [observed · planner-108 · 2026-10-30]
- The editor adds a kind by an Assets drop, with no undo step. [observed · planner-108 · 2026-10-30]

## Its box
- "I cannot fold foliage kind in inspector": the sponsor wants each foliage kind's box to fold (108). [intent · tech-lead · 2026-10-30]
- A kind's drop target is its title row, not the whole box. [observed · accept 108 · 2026-10-30]
- A folded kind's look is not refreshed, which is safe only while nothing reads it when folded. [observed · coder-108-06 · 2026-10-30]

## Its seed
- A foliage copy keeps x and z as whole millimetres, so a pick hashed from them is stable across save and reopen. [observed · planner 117 · 2026-11-20]
- A `seed<k>` equal to its kind's index means the same as no seed, so two looks meaning one seed compare equal; see 0517. [assumed · coder 117-25 · 2026-11-24]
- The lenient read clamps a whole-number `seed<k>` outside 0–15; one that is not a whole number is refused as malformed by both reads, as every look key's text is. [assumed · coder 117-25 · 2026-11-24]
- Every other landscape look key out of range is refused by the lenient read too; only `seed<k>` clamps. [observed · coder 117-25 · 2026-11-24]
- A look edit never changes a kind's seed. [assumed · planner 117-21 · 2026-11-24]

## Removing one
- The sponsor wants to remove one kind, a rock painted as foliage, and keep the landscape: "Now I want to remove that rock and dont have any rocks … All I try ends up deleting the whole landscape."; see 0516. [intent · tech-lead · 2026-11-24]
- A removal is made safe by undo, not by a confirm dialog: asked which, the sponsor chose undo, one step restoring every copy; see 0516. [confirmed · tech-lead · 2026-11-24]
- A kind brought back by undo comes back unfolded, and the removed kind is unpicked from the foliage brush; see 0517. [assumed · planner 117-21 · 2026-11-24]
- A foliage copy's kept level of detail is still keyed by kind index, so after a removal the later kinds' copies may show a wrong level for one frame before settling. [inferred · planner 117-21 · 2026-11-24]

## In the game
- The landscape cook writes each kind's look positionally, so a look field the cook omits is 0 in Play and Ship, not its default. [observed · planner 117-21 · 2026-11-24]
