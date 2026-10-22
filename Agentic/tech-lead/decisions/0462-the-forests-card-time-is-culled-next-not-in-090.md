# 0462 — The forest's card time is culled next, not in 090
date: 2026-10-22
by: tech-lead

## Decision
For 090 bug 12. 090 accepts the forest's card time as it stands: `view target 1: foliage` at 40–104 ms
and a card mean near 30 ms, with every copy drawn at its level, behind the camera too. Bug 12's
foliage part is closed by this decision and gets no 090 card. Culling, what is off screen is not drawn,
moves up to come right after 090. The queue is renumbered: 091 culling (was 095), 092 import size (was
091), 093 cooked copy (was 092), 094 grass (was 093), 095 compressed textures (was 094); 096 the hill
shipped, 097 snap and 098 docking panels keep their numbers. Old numbers in earlier decisions and in
090's bug reports mean these. The rest of bug 12 is planned in 090 as it stands: the watch times its
own frame from `_begin` to `_end`, `pace: wait` names the 0215 wait, the bounce's CPU work is timed by
its parts and the stalled part fixed, and the frame line says whether a light bounces. Once culling is
in, the foliage swings are measured again.

## Reasoning
The freeze is the CPU side, the bounce stall and the frame that does not add up, and that is fixed
inside 090 either way. The card's 5.7 M triangles hold whatever the camera faces, so culling cuts the
steady share, which is exactly the next work order's job. The swings on top likely come from cut-out
leaf pixels near the eye, which culling does not touch, so they are measured after it rather than
guessed at now.
- Culling pulled into 090: a work order's worth of cards across render, 3d, editor and game, and the
  swings stay.
- A per-kind draw distance in 090: a new authored setting the sponsor did not ask for, and culling
  still comes later.

## Replaces
Nothing. Amends 0455's queue order; answers 0461 point 5.
