# 0036. Dev programs — one per file in `<folder>/dev/`

- **Status:** Superseded by ADR-0038
- **Date:** 2026-08-30
- **Deciders:** Tech Lead (delegated by Human)
- **Supersedes:** —
- **Superseded by:** ADR-0038 — one dev project at `dev/`, not one per folder. The error recorded here was applying the fragmented, machine-facing shape of tests to a human-facing artefact.

## Context

D-037. Proposed alongside ADR-0031 and parked by the principal the same day —
correctly, because nothing needed one. It is now needed: the principal asked
when a triangle would be visible in a dev project, and there are no executables
in this repository at all. Every target is a static library. A window cannot be
opened and looked at, and neither can anything after it.

This is the implement-on-demand rule (ADR-0034) working as intended rather than
an exception to it: the feature waited until a call site existed, and the call
site arrived.

The principal's own description, from the session that parked it: *one extra
"dev" project for each library project to just see how it is working.* That is
the design; this ADR fixes the mechanism and the fence around it.

## Options considered

### Option A — `<folder>/dev/*.c`, globbed, one executable per file
Exactly the shape ADR-0031 gave tests, for the same reasons: nothing is
registered, adding one creates a file and changes nothing else, and the folder's
`CMakeLists.txt` stays four lines.

### Option B — A single `dev/` folder at the root holding programs for everything
One place to look. Costs a dependency edge from that folder to every other
folder, which is precisely the shape the module map exists to prevent, and it
separates a dev program from the code it exercises.

### Option C — Declare each one explicitly, `voe_dev_program(name DEPENDS ...)`
Visible and explicit. Costs a fifth line, an edit per program, and a shared file
to touch — the properties ADR-0031 spent a decision removing.

## Decision

**Option A.**

Deciding factor: tests and dev programs are the same problem — a small
executable exercising one folder — and having them work identically means there
is one rule to know instead of two.

1. **`<folder>/dev/*.c`**, globbed by `voe_module()` with `CONFIGURE_DEPENDS`,
   one executable per file. Target `voe_<folder>_dev_<file>`.
2. **Each links its own folder's target and nothing else** beyond what that
   folder already declares. Not `voe::testing`.
3. **Built by the ordinary build, under the same `-Werror`.** A dev program that
   stops compiling breaks the build, which is the point: an uncompiled dev
   program is worse than none, because it looks like working documentation and
   is not.
4. **Never run by `check.cmake`.** They open windows and wait for a human.
   Building them is the automated part; running them is the human part.
5. **Nothing links a dev program**, and no engine code may reach into `dev/`.
6. **They must build on both platforms**, like everything else. A dev program
   uses its folder's API, which is portable by construction — if one needs an
   `#ifdef` for an operating system, the folder's API has a hole and that is the
   finding.
7. **No engine logic lives in `dev/`.** If something there is worth keeping, it
   moves into `src/` with a test. A dev program is a call site, not a home.

**Not decided here:** how `app` and tool executables are declared. There is no
`app` yet. D-034 stays open, and `voe_executable()` is written when something
needs it — not now, on the grounds this ADR's own context describes.

## Blast radius

**Reversibility: cheap.** Deleting a `dev/` folder deletes the program. Nothing
depends on one, by rule 5.

## Consequences

- **`dev/` is where real code will try to hide.** The genuine risk, and rules 5
  and 7 are the fence. It shows up at review as a dev program that has grown
  functions rather than calls.
- **Every folder may now produce three kinds of target** — a library, tests, and
  dev programs — from four lines of CMake and two directory names. That is a
  lot of behaviour packed into `voe_module()`, and it is the reason
  `voe_module()` is the one file besides `check.cmake` that a coder should never
  need to open.
- **Dev programs are unverified by anything but a person.** Correct: their whole
  purpose is to be looked at. Nothing about them should be trusted as evidence
  in a review that a card is complete — that is what tests are for.
- **The first one will be `platform`'s**, opening a window, and for that card the
  dev program *is* the verification, because window creation has no meaningful
  unit test.

## Rejected options and why

- **Option B** — a root folder depending on everything inverts the module map to
  get a directory listing.
- **Option C** — a fifth line and a shared edit, for explicitness that the
  directory name already provides.

## Questions this opens

- **Closes D-037.**
- **D-034 stays open** and is now clearly separate: `app` is a shipped
  executable with a dependency row in the map; a dev program is a scratch call
  site. They should not be the same mechanism just because both link.
