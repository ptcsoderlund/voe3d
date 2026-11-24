# 0516 — 117 lays sprigs flat, and a landscape drops a kind
date: 2026-11-24
by: tech-lead

## Decision
Two things join feature 117, and 117 is not accepted without them. First, a species has two more leaf
settings, each with a range, a default, the lenient and strict reads and a `Docs/` row as every `.tree` key
(0515, 0511). **Leaf roll**, 0–1: 0 is today's random roll of a card about its twig; 1 lays every card flat in
the twig's horizontal plane, the plane holding the twig and the level direction across it, so spruce sprigs
lie in layers along a branch. What a twig with no level direction across it (near vertical) does is the
planner's. **Leaf width**, the card's width as a share of its length: `leaf_size` stays the card's length along
the twig and this sets its width; the range is the planner's and must allow cards both narrower and wider than
long. The leaf material's picture is stretched over the card; the sponsor draws the sprig at the card's ratio.
The defaults are roll 0 and width 1, so a species file without the keys grows exactly as before. Far levels
keep a card's ratio when they scale it, so a swap does not change the crown's shape (0513, 0514). Second, a
foliage kind can be removed from a landscape in the Inspector's Foliage section without deleting the
landscape: its painted copies, its look and every layer's growth of it go, the kinds after it move down a
place keeping their copies, looks, growth and folds, and one undo step brings the kind back with every copy
where it was. The Delete key keeps deleting the selected entity. Amends 0514's "one rolled quad" and 0515's
key list.

## Reasoning
The sponsor is making the hill's spruce with a small branch on each card; cards rolled at random about the
twig read as broadleaf, not spruce. Per-species settings rather than a conifer switch: any species tunes them,
pine or fir later need no new code. One needle per card was ruled out: hundreds of thousands of cards a tree.
Removal undoable, not confirmed: one misclick should not cost an afternoon's painting, and the rest of the
editor undoes. Inside 117, not new orders after it: the sponsor's call, the spruce is 117's own first test.

## Replaces
nothing
