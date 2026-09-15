# 0060. `render`'s public surface is by-id functions, grown on demand by `3d`

- **Status:** Accepted
- **Date:** 2026-09-03
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —
- **Closes:** D-035

## Context

ADR-0030 deferred `render`'s v1 public surface to the first `3d` card, so it
would be designed against a real caller. Card 018 is that caller. Today the
surface is two functions — `voe_render_device_new` and `_frame` — and a header
that says it is deliberately close to nothing.

Constraints already fixed: ADR-0018 (resources by generational id), ADR-0033
(the four depth/Y-flip fixtures, not open), ADR-0050 (per frame slot),
ADR-0051 (the frame is an offscreen target), ADR-0059 (pools, per-object
buffer, material buffer), ADR-0034 (on demand), and `CLAUDE.md`'s rule that a
gap in `render`'s API gets a function in `render`, never a reach-around.

## Options considered

### Option A — a designed resource/pipeline/frame API up front
A full RHI-shaped surface: buffers, images, samplers, pipelines, passes, command
lists, all designed now. Every function without a caller is ADR-0034's
"in case", and ADR-0008 already rejected the RHI-with-one-backend shape.

### Option B — exactly the functions `3d` calls, by id, added as called
`render` grows one function per line in `3d` that needs it. Every resource is
named by generational id. `device_internal.h` stays internal. The surface is
whatever the sum of callers is, and each card that needs more adds it *in
`render`*.

## Decision

**Option B.** The deciding factor is that ADR-0034 and `CLAUDE.md` already
say this; the ADR exists so the register row can close against a real caller.

**Ownership clarification, recorded here because ADR-0018 and ADR-0030
disagree on it:** ADR-0018 says `render` writes the components carrying GPU
ids; ADR-0030 later removed `ecs` from `render`'s dependencies, so `render`
cannot see a component. **`render` owns the id space; `3d` owns the `mesh`
and `material` components that carry ids.** The later decision governs.

The functions card 018 is expected to add, each justified by its caller in
`3d`, and none added without one:

| Need in `3d` | `render` provides |
|---|---|
| Put geometry on the GPU | Append vertices / indices to the pools; returns the range. Pools created with the device, capacity a parameter |
| Put decoded pixels on the GPU | Create a texture from pixels; returns a generational id that *is* the shader's index. Destroy by id; a stale id is rejected |
| Per-object and material records | Write a frame slot's per-object buffer and the material buffer from CPU arrays |
| Draw the frame | Begin the frame into the offscreen target with the camera's view-projection; draw a range with an object number; end and present |

Shapes not decided here and not to be invented: pipelines other than the one
opaque pipeline, samplers other than one default, render targets other than the
frame's, compute dispatch. Each arrives with the card that calls it.

## Blast radius

**Cheap.** Adding functions is additive; the ids and pools (ADR-0059) are the
only structural commitment, and they are that ADR's, not this one's.

## Consequences

- `render`'s header stays honest: it lists what exists, and every function has a
  caller in the tree.
- A card in `text`, `sprite` or `ui` that needs something `render` lacks adds
  it in `render` under this ADR, no new decision needed, provided it is by id
  and names no scene, entity or file.
- `render/tests/` gains tests for id creation, destruction and stale rejection.
- The `_frame` call of today, which draws two hardcoded cubes, is replaced by
  begin/draw/end; `dev` keeps working through `3d`.

## Rejected options and why

- **Option A** — every function without a caller is a function nobody knows is
  wrong until something calls it. ADR-0008 and ADR-0034 both said no already.

## Questions this opens

None new. D-052 (where `slangc` runs once a second folder has shaders) is
unaffected: `3d`'s shader replaces `render`'s cube shader and is the *first*
folder's shaders still, unless the coder finds it belongs in `3d` — in which
case D-052 triggers and the card says so.
