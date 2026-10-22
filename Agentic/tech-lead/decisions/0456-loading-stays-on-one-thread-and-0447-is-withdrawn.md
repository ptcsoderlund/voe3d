# 0456 — Loading stays on one thread, and 0447 is withdrawn
date: 2026-10-20
by: tech-lead

## Decision
0447 is withdrawn: models are not loaded on a pool of workers. Import, cooking and every load keep running as
they do now, on one thread. Today's decisions come first: the import size (0453, work order 091) and the
cooked copy (0454, work order 092), which the editor reads when it starts as well as Play and the shipped
game. 090's bug 02, if it turns up, is withdrawn with 0447. Loading on every core goes to ideas.md and comes
back only if loads are still too slow once the cooked copy exists.

## Reasoning
The sponsor (2026-10-20): "Doing multithreaded model loading is not a priority. If editor also uses cache
when firing up, i think we are good to go a long while before doing optimizations like this." A cooked copy
removes the work from every load after the first, so threads would only speed up the first cook after an
import or a deleted `Cache/`. That is not worth a worker pool and render's upload guard now (0358: measure
first).

## Replaces
Supersedes 0447. Amends 0454 point 3: cooking runs on one thread.
