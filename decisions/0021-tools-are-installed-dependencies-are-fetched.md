# 0021. Tools are installed by the programmer; dependencies are fetched by the build

- **Status:** Accepted
- **Date:** 2026-08-28
- **Deciders:** Human, Tech Lead
- **Supersedes:** ADR-0019

## Context

ADR-0019 kept `slangc` off the critical path by committing compiled SPIR-V, so a
clean clone would build with no shader compiler present. The principal's reading
is different and simpler:

> *"Slang compiler on the side is logic. Same as having clang on the side.
> Programmer need to install the tools."*

Slang is a compiler. The project already requires a compiler. Requiring a second
one is not a new category of demand.

## Decision

**`slangc` is a required tool, installed by the programmer, alongside Clang and
CMake. Compiled SPIR-V is not committed.** The build invokes `slangc`; there are
no generated artifacts in version control and no drift to police.

**The onboarding invariant is restated, not weakened.** Its wording — "nothing
beyond a compiler and CMake" — was never about counting installers. It was about
refusing to make a contributor assemble an environment. The line it actually
draws, now written down:

> **Tools that transform source are installed by the programmer.
> Anything the engine links against or ships is fetched by the build.**

Clang, CMake and `slangc` are tools. The Vulkan headers and loader, a glTF
parser, and any library the engine links are dependencies.

## Why the line matters more than the rule

Without it, *"the programmer installs the tools"* erodes in one step into *"the
programmer installs the Vulkan SDK"*, and the invariant that has decided more
questions in this pre-study than any other quietly dies.

The distinction holds it: the Vulkan SDK is not a compiler. It is headers, a
loader, validation layers and libraries — things the engine builds against and
ships beside. It stays on the fetched side of the line, and **D-006 is not
reopened by this decision.**

## Consequences

- **A clean clone now needs three tools, not two.** Stated plainly in the README
  and in the engine's own rules, because a promise that needs a footnote is not
  a promise.
- No generated artifacts in the repository, and no possibility of `.spv` drifting
  from `.slang`. This is the clear win over ADR-0019 and the reason that decision
  was superseded rather than amended.
- The build fails on a machine without `slangc`, with a message that says so and
  says where to get it. That is a better failure than silently building stale
  shaders, which was ADR-0019's quiet risk.
- `slangc` is distributed as a prebuilt binary per release, which is a
  materially lighter install than a full SDK. The invariant's spirit — no
  environment assembly — survives intact.
- **The check script gains a job:** confirm the three tools are present and meet
  their version floors before doing anything else, so a missing tool is
  diagnosed rather than discovered halfway through a build.

## Rejected options and why

- **ADR-0019's committed SPIR-V** — superseded. It protected the two-tool
  promise by moving build output into version control, trading a clean repo for
  a promise the principal did not need protecting.
- **Fetching `slangc` at configure time** — rejected, and it was ADR-0015's
  original flagged risk. It pins the project to the platforms and versions Slang
  publishes binaries for, and puts a network fetch of a compiler on the critical
  path of every fresh configure.

## Questions this opens

- **D-032** — the `slangc` version floor, and where the required-tools list is
  stated so a contributor meets it before their first build fails.
