# 0019. The shader compiler is a separate tool; compiled SPIR-V is committed

**Rule:** Slang is **not** a build dependency. `slangc` is managed on the side,
as developer tooling. The compiled `.spv` output is committed to the repository,
so building the engine never requires Slang, a network, or a shader compiler of
any kind.

**Status:** Superseded by ADR-0021 · 2026-08-28
**Amends:** the acquisition consequence of ADR-0015. The choice of Slang as the
shader language stands unchanged.

**Why:** Principal's suggestion, and it is the strongest protection of the
onboarding invariant found so far. ADR-0015 recorded Slang as a substantial C++
project whose configure-time acquisition was unverified and flagged as the
riskiest claim in that decision. This removes the risk rather than testing it:
a clean machine needs Clang and CMake, exactly as promised, and Slang is needed
only by someone changing a shader.

**Closes D-029** — "does acquiring Slang satisfy the onboarding invariant" no
longer has to be answered, because acquiring it is no longer on the critical
path. The bring-up spike is correspondingly narrowed to the Vulkan question
alone.

**Cost accepted:** Generated artifacts live in version control, which is
normally a smell, and `.spv` can drift from the `.slang` source when someone
edits one and forgets the other. Mitigation: when `slangc` is present, the local
check script recompiles and compares. When it is absent — the clean-clone case —
there is nothing to check and nothing that needs checking.

**Note:** `.spv` is binary in a project that prefers text (ADR-0010). No
conflict: the *source* is Slang text and stays authoritative. The `.spv` is
build output that happens to be cached in the repository, in the same category
as the binary the build produces.
