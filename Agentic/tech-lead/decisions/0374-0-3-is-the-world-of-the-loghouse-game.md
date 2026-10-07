# 0374 — 0.3 is the world of the loghouse game
date: 2026-10-07
by: tech-lead

## Decision
0.3 is chosen by a game again, as 0267 has it. The game: a third-person player on one hill (after the
radio hill in Days Gone, without the enemy base) saws down trees and breaks stone to build a loghouse
on a concrete base, and a lookout tower to see the valley from. The house goes up in fixed stages as
materials are brought, not by free building. The game is reached over several releases. 0.3 is its
world without the player: the hill as a landscape whose ground blends materials (dirt, grass, moss,
rock), grass growing on the grassy parts, trees with LOD levels, a sky, and the house and tower as the
sponsor's own Blender models. It is flown through with a free camera in the editor and as a shipped
build. Materials are instances of shaders (0189), and the editor edits them at least far enough to make
the ground's layers. 0.4 is the player, a skinned and animated third-person character, as 0368 and 0373
already kept animation for 0.4. Chopping, mining and building come after that. Multiplayer comes after
all of these, in a release of its own if at all. 0.3's road (its milestones and work orders) is its own
decision, still to come. That decision also settles whether the hill is shaped and painted in Blender or
with brushes in the editor, and it keeps, rewrites or withdraws each of work orders 067–080, which wait
until then. Khronos sample models may serve as test models but never set a goal.

## Reasoning
The sponsor's call (2026-10-07): "I focused on feature instead of a game i want to play or make." 0368
and 0372 chose 0.3 first by a scene and then by a test suite. That is the feature-driven road 0267
rejected, and half of 0373's road (074–080) served nothing the sponsor wants to make. This is the
sponsor's road to learning to make games, not a race ("Its not meant to be fast"). The world comes
first because the view from the tower is the game's finish and every later step stands on it.
- Keep 0373: a conformance suite judged by eye against a reference renderer, at odds with 0316.
- The whole game in 0.3: character, building and world at once is too big for one release.
- Chopping and building on flat ground first: the sponsor chose the world first.
- Multiplayer in 0.3: it touches everything and would slow every card.

## Replaces
0372 and 0373. Their amendments to 0270, 0278, 0328 and 0359 were never built and fall with them.
Restores 0267's rule that a release is chosen by a game, which 0368 had amended.
