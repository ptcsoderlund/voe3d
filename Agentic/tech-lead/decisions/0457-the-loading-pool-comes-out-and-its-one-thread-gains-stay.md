# 0457 — The loading pool comes out, and its one-thread gains stay
date: 2026-10-22
by: tech-lead

## Decision
On 2026-10-21 two machines worked 090 apart. The evening's line never saw 0456, which the afternoon's line
had written, and built 0448's worker pool for 090 bug 02. The sponsor's ruling: where the two disagree, the
afternoon's decisions stand. So:

1. **0448 is withdrawn, except three of its gains.** No pool, no core count, no task reporting across
   threads, no loader thread beside the frames: loading runs on one thread as 0456 says, and a load while
   editing runs between frames as it did before 0448, until 092's cooked copy takes that work away (0454
   point 3). What stays, each being single-threaded: the CPU edges weld through a hash of cells, linear
   (point 6); a `.glb` read with its pictures left encoded, so Import and the levels make decode no picture
   they do not need; and the `game: models:` / `game: materials:` line of each step's wall time (point 4),
   with steps that no longer exist left out of it.
2. **090 bug 02 is withdrawn with 0447** (0456). Its speed goal moves to 092.
3. **No bounce falloff.** Bug 07's ask for a falloff setting is dropped by the sponsor; 0450 stands whole,
   and bug 07's first ask is what 0450 and 0451 already carried out.

## Reasoning
The sponsor (2026-10-22): "What was said in the afternoon was better, they should have priority." On the
falloff: "I think we wait with the bounce falloff. Throw it away. I need to let go of my perfection so we can
move forward." Keeping the weld and the undecoded read keeps the measured single-thread wins (the weld was
about 35 % of the lumberhill start) without the pool 0456 refused. Alternatives:
- Taking out all of 0448's code, the weld included. Rejected: throws away speed that needs no threads.
- Leaving the pool in until 092 rewrites loading. Rejected: the code would contradict 0456, and planners
  build on what they find.

## Replaces
Withdraws 0448 but its points 4 and 6 and the encoded read of point 3. Confirms 0456 over 0447 and 0448.
