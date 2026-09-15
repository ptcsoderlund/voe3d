# 0020. GPU work is justified by measured gain, not taken by default

**Rule:** Work moves to the GPU when there is a performance reason to move it.
Where there is no gain, it stays on the CPU. "As much as possible on the GPU"
means *as much as pays for itself*, not as much as will fit.

**Status:** Accepted · 2026-08-28
**Amends:** ADR-0016. The GPU-first stance stands — compute is a primary tool
and the renderer is architected for it. What changes is that it is no longer
unconditional.

**Why:** Principal's call, and it removes the failure mode ADR-0016 invited.
Without this qualification, "do as much as possible on the GPU" becomes a reason
to move work across the bus for consistency's sake — the same over-application
already rejected once, when "ECS for everything" was narrowed to keep the
renderer's per-frame internals out of component tables.

Moving work to the GPU is not free: it costs an upload, a dispatch, a
synchronisation point, and a debugging story that is worse than a breakpoint.
Below some amount of work, all of that exceeds what the parallelism returns.

**Cost accepted:** "No performance gain" is a claim about measurement, and this
project has no CI and no benchmark harness. The v1 capability list already
carries on-screen frame statistics, which is the minimum needed to make the rule
honest rather than rhetorical. Where a move is genuinely uncertain, the answer
is a spike, not an argument.

**Closes D-030.** Components are CPU memory by default. A component whose truth
lives on the GPU — compute-written particle state and the like — exists only
where the gain justifies it, and is owned by the system that dispatches the
compute that writes it. The one-writer rule holds either way; the writer just
happens to write by dispatch rather than by loop.
