# 0506 — Outside changes ask per file, and agents read Docs beside Assets
date: 2026-11-06
by: tech-lead

## Decision
How 0505 is carried out. (1) When a sectioned-text file of the open project changes on disk and the editor
holds no unsaved change to that file, the editor takes the new file in by itself, as one undo step. When it
does hold unsaved changes to that file, it asks, for that file alone: load from disk, or keep mine. Several
changed files ask one by one; the editor's own saves never ask. (2) A file that changed on disk and is wrong
(an unknown section or key, a value of the wrong type or outside its range) is not taken in: the editor keeps
its last good state and says which file, which line, which key and why. (3) The reference for agents lives in
a `Docs/` folder beside `Assets/` in every project. The editor writes it and keeps it true to itself; it is
made from the same tables the Inspector draws from, so a key, its type, its range, its unit and its default
read alike in both. The editor rewrites only its own files in `Docs/` and leaves anything else there alone.
It also says where the engine's public headers are, for the project's `Code/`. (4) `CLAUDE.md` and
`AGENTS.md` belong to whoever uses an agent in the project: the editor never writes, creates or names them.
(5) A command an agent can run in the project folder checks every sectioned-text file and reports each
mistake by file, line, key and reason, or that all is well. `Docs/` names it. (6) Two work orders: 115, the
editor takes in files changed on disk (points 1 and 2); 116, a project tells agents how its files are written
(points 3 to 5). Both come before the hill.

## Reasoning
The sponsor (2026-11-06): "If I have unsaved changes to a file and something edited the file on disk, I think
a question should pop up (load from disk or keep <file>?). On a per file basis." "We should do a "docs" file
next to "assets" where we put information for agents. CLAUDE.md and AGENTS.md is reserved for whoever uses
the agent when working in the project(s)." The check: the tech-lead's option A, taken. Taking a clean file in
without asking, as an undo step, and rewriting `Docs/` from the editor's own tables, are the tech-lead's,
assumed.
- Disk always wins, Undo brings mine back: surprises a person mid-edit.
- Editor always wins with a warning: the agent's work is easily lost.
- One reference beside the editor install: one copy, but an agent in the project folder may never find it.
- A pointer written into CLAUDE.md: takes a file the user owns.
- Only an editor message for a bad file: the agent never learns of its mistake.

## Replaces
Nothing.
