# 0047. The Clang floor is 19

- **Status:** Accepted
- **Date:** 2026-08-31
- **Deciders:** Human, Tech Lead
- **Amends:** ADR-0005 (the minimum Clang version, 18 → 19)
- **Superseded by:** —

## Context

**Closes D-033.** ADR-0005 set the floor at Clang 18 and the register has carried
the objection since: **18 is "the first release that spells `-std=c23`", not "the
first release that implements the C23 features we use."** `constexpr` and
`#embed` both landed later. The row asked for the floor to be re-argued against
the features real cards actually use, and noted the principal's position that the
newest common LLVM release is always installable.

ADR-0046 makes it concrete. Embedding compiled SPIR-V into the binary uses
`#embed`, which **Clang did not implement until 19**. Under an 18 floor,
`check.cmake` step 1 passes on a machine where the engine does not compile — the
guard would confirm a version that cannot build the tree, which is worse than no
guard, because it reports success.

**Verified on the development machine, 2026-08-31.** `#embed` compiles and runs
under the engine's exact flag set — `-std=c23 -Wall -Wextra -Wpedantic -Werror`
— on clang 22.1.8, reading a binary file into a `static const unsigned char[]`
with correct size and contents.

### What the raise actually costs a contributor

Clang 19 was released in September 2024, roughly two years before this decision.
The development machine runs 22.1.8 from Fedora 44. The standalone LLVM release
recommended for Windows by `CLAUDE.md` is well past 19. **No platform this
project supports makes 19 difficult to obtain**, and ADR-0026 already requires a
specific Clang rather than whatever the system provides, so a contributor is
already installing a compiler deliberately rather than accepting a distribution
default.

## Options considered

### Option A — raise the floor to 19
Matches the language level the engine actually uses. Costs: contributors on
Clang 18 must upgrade. ADR-0005 is amended and `check.cmake`'s version guard, its
message text, and `CLAUDE.md`'s *Givens* all change together.

### Option B — keep 18 and avoid `#embed`
Generate a C source file containing the shader bytes at build time instead —
`file(READ … HEX)` in CMake, emitting an array. Portable to Clang 18, and it is
what everyone did before `#embed` existed.

Costs: a build-time code generator for something the language now does natively,
producing a large generated `.c` per shader that must be compiled. It also
directly contradicts the principal's instruction on card 008 — *"No extra scripts
for this, cmake should do all the work needed"* — in spirit, by reintroducing
generation where a language feature suffices.

### Option C — keep 18 and load shaders from disk
Avoids the question by reversing ADR-0046. Rejected there, for reasons that
decision records.

## Decision

**Option A. The floor is Clang 19.**

The deciding factor: **a floor that permits a compiler the engine cannot build
with is not a floor, it is a false negative in the one script that enforces
anything.** With no CI (ADR-0004), `check.cmake` step 1 is the only thing
standing between a contributor and a confusing failure, and it must fail on 18
rather than let 18 through to fail later at a `#embed` nobody expected to be the
problem.

The cost is genuinely small and was already the principal's stated position on
D-033: the newest common LLVM release is always installable, and ADR-0026
already has contributors installing a specific Clang rather than using a system
default.

**This closes the objection D-033 raised**, and closes it in the direction the
register predicted: the floor is now the version that implements what we use,
not the version that accepts the flag.

## Blast radius

**Reversibility: moderate.** Lowering the floor again means removing every
`#embed` and reinstating a generator, and by then there will be one per shader
stage across several folders.

The number itself is cheap to change — it is one comparison in `cmake/voe.cmake`
and one line in `CLAUDE.md`. What is not cheap is the language features written
on the assumption it holds. **Raising it again later is easy; lowering it is
not.** That asymmetry argues for raising it only when a feature demands it,
which is exactly the trigger here and the reason it was not raised earlier on
general principle.

## Consequences

- **`check.cmake`'s version guard changes, and so does its message text.** That
  text is a **contract with the check script**, matched by substring —
  `cmake/voe.cmake`'s header says so explicitly and ADR-0028 relies on it. The
  guard test asserts on "Clang 18 or newer". **Both the guard and the test that
  proves the guard fires must change together**, and `VOE_CHECK_FAKE_CLANG_VERSION`
  must be given a value that is now below the floor. Changing one and not the
  other leaves a guard that fires with different wording and a test that still
  passes for the wrong reason.
- **`CLAUDE.md`'s *Givens* table and `docs/prestudy/README.md` both say 18** and
  must say 19. So does ADR-0005, which is amended rather than edited — its text
  stands and this ADR is the amendment.
- **A contributor on Clang 18 is now turned away at configure time**, with a
  message naming the version. That is the intended behaviour and it is better
  than a `#embed` error in a generated build step.
- **`constexpr` also becomes available**, which D-033 raised alongside `#embed`.
  Nothing uses it and nothing should start on the strength of this ADR —
  ADR-0034 still applies.
- **This does not raise the CMake floor (3.28) or change any other tool.** One
  number moves.

## Rejected options and why

- **Keep 18, generate a C array (B)** — rejected. It reintroduces build-time code
  generation to work around the absence of a language feature that a two-year-old
  compiler has, on a project whose contributors already install a specific Clang
  deliberately. It also produces a large generated source per shader where
  `#embed` produces none.
- **Keep 18, shaders on disk (C)** — this is ADR-0046 reversed, and is rejected
  there on failure surface. Noted here only so it is clear the floor was not
  raised without checking whether the requirement could be avoided.
- **Raise the floor higher than 19** — considered and rejected. Nothing in the
  engine uses a feature newer than `#embed`, and a floor set above what is needed
  costs contributors for nothing. 19 is the first version that compiles what we
  write, which is the only defensible number.

## Questions this opens

None. **D-033 is closed**, and D-032 — the `slangc` version floor — remains open
and is unaffected: it is a different tool with a different release cadence, and
ADR-0046 asks the first card that invokes `slangc` to report the version it used
so that floor is set from evidence.
