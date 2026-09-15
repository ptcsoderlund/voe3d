# 0055. Fast rebuild is the iteration strategy; five seconds is the budget

- **Status:** Accepted
- **Date:** 2026-09-01
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

The principal's point, stated 2026-09-01: *"We have to remember that we are
developing in C. We have fast iteration in our advantage, we should design
around it. Hot reload might not be necessary when we have fast reload."*

This names the premise three existing decisions already rely on without saying
so:

- **ADR-0008** — no plugin boundary, no runtime code loading. Its stated cost is
  that hot reload becomes expensive later.
- **ADR-0046** — shaders embedded in the binary. Its stated cost is that a
  shader change requires a rebuild.
- **ADR-0052** — Play is a debug build. Its cost is a compile and a link per
  Play.

Each of those is cheap *only* if rebuilding is fast. Three separate acts of
restraint turn out to be one bet, and the bet has never been written down — which
means it gets re-argued from scratch every time someone wants hot reload, and
nobody can say whether it is still holding.

C is what makes the bet plausible. No templates, no header-only libraries, and
ADR-0005's single compiler; the pathologies that make a C++ engine's debug build
take minutes are largely absent by construction.

## Options considered

### Option A — the strategy, with a number
Declare fast rebuild the iteration mechanism and attach a budget: a one-file
change reaches a running program within N seconds. Exceeding it is a defect
against the build.

Costs: the number must be measured, so something has to measure it. A budget
nobody checks is worse than no budget, because it sounds like a guarantee.

### Option B — the principle, without a number
Record the preference for fast rebuild over hot reload and leave it qualitative.

Costs: "fast" is unfalsifiable. Build time degrades a second at a time, every
increment individually defensible, and there is no moment at which anyone is
obliged to notice.

### Option C — pursue hot reload
Treat instant iteration as the goal and pay for it with the mechanisms ADR-0008
declined.

Costs: reopens ADR-0008 and ADR-0046 to buy a capability whose absence has not
yet cost anything measurable.

## Decision

**Option A, with N = 5 seconds.** The deciding factor is that this question is
settleable by measurement and unsettleable by argument, so it gets a number
rather than another round of taste.

**The rule:** a one-file source change reaches a running program in **under five
seconds**. Exceeding it is a bug against the build — not an argument for hot
reload.

**Hot reload is therefore not pursued**, and "reload shaders and assets without
restarting" stays on the *later* list rather than moving up. The condition under
which it becomes a real proposal is now explicit: the budget being unmeetable,
demonstrated by measurement.

**Scope: the C engine and the game.** If editor scripting arrives with its own
iteration mechanism — assembly reload, or an interpreter — that is that
decision's business and this budget does not govern it.

## Blast radius

**Cheap to reverse, and it is a policy rather than a structure.** Abandoning it
costs nothing already built. Its value is entirely in being checkable, so the
thing that would quietly destroy it is not a reversal but a budget that is never
measured.

## Consequences

- **Link time is the term that grows**, and it is the one to measure. Compiling
  one C translation unit stays fast almost regardless of project size; linking
  does not. Expect the budget to be spent on the link long before it is spent on
  the compile.
- **The shader embed is a known tax.** ADR-0046 ties a folder's objects to its
  shader binaries through a folder-wide dependency, so editing one shader
  rebuilds more than strictly necessary. ADR-0053's sideload path takes shader
  iteration off the rebuild path during authoring, which is now a second reason
  that decision earns its place.
- **This decides the parked Play question later, on data.** Whether Play runs
  in-process or as a child process (ADR-0052) turns on how many seconds
  in-process actually saves. If the budget holds, the child process wins and
  ADR-0008 is never reopened.
- **Something must measure it.** A number with no measurement is the failure
  mode of this ADR, so a measurement is a required follow-up, not an optional
  one. Where it lives — a check step, a dev target, or a card's acceptance
  criterion — is a new question.
- **Choices that trade build time for run time now have a stated ceiling.**
  Whole-program optimisation, unity builds and link-time optimisation are release
  concerns and must not reach the debug configuration that Play uses.
- **Five seconds is a ceiling, not a target.** Today a one-file change should be
  well under it. The budget exists to catch drift, and the interesting number is
  the trend, not the pass/fail.

## Rejected options and why

- **Option B** — rejected because it is what the project already had implicitly,
  and it is precisely why the shader question read as a contradiction for a
  session: an unstated premise cannot be checked against, so decisions that
  depend on it look arbitrary. A number is what turns a preference into an
  invariant.
- **Option C** — rejected. It is a real answer, and it is the one every
  commercial engine reached, but each of them arrived there from a language where
  the budget was already lost. Buying the mechanism before the budget fails is
  paying for a cure without the disease.

## Questions this opens

- **Where the rebuild time is measured, and against what change.** "A one-file
  change" needs a specific file for the measurement to be comparable over time —
  a leaf `.c` in `base` and a shader are the two interesting cases.
- **Whether the check script gains a timing step**, or whether this is a dev
  target run by hand. The check script is CMake script mode (ADR-0028), which may
  or may not be the right place to time a build.
