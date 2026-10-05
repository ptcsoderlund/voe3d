# 0361 — A directional light inside a blocker lights and shadows its inside
date: 2026-10-05
by: planner

## Decision
For 058 bug 02. A directional light whose place a blocker holds, of any Block kind, is active within it,
whether it casts shadows or not.
1. Its fill reaches a point when the point's Room bits are the light's (as today) and every Fill box holding
   the point also holds the light. A light inside a Fill box fills the inside of that box, and the outside as
   before. Amends 0348 point 2, "behaves as one outside", for the fill only; direct light is unchanged.
2. Its shadows are only of casters held by every blocker holding its place: a caster's place is its world
   matrix's origin, masked over the frame's blockers as a light's place is. This holds for its cascades and
   for its layer of the relight's sun map. A light no blocker holds casts every caster, as today. Bounce
   captures and the point-shadow pass draw every caster, as today.

## Reasoning
- The inside went black when the light cast because the world outside the blocker, a cave's rock or a roof,
  stood between it and the inside in its shadow map. A light that lights only its blocker must not be
  shadowed by what is outside it.
- By the caster's origin and not its bounds: one mask a caster, the twin the lights already use; a caster
  straddling a blocker is the scene's to split.
- Fill inside a Fill box from its own light: the inside was the one place it never filled, the inverse of
  what the human expects of a light placed there.

## Replaces
Amends 0348 point 2 (a directional light inside an Indoors box fills it) and 0357 point 3 (who a slotted
light's cascades draw).
