# 0117. Below the shader compiler's floor the check warns and continues

- **Status:** Accepted
- **Date:** 2026-09-11
- **Deciders:** Human, Tech Lead
- **Supersedes:** — (amends ADR-0112's consequence *a programmer below the floor gets a
  named failure from step 1*; every other clause of ADR-0112, including the floor value
  itself, stands)
- **Superseded by:** —

## Context

ADR-0112 made the shader compiler's version part of the binary: `slangc` is parsed,
floored and printed by `check.cmake` like clang and cmake already are. Its addendum fixed
the floor at **2026.13.1** — the newer of the two machines in hand, because the older,
2024.17 on the principal's Windows machine, is not known good but is the suspect in the
blank element surface. Card 045 carries the mechanism and is claimed.

ADR-0112 listed among its consequences that *a programmer below the floor gets a named
failure from step 1*, and card 045 implements that as `step_fail`. That has a cost the
ADR did not weigh: **the principal is the only Windows tester in this project, and his
machine is the one below the floor.** A check that refuses to complete on the sole machine
capable of verifying the Windows half of every card does not gate a risk, it gates the
testing.

The principal asked for a warning instead, and separately confirmed the floor stays at the
measured 2026.13.1 rather than relaxing to the year.

The history is also now settled rather than suspected: the difference between the two
compilers **was** the cause of the blank element surface, after it was chased as an engine
fault first — *"bug fixes (which wasnt bugs, slangc versions diff)"*.

Constraints already fixed: `check.cmake` is the one gate this repository has, and a check
that has never been seen to fail has not been tested (ADR-0112, card 044); tools that
transform source are installed by the programmer and never fetched by the build
(onboarding invariant, ADR-0112); the version is printed in the tools line
unconditionally, which is not in question here.

## Options considered

### Option A — Warn and continue
Step 1 prints the version, notes that it is below the floor with both numbers, and exits
zero. Nobody is ever blocked. Costs the guarantee: a build can still produce SPIR-V from
an unverified compiler, and the only thing standing between that and a repeat of the
misdiagnosis is somebody reading a line of output.

### Option B — Fail, as ADR-0112 said
The strongest guarantee, and it stops unverified bytes reaching a binary. It also stops
`cmake -P check.cmake` on the principal's machine until he installs a current `slangc`,
which is a one-time act but an unscheduled one, and it lands on the person who is least
able to be blocked.

### Option C — Fail, with an explicit override
`step_fail` by default; `-DVOE_ALLOW_OLD_SLANGC=ON` warns loudly and continues. Default
safe for anyone cloning, and the principal is never blocked once he has said so on
purpose. Costs one more knob, and a knob set once in a shell history is a knob nobody
remembers is set.

## Decision

**Option A.** The deciding factor is the principal's: the person the failure would land on
is the only Windows tester, and a gate that interrupts the one scarce testing resource
gets worked around or switched off, at which point it protects nothing.

**The tech lead recommended C and this is recorded as a recommendation not taken.** The
concern, stated once and not to be relitigated: a printed warning is close to the state
that let two years of version divergence pass unnoticed, and the misdiagnosis happened
anyway. What answers that concern is not the exit code but the *visibility*, so:

**The warning is spent on being impossible to scroll past, since it is not spent on
stopping the build.**

1. The version is printed in the tools line **always**, floor or no floor. This is the
   two-year hole and it is unconditional.
2. Below the floor, the notice names **both numbers and what it means for the binary** —
   that this build's shaders are compiled by a version nothing in the engine has been
   verified under.
3. It is **repeated as the last thing the check prints**, after every other step, not left
   in step 1 where the rest of the output scrolls it away. A check whose warning appears
   only at the top is a check whose warning is read once.
4. It stays a warning: the check exits zero and every later step runs.

The floor stays **2026.13.1**, compared as a version and not as a string.

## Blast radius

Trivial to reverse — the difference between Option A and Option B is one call in
`check.cmake`, and the floor, the parse and the printing are identical under all three. If
a shader is ever found to miscompile below the floor, this becomes Option C in a one-line
card.

Reversibility: **cheap.**

## Consequences

- `cmake -P check.cmake` keeps passing on the principal's Windows machine, and tells him
  the truth about his compiler every time he runs it.
- **A build can still be made from an unverified compiler, and that is now a known and
  accepted state rather than an invisible one.** Any Windows fault reported while the
  warning is showing is suspect until the compiler is current — and the check now says so
  on the machine, which is where it needed saying.
- The engine has one fewer hard gate than ADR-0112 intended. The remaining gates on the
  same path are unchanged: `slangc` that cannot be run, or whose version cannot be parsed,
  still fails step 1. *Old* is a warning; *unidentifiable* is not.
- Card 045 is amended rather than reissued, since it is claimed and its other four steps
  are untouched.

## Rejected options and why

- **B, fail.** Rejected by the principal. It is the right default for a project with more
  than one tester per platform, and this project has one.
- **C, fail with an override.** The tech lead's recommendation. Rejected as a knob that
  would be set once and then be indistinguishable from Option A forever — with the
  drawback that the *reason* it was set would be nowhere, whereas Option A's warning is
  printed on every run.

## Questions this opens

- Whether the warning is ever seen to change anyone's behaviour, which is the test of
  Option A and cannot be answered by argument. The condition that reopens this is a
  Windows fault reported while the warning is showing.
