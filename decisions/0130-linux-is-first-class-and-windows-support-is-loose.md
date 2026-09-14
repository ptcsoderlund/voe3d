# 0130. Linux is first-class and Windows support is loose

- **Status:** Accepted
- **Date:** 2026-09-11
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

`STATUS.md`'s first line has always read *"C23, Vulkan, Windows and Linux desktop"*, and
ADR-0022 confines every OS difference to `platform` so both are written once. What was
never decided is whether both are **verified** equally, and in practice they are not: no
agent can run on the principal's Windows machine, so every card is verified on Linux and
its Windows half closes by hand, by him, whenever he is next at that machine.

The tech lead raised this as an outstanding obligation on 2026-09-11 and pressed it twice.
The principal settled it: *"Linux is our first class citizen, linux first … We dont care
about windows, we will get bug reports when usage starts. We keep it casual until then …
We keep windows support loosely even though we implement platform specific stuff from time
to time. **Not negotiatable.**"*

He also corrected the premise the tech lead had been pressing on: **bug 002 was not a bug.**
The blank element surface on Windows was a mismatch between `slangc` versions, recorded in
the commit that closed it, and the reports for bugs 001 and 002 were removed from the board
in the same period.

## Options considered

### Option A — verified parity
No card closes until it is checked on both platforms. Correct on paper, and it makes the
principal a blocking dependency on every card.

### Option B — Linux first, Windows loose
Linux is the platform work is verified on. Windows is supported and written for, and its
verification is best-effort, driven by actual use rather than by obligation.

### Option C — drop Windows
Not proposed by anyone and recorded only to be refused.

## Decision

**Option B**, the principal's call and not negotiable.

1. **Linux is the platform a card is verified on.** A card is done when it passes on Linux.
   Nothing waits for a Windows run.
2. **Windows remains supported, and platform-specific work continues for it.** No Windows
   code is removed, no Windows path is left unwritten, and `platform` keeps both backends
   as equals in the source.
3. **Windows verification is driven by use, not obligation.** The principal is on Windows
   from time to time and runs the check then. What that surfaces becomes a bug report in the
   ordinary way.
4. **A Windows problem is a bug report, not a standing debt.** ADR-0108's loop already
   handles it and ADR-0109 already says a report from a real driver is worth more than one
   from ours. Nothing new is needed.
5. **This does not weaken the build promise.** Clone, install Clang, CMake and Slang, build
   — on either platform. That is about the *build*, and it is unaffected.

## Blast radius

**Cheap to reverse, gradual to pay for.** Reversing is a sentence. What accrues meanwhile is
drift: the longer Windows goes unexercised, the larger the eventual catch-up. That is the
cost being bought deliberately, in exchange for not making one person a blocking dependency
on every card in the project.

## Consequences

- **`STATUS.md` should stop carrying *every Linux-verified card owes its Windows check* as
  an open obligation.** It is not one any more, and leaving it there describes a debt nobody
  intends to pay.
- **A Windows regression may live for a while**, and it will be found by the principal using
  the engine rather than by a check. That is the accepted trade.
- **The cook is per-platform anyway** (ADR-0128), so Windows output is produced regardless
  of whether anyone has verified it recently.
- **Nothing about this is a judgement on Windows users.** It is a statement about where the
  one available tester's time goes, and it changes when there is more than one.

## Rejected options and why

**A — verified parity.** Rejected by the principal. Worth recording what it actually cost:
it made every card's completion wait on one person being at one machine, in a project where
that person is also the only decider. The obligation was real and unpayable, which is the
worst kind.

**C — drop Windows.** Never proposed. Recorded because the decision above could be
misread as it, and it is not: Windows is supported, written for, and shipped.

## Questions this opens

None.
