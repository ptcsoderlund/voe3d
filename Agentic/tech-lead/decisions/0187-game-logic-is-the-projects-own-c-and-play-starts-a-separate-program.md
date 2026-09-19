# 0187 — Game logic is the project's own C, written outside the editor, and Play starts a separate program
date: 2026-09-19
by: tech-lead

## Decision
A game's logic is **C source files in the project folder**, compiled together with the engine
into the game program. The editor **has no code editor and never will**. Developers write the
code in the editor of their choice, and the editor only builds it and runs it. **Play** cooks the
scene (ADR-0144), builds the project's game (a debug build, ADR-0052) and **starts it as a
separate program in its own window**. The editor stays open and usable while the game runs.
Closing the game returns you to the editor with nothing changed. A build that fails shows the
compiler's errors in the editor and starts nothing. Nothing is loaded into a running process: to
change the code you stop and press Play again, and ADR-0055's five-second budget is what keeps
that cheap. **Visual logic** (node graphs) comes later as an opt-in editor plugin, off by
default, that produces what hand-written C would. It is not on the road (0186).

## Reasoning
This is the smallest step from what exists. It keeps ADR-0008 (no runtime code loading),
ADR-0052 (Play is a build), ADR-0144 (the cook emits C) and ADR-0151 (a game does not build
`authoring`), and it uses the same mechanism as the plugins idea, where the editor writes the
build file. Alternatives rejected: an embedded scripting language, which is a second language and
a hard boundary between it and C; visual logic first, which is the largest thing to build before
any game runs; Play inside the editor's window, which needs code loaded into the editor's own
process.

## Replaces
nothing
