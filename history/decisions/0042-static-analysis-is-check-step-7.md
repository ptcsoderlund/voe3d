# 0042. `clang --analyze` is `check.cmake` step 7

- **Status:** Accepted
- **Date:** 2026-08-30
- **Deciders:** Human, Tech Lead
- **Amends:** ADR-0028 (adds step 7 to the check script)
- **Superseded by:** —

## Context

ADR-0028 fixed six steps for `check.cmake`, and ADR-0004 removed CI, which makes
that script the only thing enforcing anything on this project. Every target
already compiles `-Wall -Wextra -Wpedantic -Werror` (ADR-0027), including tests
— *"a test compiled with weaker flags than the code it tests is a test that
lies."*

That is the first half of *Power of Ten* rule 10. The second half — *all code
must be checked with at least one state-of-the-art static source code
analyzer* — is absent, and the two halves catch different bugs. Holzmann's
argument is that warnings catch mistakes near the syntax and analyzers catch
mistakes in the reasoning.

**Demonstrated on this machine rather than asserted.** Given a four-line
function that leaks a `malloc` and indexes it with an unchecked parameter:

```
$ clang -std=c23 -Wall -Wextra -Wpedantic -c an.c
(silent)

$ clang -std=c23 --analyze an.c
an.c:5:2: warning: Potential leak of memory pointed to by 'p' [unix.Malloc]
```

The full pedantic warning set says nothing. The analyzer finds it. That is the
gap, on the exact flags this engine already uses.

**The tool question is the whole decision here**, because ADR-0021's line —
tools are installed by the programmer, dependencies are fetched — means any new
required tool is a cost paid by every contributor forever, and the onboarding
invariant is a given that eliminates options on its own.

## Options considered

### Option A — `clang-tidy`
The usual choice, and much richer: configurable check sets, naming rules,
readability checks, and it could in principle enforce parts of
`coding_convention.md` mechanically.

**It is a separate installation.** Verified on the development machine: `clang`
22.1.8 is present and `clang-tidy` is not — on Fedora it lives in
`clang-tools-extra`, and the situation is similar on other distributions and in
the standalone LLVM release layout. Adopting it adds a fifth required tool and a
new way for a fresh clone to fail.

### Option B — `clang --analyze`
The static analyzer built into the compiler driver. Nothing to install: any
machine that can build this engine already has it, because ADR-0026 already
requires that exact `clang`. Weaker than `clang-tidy` — path-sensitive bug
finding only, no style or naming checks — but it is the half that finds the
bugs warnings miss.

### Option C — nothing; rely on warnings and review
Status quo. Costs nothing and leaves the demonstrated gap open, on a project
with no CI and no second reviewer on most cards.

## Decision

**Option B. `clang --analyze` becomes step 7 of `check.cmake`, and an analyzer
warning fails the script**, on the same terms as `-Werror`: zero warnings, no
ratchet, no baseline file.

The deciding factor: it closes the demonstrated half of rule 10 **without adding
a required tool**, which is the only ground on which a new check could have been
rejected outright.

**Zero tolerated, from the first day.** Holzmann's reasoning applies exactly:
a project that permits a nonzero warning count never returns to zero, and a
count nobody can reach stops being read. The tree is small enough today that
the cost of starting clean is nothing, and this is the last moment that is true.

**`clang-tidy` is not adopted and not forbidden.** A contributor who has it may
run it; nothing depends on it and `check.cmake` never invokes it. If it later
earns a place — most plausibly to enforce naming from `coding_convention.md`
mechanically — that is a new ADR that must argue for the fifth tool on its own
merits, against the onboarding invariant.

## Blast radius

**Reversibility: cheap.** Deleting a step from `check.cmake`. No code depends on
the analyzer, no build output changes, nothing ships differently.

The expensive direction is the one this avoids: adopting a required tool is
cheap to add and very hard to remove, because contributors' machines, the
tools list, and both platforms' setup instructions all acquire it.

## Consequences

- **`check.cmake` grows a seventh step** and gets slower. Analysis runs over
  each folder's sources; at nine small folders this is seconds, and it is the
  human's own script rather than a gate anyone waits on remotely.
- **The analyzer will disagree with us somewhere.** Its `unix.Malloc` checks
  reason about `malloc`/`free` pairs, and ADR-0032's arena deliberately never
  frees individually — a bump allocator over chained blocks is exactly the shape
  a leak checker is built to complain about. **Where it is wrong, the fix is a
  suppression at the site with a comment saying why, never disabling the check
  globally**, and never a code change made to appease a false positive.
- **It reads `base` more harshly than anything else**, because `base` is where
  the raw allocation lives. Expected, and the right place to be read harshly.
- **It is not a substitute for tests.** Step 6 stays exactly as ADR-0031 fixed
  it.
- **This is the second `check.cmake` amendment**, and the script is now the
  single point where every mechanical rule on this project is enforced. That
  concentration is deliberate — there is no CI to put it anywhere else — and it
  makes step 0 of any debugging session "read `check.cmake`".
- **A card whose analysis does not pass does not reach `review/`**, on the same
  terms as a failing test, per `CLAUDE.md` rule 8.

## Rejected options and why

- **`clang-tidy`** — rejected on the tools line, not on quality; it is the
  better analyzer. Verified absent on the development machine while `clang` was
  present, which is exactly the failure a fresh contributor would hit. The
  onboarding invariant is a given, and an option that adds a required tool has
  to beat one that adds none.
- **Nothing** — rejected against demonstrated evidence: the pedantic warning set
  in use today is silent on a leak that `--analyze` reports.
- **Warnings-not-errors, with a baseline** — rejected. A tolerated count grows
  and stops being read, and the tree is currently small enough that clean costs
  nothing.

## Questions this opens

- **D-046 (new)** — the exact analyzer invocation and checker set: default
  checkers only, or named additions; how a per-site suppression is spelled. Left
  to the card that implements step 7, which will run it against real `base` and
  `platform` code and see what it actually says. Deciding it here would be taste.
