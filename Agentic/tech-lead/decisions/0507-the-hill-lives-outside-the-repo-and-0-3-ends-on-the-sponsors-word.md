# 0507 — The hill lives outside the repository, and 0.3 ends on the sponsor's word
date: 2026-11-06
by: tech-lead

## Decision
The hill is the sponsor's own game project in its own folder. It is not in `examples/` and not part of
this repository. No work order finishes 0.3: it is done when the sponsor says the hill is made, with or
without agents. Work order 113, the hill shipped, becomes 117 and stays the free-fly shipped build the hill
needs, tested on the hill; it is no longer the line that closes 0.3. The road ahead is 106, 114, 115, 116,
117.

## Reasoning
The sponsor (2026-11-06): "The hill is a project outside of this scope. It's only done when i have made the
project (with or without agents). It will not be in examples or part of this repo. Which means it's only a
"we are done" flag from me." The renumbering follows 0499's: the queue reads in number order.
- Keep 113 as 0.3's finish: makes an engine feature the judge of a game only the sponsor can call done.

## Replaces
Amends 0374 and 0479 on what ends 0.3 (the sponsor's word, not the shipped build); 0499's numbering of
the hill as 113.
