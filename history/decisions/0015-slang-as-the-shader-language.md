# 0015. Slang is the shader language, compiled to SPIR-V

- **Status:** Accepted — with one sub-question open
- **Date:** 2026-08-28
- **Deciders:** Human, Tech Lead

## Decision

**Shaders are written in Slang and compiled to SPIR-V.** Not GLSL, not HLSL.

Slang is a Khronos-hosted shading language with HLSL-derived syntax, and unlike
GLSL it has real modules, generics and interfaces — the things that stop a large
shader codebase turning into `#include` and `#define` archaeology. Given ADR-0016
commits the engine to doing as much as possible on the GPU, the shader codebase
will be large, and the language it is written in stops being a detail.

**Still open (D-007a): offline or runtime compilation.** Offline — `slangc` at
build time, shipping `.spv` — is simpler and ships no compiler. Runtime
compilation buys shader hot-reload (tiered *later*) and specialisation. The
recommendation is offline for v1 with the seam kept, consistent with every other
"later" on this project.

## Consequences

- **Slang is a build-time dependency and a real test of the onboarding
  invariant.** It is a substantial C++ project. Building it from source at
  configure time would be slow; fetching a prebuilt `slangc` at configure time
  fits the model already established, but pins us to the platforms and versions
  they publish binaries for. **This must be verified, not assumed** — folded
  into the Vulkan bring-up spike alongside D-006, since both are the same
  question about what a clean machine can do.
- Slang's own toolchain details are the least-verified claim in this ADR and are
  flagged as such rather than stated with confidence.
- A shader-authoring path exists from day one, which pulls a small amount of
  work forward into v1 that a GLSL project would have deferred.
- Slang also targets HLSL, Metal and WGSL. This is noted and explicitly *not*
  relied upon — the platform given is Windows and Linux desktop, and treating
  portability we do not need as a reason for a choice would be deciding by
  brochure.

## Rejected options and why

- **GLSL** — the obvious Vulkan default, rejected because it has no module
  system worth the name, and ADR-0016 guarantees enough shader code that this
  bites.
- **HLSL via DXC** — closer to Slang in ergonomics and widely used, rejected
  because Slang is a superset in the ways that matter here and is the direction
  Khronos is investing in.

## Questions this opens

- **D-007a** — offline or runtime SPIR-V compilation.
- **D-029** — does acquiring Slang satisfy the onboarding invariant? Verified in
  the bring-up spike, with D-006.
