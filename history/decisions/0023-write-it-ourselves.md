# 0023. Write it ourselves; dependencies are consulted, not adopted

**Rule:** The default answer to a third-party dependency is **no — write it**.
Adding one is a decision the principal takes, not an implementation detail an
agent or a card may settle. This includes the glTF parser, which is written
in-house.

**Status:** Accepted · 2026-08-28
**Decides:** D-004, the dependency policy.

**Why:** Principal's call — *"We do everything on our own. Adding dependencies
should be consulted first."* It is also the cheapest possible answer to the
onboarding invariant: code that is already in the tree cannot fail to be
fetched, cannot drift, cannot pin us to someone's platform support, and cannot
be pulled out from under the project.

**What this makes us write.** For v1, in rough order of nastiness:

| | Assessment |
|---|---|
| JSON parser | Small and well-bounded. A day's work for the subset glTF needs |
| glTF reader | A *subset* — static meshes, positions, normals, UVs, indices, one material, base colour texture. The full spec, with skins, morph targets and sparse accessors, is not v1 |
| PNG decode | The genuinely nasty one: it requires an inflate implementation. Well-specified, deterministic, testable, but the largest single piece of work in this list |
| Containers, strings, allocators | `base`. Ordinary work, and the kind C projects write anyway |

**Recommendation on the sting:** scope the glTF reader to what v1 draws, and
treat PNG as its own card rather than smuggling it inside "load a model". If
inflate looks like it will dominate the schedule, an uncompressed texture format
for v1 is a legitimate way to defer it — the engine needs textures, not
specifically PNG.

**Cost accepted:** More code we own, and correctness risk in places where a
mature library would have been right. glTF in particular has subtleties —
accessor strides, node transform composition, handedness — where a bug presents
as a rendering artefact and is hunted in the wrong folder. Mitigated by scoping
to a subset and by testing `assets` without a GPU, which ADR-0022 deliberately
made possible.

**What remains a dependency.** Not everything can be written:

- **Vulkan headers and the loader.** The loader ships with the GPU driver on
  both platforms. See D-006 — the recommendation is to vendor the headers and
  load the loader dynamically at runtime, which leaves *no* build-time
  dependency at all.
- **Slang.** Already resolved: an installed tool, not a fetched dependency
  (ADR-0021).

**Consequence for the build:** the fetched side of ADR-0021's line may end up
empty. That is a good outcome — it makes the onboarding invariant trivially
true rather than carefully defended.

**Closes D-026** — "which glTF parser" no longer has a shortlist.
