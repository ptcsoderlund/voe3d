# 0386 — Smooth frames come before throughput
date: 2026-10-08
by: tech-lead

## Decision
The product's feel is judged by its steadiest frame, not its average one. Work that would make a frame miss its
time — loading, decoding, uploading, rebuilding meshes or terrain chunks, building pipelines, recomputing light,
growing grass or trees, anything that scales with the world rather than the view — is spread across frames under
a fixed per-frame budget, done ahead of need, or done once while the splash is up (0362), never all at once in
one frame while the program runs. When a design can choose between finishing sooner with a hitch and finishing
later with steady frames, it takes the steady frames. A visible hitch in the editor or a game is a bug against
the feature that caused it. The frame breakdown (0358) is how a hitch is shown and argued. Spreading work over
more threads than the splash worker is still its own decision (0362, 0370); this one asks only that work be cut
into pieces and paced.

## Reasoning
The sponsor wants a smooth experience above all, and the 0.3 world (terrain, forest, grass, streaming to the
horizon) is exactly the kind of work that hitches when done in one go.
- Leave pacing to each feature: hitches arrive one feature at a time and are found late.
- Add a job system now: wider than this asks, and 0362 keeps threading a separate choice.

## Replaces
Nothing.
