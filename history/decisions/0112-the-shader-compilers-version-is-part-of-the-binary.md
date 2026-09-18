# 0112. The shader compiler's version is part of the binary, and the check names it

- **Status:** Accepted
- **Date:** 2026-09-10
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —
- **Closes:** nothing. Opens D-214. **Motivated by bug 002, and deliberately not justified by it** — the same discipline ADR-0111 was written under, and this time the ground is firmer.

## Context

Every `.slang` file in this project is compiled by `slangc` **on the machine doing
the build**, and the SPIR-V it emits is `#embed`ded into the binary
(`cmake/voe.cmake:331`). Nothing is compiled ahead of time, nothing is checked in,
and nothing is fetched.

**So the Windows binary and the Linux binary do not contain the same shader bytes.**
They contain the output of two different installations of a source-transforming
tool, and until now the project has never said that out loud, never compared the
two, and had no way to.

`check.cmake` step 1 is where every required tool is proved. It parses clang's
version, floors it at 19 and prints it. It parses cmake's version, floors it at 3.28
and prints it. For `slangc` it does this:

```cmake
run_capture(code text slangc -v)
if(NOT code MATCHES "^[0-9]+$")
    step_fail("${step}" "slangc could not be run: ${code}")
endif()
```

`text` is captured and thrown away. There is no floor. There is no version in the
tools line — it reads `clang 22, cmake 3.31, slangc`, with the one tool that
produces machine code for the GPU named without a number. And the exit code is
only checked for *being* a number: a `slangc` that runs and exits 1 passes step 1.
`wayland-scanner` is checked the same way.

**What that costs came due in bug 002.** The report contains a section headed *two
things that would have explained it are checked and clean*, resting on a
disassembly of what `slangc` emits for the element vertex shader — and that
disassembly was necessarily produced **on Linux, by the Linux `slangc`**, about a
fault that only exists on Windows. It is good evidence about the wrong binary, and
nothing in the repository made that visible to the person who wrote it, to the
person who reviewed it, or to me.

## Decision

**A tool that transforms source into something the binary contains is named with
its version by `check.cmake`, and floored.** clang and cmake already are. `slangc`
now is, and so is `wayland-scanner`; every one of them also has its exit code
checked properly.

The floor is set from the versions the two machines in hand actually report — the
older of the two, where both are known good — and is raised deliberately, in a
card, when the engine starts depending on newer behaviour. **A floor nobody can
justify is not invented**; the immediately binding half of this decision is that
the number is *recorded*, in the line the principal and every bug report already
quote.

**This decides nothing about `SV_InstanceID`, `SV_StartInstanceLocation`, or what
is actually wrong with the element draws.** That is bug 002's business and it waits
on a measurement. This ADR is right whichever way that goes.

## Why this and not the alternatives

**Ship the compiled SPIR-V in the repository** — compile once, check the result in,
and the two platforms provably run the same bytes. It removes this whole class of
divergence rather than merely naming it. **Rejected**: shaders stop being source,
every shader change becomes a two-step commit with a build artefact in it, the diff
of a `.slang` no longer tells you what the GPU runs, and `slangc` quietly leaves
the required-tools list while remaining required by anyone who edits a shader. It
is the right answer for a shipping product with a release pipeline and the wrong
one for a repository whose promise is *clone it and build*.

**Fetch a pinned `slangc` in the build.** Removes the divergence and the floor at
once. **Rejected on the onboarding invariant**, which is not negotiable here:
*tools that transform source are installed by the programmer; anything the engine
links against or ships is fetched by the build.* `slangc` is the first kind. An
option that breaks that line is rejected on that ground alone, and this one does.

**Leave it and compare versions by hand when a fault smells like a compiler.**
**Rejected because that is exactly what just failed.** Nobody compares two numbers
neither machine prints, and the cost of not comparing them was a day of a coder's
time spent on the near plane.

## Consequences

- **A bug report can now say which shader compiler produced the binary**, alongside
  the driver and clang versions it already carries. That is the line that would
  have made this a one-minute question.
- **A programmer below the floor gets a named failure** from step 1 instead of a
  shader that compiles and misbehaves. That is ADR-0106's rule — a skipped check
  names what went unchecked — applied to the one tool that had escaped it.
- **The floor has to be maintained**, and a floor set from two machines is a floor
  set from a sample of two. It will be wrong for somebody eventually; the failure
  mode is a named refusal to build, which is the good one.
- **The divergence itself is not removed, only made visible.** Two conforming
  `slangc` installations of different versions can still emit different SPIR-V from
  the same source, and this project will still find out about it from the
  principal's screen. What changes is that the version is in front of whoever
  reads the report. **D-214** records the unremoved half.
- **Slang's own semantics are now a versioned dependency we have written down**,
  which we had been treating as a constant. Anything the engine relies on Slang to
  lower a particular way is a thing the floor is protecting.

## Measured the same day — the floor is 2026.13.1

This ADR left the floor to measurement. The measurement came in within the hour and
is recorded here rather than only in card 045, because a floor whose reason lives in
a card is a number somebody will later "tidy".

| | `slangc` |
|---|---|
| The Linux workstation every shader has been verified on | **2026.13.1**-1-g84792eb15 |
| The principal's Windows machine | **2024.17**-1-g839bc9aa |

**Two years apart, on the one tool whose output reaches the GPU untranslated.** The
Linux number existed in exactly one place — card 009's notes, written when `slangc`
was first run — and had not been recorded since. That the comparison was possible at
all today is luck, and it is the argument for this ADR made better than the ADR made
it.

**The floor is the *newer* of the two, which reverses what the decision above said,
and the reason is that its condition is not met.** The rule was *the older of the
two, where both are known good.* **2024.17 is not known good**: it is the compiler
that built the binary bug 002 is about, and nothing in this engine has been verified
under it. Flooring at the suspect would write the fault into the build as a supported
configuration. The rule stands as written; this is the rule refusing to floor at a
version that fails its own condition, not an amendment to it.

**The consequence is stated plainly because somebody will meet it:** step 1 will
refuse on the principal's machine until he updates `slangc`. That is the check
telling him something true that nothing has told him for two years, and it is the
first time this project has been able to.
