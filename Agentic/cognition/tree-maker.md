# Tree maker

## What is wanted
- The sponsor wants trees of many shapes from a trunk and a branch or two, varied by the engine, "without a stored version of every tree"; see 0510. [intent · tech-lead · 2026-11-20]
- No trees are made yet; the tree maker comes first so the trees the sponsor makes are made once and reused, "I dont need to do double work"; see 0510. [intent · tech-lead · 2026-11-20]
- The sponsor wants each tree drawn as one object, branches and trunk merged, "so we avoid drawing many instances of small objects"; see 0510. [intent · tech-lead · 2026-11-20]
- Off-standard tools are welcome when they are fun: "we can do stuff thats not following standards just for the fun of creativity". [intent · tech-lead · 2026-11-20]
- The sponsor's own idea is "a theory in my head", and they do not know the industry standard here: they want told what would actually work and what code can solve. [intent · tech-lead · 2026-11-20]

## The bark
- Trunk and branches are grown from a bark material and settings, the sponsor's own proposal, so branches look right where they meet the trunk; see 0510. [confirmed · tech-lead · 2026-11-20]
- Moss is in the first round: "Lets do moss and everything", with patches of bare wood the same way. [confirmed · tech-lead · 2026-11-20]
- Moss and bare wood are placed by rules with noise, not painted by hand, since a variation is regrown whenever its species changes and would lose painting: "lets not do hand painting"; see 0510. [confirmed · tech-lead · 2026-11-20]

## Its leaves and far levels
- A leaf is a textured plane, a card of the leaf material: "yes, a leaf should be a textured plane"; see 0513. [confirmed · tech-lead · 2026-11-20]
- Measured speed decides between one card per leaf and cards that gather leaves and twigs: "Whichever gives us best performance"; see 0513. [intent · tech-lead · 2026-11-20]
- The sponsor is making a spruce, its leaf texture a small spruce branch, not one leaf: "I am doing a spruce so i have small branches as 'leaves'". [intent · tech-lead · 2026-11-24]
- Leaf cards pointing in every direction look wrong for a spruce branch: "This might fit leaves but maybe not spruce branch"; see 0516. [intent · tech-lead · 2026-11-24]
- The sponsor draws the sprig picture at the card's width-to-length ratio, since the picture stretches over the card; see 0516. [confirmed · tech-lead · 2026-11-24]
- Whether a spruce's overall shape (a cone, long lower branches, in tiers) can be grown with today's settings is undecided: asked, not answered. [open · tech-lead · 2026-11-24]
- The sponsor asked whether the leaf could be a single needle instead: "I cant just do one needle and use that as 'leaf' can I?" [intent · tech-lead · 2026-11-24]
- A leaf's alpha cuts out only when its Leaf `.material` has Cutout above 0; the default 0 is opaque, which is what made the alpha look solid: "cutout fixes it". [confirmed · tech-lead · 2026-11-24]

## Opening a species
- Opening a species shows it alone in the view with a stepper through its variations, as opening a prefab does. [assumed · tech-lead · 2026-11-20]
- Create → Tree in Assets makes the next free name, `Tree.tree`, then `Tree 2.tree` up to `Tree 99.tree` as Duplicate names; past 99 it is refused with a notice. Only an untitled project refuses it, not an open prefab. [assumed · coder 117-09 · 2026-11-20]
- A `.tree` in Assets opens its species and is no prefab; held, it drops onto Foliage kind rows only, never into a view or onto a model. [assumed · coder 117-14 · 2026-11-20]
- The species view is lit by the untitled scene's own sun, so the two cannot drift. [assumed · coder 117-11 · 2026-11-20]
- Save with a species open writes `project.voe3d` and no world; the set-aside level keeps its unsaved flag until Back. [assumed · planner 117 · 2026-11-20]
- A species opened before its row is read waits closed until the row comes. [assumed · coder 117-12 · 2026-11-20]
- An open material wins over an open species in the Inspector, being opened by a later explicit click. [assumed · coder 117-12 · 2026-11-20]
- While a species is open, an Inspector drop does only Bark or Leaf; every other is refused, as with an open material, since the section hides what they would edit. [assumed · coder 117-14 · 2026-11-20]
- The Variation stepper wraps both ways; a refused step says "The variation could not be shown." [assumed · coder 117-12 · 2026-11-20]
- Stepping a variation is no edit and records nothing, so an undo that applies the species' step shows again the variation the stepper shows. [assumed · coder 117-13 · 2026-11-20]
- An editor `--capture` of a project holding a `.tree` under `Assets/` aborted on an assertion that the tree read to no variations, though the file held `variations=3`. [observed · coder 117-16 · 2026-11-24]

## What it leaves out
- Wind and chopping are not in 117 or 118. [assumed · tech-lead · 2026-11-20]
