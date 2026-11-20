# 0510 — Trees are grown from materials and settings, in 0.3 before the hill gets its trees
date: 2026-11-20
by: tech-lead

## Decision
The editor gets a tree maker in 0.3, before the hill gets its trees. A tree species is a project file of
materials and settings, not a modelled mesh: the engine grows the trunk and branches as bent tubes in a bark
material, joined so a branch meets its parent cleanly, and sets leaves of a leaf material on the outer
branches. The sponsor chooses how many variations a species has, as with LOD levels; each variation is grown
once into one merged tree, its far levels made by the tree maker itself, and is grown again from the file
when the project loads, so nothing but the species file is kept. A species is one foliage kind: each painted
copy takes one of its variations by a seeded random, the same every run, under the size, turn and tilt the
brush already gives. Changing a species changes the trees already painted. On the bark lie layers, each a
material with rules for where it grows — up-facing side, near the ground, patchiness by noise, amount — so
moss, bare wood without bark, lichen are all the same kind of layer; layers are placed by rule, not painted
by hand. A tree modelled in Blender stays an ordinary foliage kind beside the grown ones. Work orders: 117
the tree grows, 118 bark layers; the hill shipped becomes 119. The road ahead is 116, 117, 118, 119.

## Reasoning
The sponsor (2026-11-20): "I have not made any trees yet and i am about to do that. If we get the tree system
now I dont need to do double work"; "With generated branches and leaves one kind would have many
variations"; "we should be able to merge branches and trunks in gpu into one object per tree"; "Should we do a
material and then the tree system makes trunks and branches from material and settings? That way our branches
would look good in the trunk connection"; "Just like LOD, dev can choose how many variations should be
generated. Then they get placed at random (seed random so its the same every run)"; "Lets do moss and
everything"; bare patches "Same as moss I should be able to do a barkless material and do patches of it, or
paint?". Grown branches are how tree tools work (SpeedTree, Blender's Sapling), and a tree that knows its
trunk and branches is what per-branch wind and chopping into logs (0374) need later.
- Whole trees modelled in Blender: the hill's trees would be made again once a tree maker came.
- Trunk and branch parts modelled in Blender and placed by the engine: joints would not meet cleanly.
- A unique tree per painted spot: one mesh each in GPU memory; a pool of variations under the brush's
  random size, turn and tilt cannot be told apart in a forest.
- Each branch drawn as its own instance: the same draw count, hundreds of thousands of tiny instances to cull,
  and no whole-tree far levels.
- Layers painted by hand: a variation is grown again whenever its species changes, which would lose the
  painting, and each variation would be painted on its own.

## Replaces
Amends 0507's road (106, 114, 115, 116, 117): the hill shipped is renumbered 119.
