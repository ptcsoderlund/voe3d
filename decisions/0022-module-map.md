# 0022. The module map: eight folders, one direction

- **Status:** Accepted
- **Date:** 2026-08-28
- **Deciders:** Human, Tech Lead
- **Amended by:** ADR-0030 — `render` split into `render` (GPU layer) and `3d`; `text`, `sprite`, `ui` planned beside `3d`.

## Context

Phase 1 of ADR-0006, and exit criterion 1. Derived from the capability list in
`STATUS.md` per `guidelines.md`'s rule that grouping follows what code *does*.

Most of the shape was fixed before it was drawn: ADR-0007 puts the ECS near the
bottom; ADR-0017 means folder edges are data, never system calls; ADR-0018 makes
render the sole owner of GPU resources.

## Decision

Eight folders. An edge means *may depend on*; the graph is acyclic and points
one way only.

```
app                     the frame loop, wiring
├── render              Vulkan. The only folder that talks to it
│   ├── assets          glTF and image reading
│   └── platform        window, input, files, time
└── scene               transform, camera, light
    └── ecs             entities, components, intent queues
        └── base        memory, containers, strings, assert
            math        vectors, matrices, quaternions
```

| Folder | Owns | Depends on |
|---|---|---|
| `base` | Allocation, containers, strings, assert | — |
| `math` | Vectors, matrices, quaternions. Pure data, no systems | — |
| `ecs` | Entities, component storage, intent queues and dispatch | `base` |
| `scene` | Transform, camera, light components and their systems | `ecs`, `math`, `base` |
| `platform` | Window, input, files, time. The only OS-aware folder | `base` |
| `assets` | glTF and image reading, into CPU data | `base`, `math`, `platform` |
| `render` | Vulkan, GPU resources, the frame | `scene`, `ecs`, `assets`, `platform`, `math`, `base` |
| `app` | The frame loop and wiring | all |

**`base`, not `productivity`** — the namespace is spelled out in every call
(ADR-0014), and `v_base_arena_new` is typed far more often than it is read.

**Build order, in three waves:**

1. `base`, `math` — depend on nothing, need no GPU, prove the build works on
   both platforms before anything hard is attempted.
2. `platform`, `render` — the risky pair, and where the onboarding invariant is
   actually tested.
3. `ecs`, `scene`, `assets`, `app` — turns a demo into an engine.

## Consequences

- **`render` is the heaviest folder and the only one that names Vulkan.** Every
  other folder is written once, for both platforms.
- **`platform` is the only OS-aware folder.** Windows and Linux differences are
  confined to it, and the check script can assert that no other folder includes
  a platform header.
- **`assets` is separate from `render` so a model can be loaded without a GPU**,
  which makes it testable on a machine with no graphics card. This is the
  weakest edge in the map and the first candidate to merge if that turns out not
  to matter.
- **Physics, when it arrives, sits beside `scene`** — depending on `math`,
  `base` and `ecs`, reading transforms and submitting intents. Nothing above it
  changes. That it fits without disturbance is evidence the shape is sound.
- **`math` has no systems** and is not an ECS module — it is data with support
  functions, used inline (ADR-0014).
- Shaders live under `render`. Slang adds no folder.
- Splitting later is cheap; merging is not. `render` was deliberately not split
  into sub-folders, and window was not split from input, because both would be
  guesses about seams that have not appeared yet.

## Rejected options and why

- **Merging `base` and `math`** — rejected. `math` is pure value types with no
  allocation and no state; keeping it separate means it configures, tests and
  builds with no dependency at all, which ADR-0001 asks every folder to do and
  this is the one folder that can do it perfectly.
- **Splitting `render`** into device, frame and material sub-folders — rejected
  as premature. The seams are not yet visible, and a wrong split is expensive to
  undo.
- **A separate `input` folder** — rejected for the same reason. Window and input
  are both the OS talking to us, and on both platforms they arrive through the
  same event loop.

## Questions this opens

Closes **D-002**. Satisfies exit criterion 1 of the pre-study phase gate.
Unblocks D-010, D-012 and D-024, and clears the way for Phase 2 scaffolding
cards.
