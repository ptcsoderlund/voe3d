# 0018. GPU resources are referenced from components by id; Vulkan 1.3 baseline

- **Status:** Accepted
- **Date:** 2026-08-28
- **Deciders:** Human, Tech Lead

## Context

ADR-0016 left open how GPU-resident data relates to an ECS that lives in CPU
memory (D-030). The principal's direction: **the ECS is CPU memory; GPU memory
is a different beast and connects to the ECS by id** — with an explicit request
for a technical judgement on whether that is sound in Vulkan.

## Decision

**Sound, standard, and the right instinct.** Components never hold Vulkan
handles. A component holds a small integer id; the render module owns the actual
`VkBuffer`, `VkImage` and memory behind it.

**Sharpened one step: the id is the same number the GPU indexes with.** Using
descriptor indexing, all textures and buffers live in one large descriptor array
and shaders index it with an integer. Making the component's id *be* that index
removes the lookup table entirely — one number, meaningful on both sides of the
bus.

**Ids are generational** — an index plus a counter — so a stale id from a
destroyed resource is detected rather than silently addressing whatever now
occupies the slot.

**Vulkan 1.3 is the baseline**, for four features that are core there and change
how much code gets written:

| Feature | Buys |
|---|---|
| Descriptor indexing (core in 1.2) | The bindless id scheme above |
| Buffer device address (core in 1.2) | Shaders holding real pointers; struct-of-arrays passed by address |
| Dynamic rendering (core in 1.3) | No render pass or framebuffer objects — a large amount of boilerplate simply gone |
| Synchronization2 (core in 1.3) | Barriers that can be reasoned about |

## Consequences

- **This is what makes ADR-0016 practical.** GPU-driven rendering wants entity
  data in one large GPU buffer indexed by the same id, compute doing culling,
  and indirect draws issued from GPU-written commands. The id scheme is the
  spine of that; without it every draw needs CPU-side bookkeeping.
- **Hardware floor: anything supporting Vulkan 1.3.** On the stated platforms —
  Windows and Linux desktop — that is any reasonably current discrete or
  integrated GPU. It does exclude older hardware, and that is the cost.
- **The render module owns the id space.** Per ADR-0011 it is the only writer of
  the components carrying those ids, which is consistent rather than a special
  case.
- Answers **D-030** for the linkage. The remaining half — components whose
  *truth* lives on the GPU, such as compute-written particle state — keeps
  ADR-0016's recommendation: GPU-resident truth, read back rarely and
  deliberately.

## Rejected options and why

- **Vulkan handles in components** — rejected. It leaks the graphics API into
  every folder that touches a renderable thing, and makes components
  non-serialisable, which ADR-0010 forbids.
- **A CPU-side map from component id to GPU resource** — rejected as a table
  that exists only because the two numbering schemes were allowed to differ.
- **Vulkan 1.0/1.1 baseline with extensions** — rejected. The features above
  would all be extension-gated with fallback paths, for hardware the stated
  platforms do not require us to support.
