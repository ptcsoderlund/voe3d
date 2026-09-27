# 0272 — The tank game grows milestone by milestone
date: 2026-09-27
by: tech-lead

## Decision
`examples/tank_game/` is made in milestone 3, the first time the sponsor imports their models. From
then on, each 0.2 engine work order is tested in that project, and it adds the game code needed to
show its feature: the tank drives and turns its turret, shells fly and hit, dust rises, and so on.
The game code is written as the game will keep it, not as throwaway test code. 0251's rules apply:
the sponsor builds the levels, agents write `Code/` and never edit the sponsor's scenes, and a card
may keep a test scene of its own. Milestone 14 is then what is left: the real level, enemy waves,
winning and losing, the menus, and shipping.

## Reasoning
Every engine feature needs something to test it in, and the sponsor wants to test in the game they
are making. Code written only for testing would be thrown away. Alternatives: a separate proving
ground project (twice the upkeep, and it still has to become a game); all game code at the end
(nothing to test the engine work in until then, and problems found late).

## Replaces
nothing. It amends 0268 milestone 14.
