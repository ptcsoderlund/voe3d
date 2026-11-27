# 0520 — Trees are made in Blender, and the tree maker is withdrawn
date: 2026-11-27
by: tech-lead

## Decision
The engine has no tree maker. A tree is modelled in Blender, its procedural materials baked there into PBR
textures, exported as a glb and painted as an ordinary foliage kind, as any model is (090, 095): cut-out,
double-sided leaves, own or engine-made LODs, as import already gives. No `.tree` species file, no growing,
no species view, no bark layers: none of 117's tree code reaches `dev`. Work order 118 (moss and bare wood on
the bark) is withdrawn unbuilt; moss and bare wood are baked in Blender. 117's card 34 (branches grow out of
the trunk) is dropped. The hill shipped stays 119 and stays last.

117 stays in build under a new scope, a foliage group: the tree code is taken out of the shared code
(store, foliage draw, cooks, check, Docs, disk watch, Assets, Inspector, undo), and what is not about trees
stays and is finished. A foliage group replaces the foliage kind everywhere, in the panel and in the
sponsor's words: a group holds one or more glbs, each with its own LODs as any model has, and the group is
what is painted and what a material grows; each painted copy picks one of its glbs, the same every time
the project opens, keyed by the group's seed (0517 point 4 holds for groups). A landscape holds up to 16
groups. Each glb in a group has its own settings, the ones a kind has today (cast shadows, base size, size,
turn and tilt scatter, blend with ground and its height), and a share, how often it is picked against the
others, equal by default; the group holds its glbs, its seed and nothing else. A group is painted at one
density, one glb per spot: mixing is allowed, and things spaced alike belong together. A glb removed from a
group leaves its spots to the group's other glbs, which may then stand in other spots; one undo step brings
it back. A group can be removed from a landscape as 0516 says of a kind, one undo step bringing it back. A
project saved with kinds opens with each kind as a one-glb group, its trees standing where they stood.

## Reasoning
The sponsor (2026-11-27): "I want to admit that I was wrong. I want to back down from having an engine tree
generator. Its not good and it will never be good. It's better to do blender with procedural material(s) and
bake into pbr to export into this engine and just use as foliage." 0510 turned down whole Blender trees only
because they "would be made again once a tree maker came"; with no tree maker that reason is gone. A tree
tool good enough to compete with Blender's is a product of its own; the engine's job is to draw what the
artist makes well.
- Keep the tree maker and improve it: the sponsor judges it will never be good enough.
- Keep the tree code in the engine, unused: dead code every later change to the store and foliage draw would
  carry.
On 117 (2026-11-27), "1a 2a What if we just do a foliage group. One could and different trees and grass
into that group and the group is what is being painted. So multiple glbs in a foliage group and then we do
LOD's per glb as we do all? 16 groups (replace kind with group)?"; settings per glb: "its a cheap thing to
have ... I can imagine its not many glb's in a group so per glb should be fine"; a group's purpose: "a group
is only so i dont have to repaint same surface with different foliage kinds/instances". Keeping 117 keeps the seed and removal
already built and tested; the variation pick 117 built is what a group's pick becomes.
- Withdraw 117 as 094 was and order removal again on `dev`: the same end, rebuilding tested code.
- One glb per kind: six spruces use six of sixteen kinds and are painted one by one.
- Settings on the group, shared by its glbs: grass and trees in one group would share size, shadows, tilt.
- A group base with overrides per glb: the sponsor's own aside; few glbs per group make plain per-glb
  settings enough.
- Each glb at its own spacing within one stroke: a different, bigger feature than a group is for.
Lost with it: the grown skeleton 0510 named for per-branch wind and chopping into logs (0374). Neither is
built or asked for; when a game wants them they come back on Blender-made trees (wind weights or a skeleton
exported from Blender, a felled tree swapped in as its own model).

## Replaces
0510, 0513, 0514, 0515, 0518, 0519. Amends 0516 and 0517: their leaf roll and leaf width go; what holds of
their kind removal and kind seed is settled here.
