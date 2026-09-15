# 0038. One dev project, at `dev/`

- **Status:** Accepted
- **Date:** 2026-08-30
- **Deciders:** Human, Tech Lead
- **Supersedes:** ADR-0036
- **Superseded by:** —

## Context

ADR-0036, written this morning, gave every folder its own `dev/*.c` programs,
one executable per file, on the argument that tests and dev programs are the
same problem and should work the same way.

The principal's correction: *one dev project that a human can run to see what
can be seen; test projects we leave fragmented for AI to use.*

That names the thing ADR-0036 got wrong. Tests and dev programs are not the same
problem, because **they have different audiences.**

- A **test** is read by a machine, and by an agent working one card. Fragmenting
  it is the whole point: one file per module, nothing registered anywhere, so a
  card adds a file and changes nothing else, and `ctest -R math` runs one
  folder's worth. That is ADR-0031 and it stands untouched.
- A **dev project** is run by a person who wants to see the engine. A person
  does not want twelve executables and a decision about which one to run. They
  want one thing that shows the current state of the engine.

ADR-0036 applied the machine-facing pattern to the human-facing artefact.

## Options considered

### Option A — One executable at `dev/`
A top-level folder, `voe_dev`, depending on whatever it currently needs. It is
the human's window into the engine and grows as the engine does: a window today,
a cleared frame next, a triangle after that.

### Option B — Keep ADR-0036's per-folder programs
Each folder demonstrates itself in isolation, and a folder's programs cannot go
stale relative to it.

### Option C — Both
Per-folder programs for coders, one aggregate for the principal.

## Decision

**Option A**, superseding ADR-0036 entirely.

Deciding factor: there is exactly one person who runs this, and they asked for
one thing to run.

1. **`dev/` is a top-level folder** holding one executable, `voe_dev`, from
   `dev/src/*.c`. It is not in the module map as a dependency *target* — nothing
   may depend on `dev` — and it is permitted to depend on anything, the same row
   `app` has.
2. **It is declared with `voe_executable(dev DEPENDS ...)`**, a sibling to
   `voe_module()` in `cmake/voe.cmake`, doing the same guards, the same map
   check, the same flags and the same `src/` glob, but producing an executable.
   This answers **D-034** for `app` too, when `app` arrives.
3. **Built by the ordinary build under the same `-Werror`.** A dev project that
   does not compile is worse than none, because it looks like working
   documentation.
4. **Never run by `check.cmake`.** It opens a window and waits for a person.
   Building it is the automated part.
5. **No engine logic in `dev/`.** It is a call site, not a home. Anything there
   worth keeping moves into a folder with a test. It may be rewritten or thrown
   away wholesale at any time — nothing depends on it, by rule 1.
6. **It must build on both platforms** with no operating-system `#ifdef`. If it
   needs one, the folder it is calling has a hole in its API, and that is the
   finding.
7. **Tests are untouched.** ADR-0031 stands exactly as written: fragmented, one
   per module, per folder.

## Blast radius

**Reversibility: cheap.** Nothing depends on `dev/`. Deleting it costs nothing;
splitting it back into per-folder programs is a file move.

## Consequences

- **Card 004 is in flight and creates `platform/dev/window.c`.** It lands as
  written rather than being changed under a working coder. **Card 005 moves it
  to `dev/src/main.c` and deletes `platform/dev/`** — a small, deliberate
  consolidation rather than a mid-card correction.
- **`voe_module()` should not grow `dev/` globbing.** If card 004 has already
  added it, card 005 removes it.
- **The dev project will accumulate.** As `render` arrives it will want to show
  a cleared frame, then a triangle, and the temptation is to keep every previous
  thing behind a flag. Rule 5 is the fence: it shows the current state, and old
  scaffolding is deleted rather than preserved.
- **A folder cannot be demonstrated in isolation any more.** That is Option B's
  real advantage and it is given up. In exchange there is one command to run,
  which is what the person running it asked for.
- **`dev/` sits outside the module map while depending on it.** Acceptable
  because the edge only points downward and nothing points back — the same shape
  `app` has. `voe_allowed_deps()` gains a `dev` row.

## Rejected options and why

- **Option B (ADR-0036)** — the machine-facing pattern applied to a human-facing
  artefact. Correct for tests, wrong here, and the distinction is the audience.
- **Option C (both)** — two mechanisms, twice the fence to maintain, and the
  per-folder half would rot immediately because nobody is its audience.

## Questions this opens

- **Closes D-034**, by deciding `voe_executable()` for every executable rather
  than only for `dev`. `app` uses it when `app` exists.
- **Reopens nothing.** ADR-0031 and ADR-0034 are unaffected.
