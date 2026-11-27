# 35 — Foliage groups replace the tree maker

## Blocked
The sponsor withdrew the tree maker on 2026-11-27 (0520). 117's scope is now foliage groups; `feature.md`
is rewritten and its `## How to test` is the contract.

Replan the rest of 117 from `feature.md` and 0520:
- Card 34 (branches grow out of the trunk) must not be built: no tree code is built from here on.
- Cards 30 and 31 (removing a kind is one undo step, a cross on each kind) are rewritten for groups, and
  for removing one glb from a group.
- The tree-only code from cards 01–24 and 33 comes out, and the shared code that leans on it (model store,
  foliage draw, cooks, check, Docs, disk watch, Assets, Inspector, undo, session) goes back to what groups
  need. The seed, kind removal and the folds that follow it (cards 25–29) stay and become the group's.
- The variation pick from card 05 is what a group's pick becomes, weighted by each glb's share.
