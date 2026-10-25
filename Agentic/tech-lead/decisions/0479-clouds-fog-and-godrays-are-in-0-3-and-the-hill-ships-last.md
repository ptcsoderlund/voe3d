# 0479 — Clouds, fog and godrays are in 0.3, and the hill ships last
date: 2026-10-25
by: tech-lead

## Decision
0.3's world gains clouds that cast shadows, and fog with godrays. The sponsor: "the hill is not a shipped
product until we have clouds and a proper sky", "I want godrays in 0.3 yes", and "I would like to see fog
moving irregularly between trees as well". The hill, shipped, is 0.3's last work order: "the hill-shipped
should come after all work-orders we have now". The queue is 097 models blend into the ground, 099 snap,
100 docking panels, 101 clouds (new), 102 fog and godrays (new), 103 the hill shipped (was 098). 098 is
left unused. Earlier decisions that say 098 for the hill mean 103. Earlier decisions that say 098 for snap
mean 099; the numbers from 0474 still hold.

## Reasoning
The sponsor will not call the hill a finished product without these, so shipping it before them would
mean shipping it twice. The other work orders stay ahead of it because the sponsor asked for that.
Alternatives considered:
- Renumbering every work order into a closed run: rejected, because 099 and 100 would change a third time
  for nothing.
- One work order for clouds, fog and godrays together: rejected, because it is too much to test in one
  sitting, and the clouds must exist before beams can fall through their gaps.

## Replaces
Amends 0374 (0.3's world adds clouds, fog and godrays) and 0474 (the queue's order).
