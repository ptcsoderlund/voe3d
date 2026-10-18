# 0445 — An import error names its file and part as the report's subject, and is said once
date: 2026-10-18
by: planner

## Decision
How 0443 is carried out, for 090 bug 01.

1. **The subject.** `base/report.h` keeps a short stack of subjects per thread, pushed and popped by
   the code that knows them: whoever opens a file by path pushes its project-relative path; the glTF
   reader pushes "the part `name`", "the material `name`" or "the picture `name`" while it reads one.
   A report prints `<level>: <module>: <subject>: …: <message>`, and the kept first error carries the
   same subjects before the message. Code below a subject never names the path again.
2. **The cause, once, where it is found.** The step that meets the cause reports it, in the artist's
   words, with the numbers, then what to do in Blender where there is something to do. The format's
   own words follow in parentheses, `(glTF: …)`, `(PNG: …)`, `(JPEG: …)`, `(WAV: …)`, for a
   developer. A broken file says it is damaged and to export it again.
3. **No restating.** A layer above a reported cause writes no line about it ("could not load the
   model", "its levels were not made"). A layer whose step failed with no line below writes the
   cause itself, once.
4. **One pass per cause.** On one file at one stamp a refusal ends the passes after it: Import's read
   refusing the `.glb` ends the import before its LODs question, the levels and the reload; a levels
   make refusing the `.glb` keeps the store's entry failed at that stamp without a second read; a
   prepare that refused is not followed by a reload.
5. **Limits and memory.** A limit the engine keeps reads "the most the engine takes is N" with the
   file's number beside it. Running out reads "the graphics card has run out of memory" (or the
   computer), with what was asked for and what was held.
6. **The notice is the kept line.** It already names the file; an editor notice adds the file only
   when the kept line does not begin with it, and shows the kept line whole, not its last phrase.

## Reasoning
The reader of a `.glb` is handed bytes and cannot name the file; the code that opened it can, and a
subject lets the cause's own line name both file and part without passing a path through every
signature in `assets`, `3d` and `render`. Saying the cause where it is found, and nothing above it,
is what makes one cause one line; ending the passes after a refusal is what stops the same line once
per pass. Alternatives: return a reason text from every reader (a signature change in every caller
of every reader, and a second way to report beside `report.h`); drop repeated identical lines in
`base` (needs a lock across threads, and hides the passes that should not run).

## Replaces
nothing. Carries out 0443 and 0444's out-of-memory report.
