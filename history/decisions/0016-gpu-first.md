# 0016. GPU-first: compute is a primary tool, not an optimisation

- **Status:** Accepted — one consequence needs a decision, see D-030
- **Date:** 2026-08-28
- **Deciders:** Human, Tech Lead

## Decision

**As much work as possible happens on the GPU.** Compute shaders and GPU-side
functions are a primary tool from the start, not something retrofitted once a
CPU implementation proves too slow.

This is a design stance with immediate consequences, not a capability. The v1
list is unchanged — no particle system, no GPU simulation — but the renderer is
architected so compute is native rather than bolted on.

## The consequence that needs resolving

**GPU-first collides with ADR-0011 and ADR-0013, and the collision is real.**

Those decisions assume component data lives in CPU memory, has exactly one
writing system, and is changed by sending that system a typed intent. A compute
shader is none of those things: it is not a C function, it does not accept an
intent, and it writes from thousands of invocations at once.

So when a million particle positions live in a GPU buffer written by a compute
dispatch, **who owns them, and does the one-writer rule still hold?**

Three answers:

- **(a) GPU-resident truth.** The buffer is the data. A CPU component holds a
  *handle*, not values. Reading it back on the CPU is possible and expensive,
  and therefore rare and deliberate.
- **(b) CPU truth, uploaded each frame.** Keeps every existing rule intact and
  defeats the purpose of this decision.
- **(c) Per-component choice.** A few hundred transforms are CPU-truth; a
  million particles are GPU-truth.

**Recommendation: (c), with (a) as the rule for anything the GPU writes.** The
one-writer rule survives intact under this reading — the writer is still exactly
one system, it simply writes by dispatching a shader rather than running a loop.
What does *not* survive is stated below.

## Consequences

- **The database rule holds on the CPU side only.** "Anyone may read anything,
  read-only or by copy" is true of CPU memory. It is not true of GPU-resident
  data, where a read means a readback, a stall and a synchronisation point.
  This is the one place the principal's model does not extend, and it should be
  known now rather than discovered by someone writing an innocent-looking read.
- **This leans D-027 toward the renderer keeping its own per-frame layout.**
  GPU-driven rendering means the renderer builds GPU buffers and dispatches
  indirect draws rather than assembling a CPU-side list of things to draw.
  Bevy's answer — a second ECS world fed by extraction — is a worse fit under
  that model than it looked when D-027 was opened. Not decided here.
- **The shader codebase will be large.** This is the argument that decided
  ADR-0015: a language with modules and generics stops being a luxury once
  meaningful logic lives in shaders.
- Debugging moves to the GPU with the work, where it is harder. Validation
  layers, GPU markers and frame statistics are on the v1 list already, which
  turns out to have been the right call for a stronger reason than intended.
- Minimum hardware expectations rise. Compute is universal on any Vulkan 1.1+
  device, so this costs nothing on the stated platforms, but features like
  indirect draw counts and descriptor indexing are not — a floor will need
  naming when the renderer is designed.

## Questions this opens

- **D-030** — ownership and access rules for GPU-resident component data.
  Recommendation (c)/(a) above, undecided.
- Feeds **D-027**, and adds a hardware-floor question to the renderer design.
