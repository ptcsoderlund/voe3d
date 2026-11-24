# 117 — A tree grows from materials and settings

## What
I make a tree species in the Assets panel. It is a small file of settings, not a model: the editor grows its
trunk and branches from a bark material and its leaves from a leaf material. In the Inspector I set how tall
it grows, how thick the trunk is, how its branches split, how far they reach, their angle and droop, how
crooked it all is, the leaf material, leaf size and how dense the leaves are, and how many variations the
species has. Each variation is a different tree of the same species; together they are a pool. Branches meet
their parent cleanly. Opening a species shows it alone in the view, where I step through its variations
while I tune it.

A species is one foliage kind in the Landscape panel: painting it, each tree takes one of its variations at
random, the same every time the project opens, with the size, turn and tilt the brush already gives. A
material can grow it as it grows any kind. When I change the species, the trees I already painted change
with it. Far trees get simpler calmly, as models do (106), and the forest stays within its frame budget
(114). Nothing but the species file is kept on disk for the trees. A tree I modelled in Blender is still an
ordinary kind beside the grown ones. The species file is taken in when it changes on disk (115) and
described in `Docs/` (116). (0510)

A spruce has a small branch on each leaf card, and those lie flat in layers along its branches rather than
fanning out around the twig. So a species also has a leaf roll, from the random roll leaves get today to
lying flat, and a leaf width, so a card can be narrower or wider than long to fit the sprig I drew. A species
file without them grows as before. And a foliage kind I no longer want — a rock I added — can be removed from
a landscape without deleting the landscape: its painted copies go, the other kinds stay as they were, and
undo brings it back. (0516)

## Why
I am about to make the hill's trees. Growing them from settings means I make each species once, every tree in
the forest varies, and the engine knows each tree's trunk and branches for wind and chopping later.

## How to test
1. In Assets, create a spruce species. Give it a bark material and a needle-leaf material. Open it: a tree
   stands in the view.
2. Drag each setting in turn — height, trunk thickness, branching, reach, angle, droop, crookedness, leaf size,
   leaf density. The tree changes as I drag, without a hitch. Where a branch leaves the trunk it looks joined,
   not stuck on.
3. Set variations to 6 and step through them: six spruces, clearly kin, clearly not copies. Set it to 1: one.
4. Make a birch from the same two kinds of material and different settings. It reads as a different tree.
5. In the Landscape panel add both species as kinds and paint a stretch of hillside. Look along it: I cannot
   pick out repeats.
6. Close and reopen the project: every tree is the same variation in the same spot as before.
7. Change the spruce's droop. Every spruce already painted droops more; the birches are untouched.
8. Let a ground material grow the birch by itself. Birches come up on it.
9. Fly away from the forest: trees get simpler without popping, and the frame cost of the forest stays under
   114's budget. Turn shadows on for the kind: the trees cast them.
10. Paint a tree I modelled in Blender beside them: it works as before.
11. Look in `Assets/`: the species are small files, no tree meshes beside them. Edit the spruce's file by hand
    in a text editor while the editor is open: the spruces change. `Docs/` describes the species file.
12. Press Play, then ship: the forest looks the same in both.
13. Open the spruce and set leaf roll to 1: the sprigs lie flat in layers along each branch, not fanned around
    it. Drag it back to 0: as before. Drag leaf width: the cards get narrower or wider and the sprig is no
    longer squashed; leaf size still sets their length. Fly away: the far trees keep the flat layers and the
    sprigs' shape, without popping.
14. A species file made before these settings existed looks exactly as it did. Edit leaf roll in the file by
    hand: the spruces change. `Docs/` describes both settings.
15. In a landscape with a rock kind painted, remove the rock kind in the Foliage section: every rock goes, the
    landscape and the other kinds stay, and the trees are the same trees in the same spots. Undo: the rock
    kind is back with every rock where it was. Redo, save, close and reopen: no rocks.
