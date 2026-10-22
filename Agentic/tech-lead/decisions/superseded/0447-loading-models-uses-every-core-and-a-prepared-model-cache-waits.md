# 0447 — Loading models uses every core, and a prepared-model cache waits
date: 2026-10-19
by: tech-lead

## Decision
For 090 bug 02: the model loader is made fast in itself. Importing a model, starting the editor, loading a
scene and the game's own start all load through it, and it may use every core of the machine: a pool of
worker threads that reads, parses, decodes pictures, bakes parts and makes LODs side by side, splitting the
work by file, picture and part wherever it splits. Only the hand-over to the card stays on one thread,
behind render's guard (0370). The load says how long each of its steps took (reading, parsing, decoding
pictures, baking, making LODs, uploading), so the slow step is fixed, not guessed. 0386 still holds: a
load during editing runs beside the frames and never hitches them. There is no on-disk cache of prepared
models now. Play staying a separate program that loads again (0187) is accepted for now. A cache comes
when the loads that repeat are what is slow, by a decision of its own.

## Reasoning
The sponsor: the import is what is slow, not Play; first build a better loader, cache later when we have
to. Measured on lumberhill (a 21 MB and a 108 MB `.glb`, 4k pictures) on a 16-core machine: the release
editor spends 9.6–9.8 s of a 10.0 s start in "scene and shapes", using about two cores (debug: 29.8 s).
Alternatives: a prepared model in `Cache/` (skips repeat loads only, the first import stays slow), Play
inside the editor (reverses 0187 and 0375), the game borrowing the editor's video memory (complex, per
platform), one loading worker as today (leaves 14 cores idle).

## Replaces
Amends 0362 ("no other threading") and 0370 point 1 (one worker thread) for loading only, as 0386 left
open. Answers bug 02's needs-decision: none of its options now.
