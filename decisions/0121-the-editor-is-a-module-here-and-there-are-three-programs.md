# 0121. The editor is a module in this repository, and there are three programs

- **Status:** Accepted
- **Date:** 2026-09-11
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

**Closes D-060**, open since ADR-0022 drew the module map: *where the editor sits — a
folder, an executable beside `app`, or its own repository.* ADR-0052 sharpened it without
solving it — *"the editor has no place in the module map. ADR-0022 names eight folders and
none of them is an editor"* — and ADR-0057 narrowed it to a C program built on the engine,
leaving only placement open.

The principal answered placement on 2026-09-11: *"we create our editor as well as a module
(using the other ones). Which means everything is compiled as is with the editor."*

He then corrected the tech lead's first reading of what happens to `dev`. That reading had
`dev` becoming the example game and then being deleted. His answer: **"we should be able to
make a game without the editor. So we keep dev for that purpose. We then do a editor
project and a dev_editor project. dev_editor is for how devs will use our editor when they
make their dream games."**

Constraints already fixed. ADR-0052: three builds — engine, editor, cooked game — and
cooking means building again. ADR-0008: no runtime code loading. ADR-0057: the editor is C
on the engine; another language is an optional bridge. ADR-0022's DAG: no upward or
sideways dependencies. ADR-0126: the cook strips editor-only components.

## Options considered

### Option A — the editor is its own repository
Keeps the engine checkout small; costs a version boundary between two things developed
together, and a second repository to keep in step with an engine that changes daily.

### Option B — a module here, and `dev` becomes the example game
The tech lead's first reading. Two programs: the editor, and `dev` repurposed as the
example game and eventually deleted.

### Option C — a module here, and three programs
`dev` stays as it is; the editor is new; a third program is the example game authored
through the editor.

## Decision

**Option C**, the principal's call, and it is better than the tech lead's Option B for a
reason worth stating plainly.

1. **The editor is a module in this repository**, built on the other modules through their
   public APIs like any other consumer. It is a leaf at the top of ADR-0022's DAG: it may
   depend on the eight folders and **no folder may depend on it**.
2. **The shipped game is built without it**, which is ADR-0052's three builds unchanged.
3. **There are three programs**, and each answers a different question:

   | Program | Answers |
   |---|---|
   | `dev` | *Can a game be made with this engine and no editor at all?* |
   | `editor` | The tool itself |
   | `dev_editor` | *What does using the editor to make a game actually look like?* |

4. **`dev` is kept, and it is now a guarantee rather than a demo.** Its value is that it
   uses no editor: as long as it builds and runs, the engine's public API stands on its own
   and a programmer who wants nothing to do with the editor is a supported user. The moment
   the engine becomes unusable without the tool, `dev` breaks and somebody finds out. That
   is a standing regression test on a promise nothing else checks.
5. **`dev_editor` is the proof the cook works**, end to end: a game authored in the editor,
   cooked, and run. It should be scoped as a real deliverable, not a demo.
6. **The editor is not on the shipping line.** No folder among the eight may gain a symbol,
   a field or a branch for the editor's sake without an ADR saying so.

## Blast radius

Moderate. Moving the editor to its own repository later is a directory move plus a
submodule and no code changes. What is expensive to reverse is point 1's direction: if any
of the eight folders ever depends on the editor, the DAG is gone and the shipped build
stops being separable from the authoring tool, which is what ADR-0052 exists to prevent.
Reversibility: **cheap for placement, load-bearing for the direction of the dependency.**

## Consequences

- **`app` is now properly forced.** D-081 has waited for a second program; there are three,
  and the frame loop currently living in `dev/src/main.c` would otherwise be written three
  times and could be written three different ways. It moves onto the path.
- **Three programs is three programs to keep working**, and `dev` stops being a throwaway.
  It has been carrying things that belong in folders that do not exist yet; as a permanent
  deliverable that stops being acceptable and those things have to move.
- **The cost the tech lead's Option B would have incurred is now visible.** Folding `dev`
  into the example game would have deleted the only check that the engine is usable without
  the editor, at exactly the moment the editor started pulling on every folder. That is the
  kind of guarantee that is never missed until it is gone.
- **The engine checkout grows an editor** whether a user wants one or not. D-109 already
  parks whether the engine and editor eventually move to a repository of their own.

## Rejected options and why

**A — its own repository.** Premature: it is a directory move at any later date, and until
the editor exists there is nothing to separate. D-109 holds the question.

**B — `dev` becomes the example game.** Rejected by the principal, and correctly. It
conflated two different questions — *can this be done without the editor* and *what does
doing it with the editor look like* — into one program that could only answer the second.
The tech lead proposed it on the grounds that a throwaway should not become permanent, and
missed that what makes `dev` worth keeping is precisely the thing that made it look
disposable: it uses none of the tool.

## Questions this opens

- **D-231** — what `dev_editor` must contain to prove the cook, and what its acceptance is.
