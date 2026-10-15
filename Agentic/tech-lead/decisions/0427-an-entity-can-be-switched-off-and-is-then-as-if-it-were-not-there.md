# 0427 — An entity can be switched off, and is then as if it were not there
date: 2026-10-15
by: tech-lead

## Decision
Every entity has an Enabled switch, on by default, shown at the top of its Inspector. Switched off, the entity
is as if it were not in the scene: it is not drawn, casts no shadow, gives no light, bounce or collision, takes
no part in hits, sound or particles, and cannot be clicked in a scene view. Its children are off with it,
whatever their own switch says; switching the parent back on brings back each child as its own switch has it.
The scene list still shows it, greyed, so it can be selected and switched back on there. The switch is saved
with the scene and holds in Play and in a shipped game. Switching it is one undo step. Built inside 089 as its
bug 01, because 089 cannot be tested without it.

## Reasoning
The human needs to take the landscape away to judge models' levels, and to flip between lights. Deleting and
undoing works today but is clumsy and one save away from losing work.
- Editor-only hide: cheaper, but Play and Ship would differ from the editor and nothing would be saved.
- Per component: finer, but two controls where one covers both uses; it can come later.
- A separate work order: it would wait behind 090–094 and leave 089 untested.

## Replaces
nothing
