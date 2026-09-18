# 0030. `render` is the GPU layer; `3d`, `text`, `sprite` and `ui` are renderers on top of it

- **Status:** Accepted
- **Date:** 2026-08-28
- **Deciders:** Human, Tech Lead
- **Amends:** ADR-0022 (the module map), ADR-0024 (roadmap unchanged; folder placement added)
- **Superseded by:** —

## Context

ADR-0022 fixed one `render` folder and rejected splitting it: "the seams are
not yet visible." The principal has now named one: the Vulkan plumbing is
shared by the 3D renderer and, later, by text, 2D and GUI rendering, and they
should be separate projects that share it rather than duplicate it.

This is Bevy's shape exactly — `bevy_render` (device, resources, pipelines)
with `bevy_pbr`, `bevy_text`, `bevy_sprite` and `bevy_ui` as separate crates on
top — and Bevy is the philosophical reference by the principal's direction.

Options weighed: split now (this ADR); keep one folder with an internal seam
enforced by the check script's include grep, and split when text arrives; or
never split and grow `render` into everything GPU. The tech lead recommended
the seam-then-split path, on the grounds that a device layer designed against
one consumer gets its boundary slightly wrong. **The principal chose to split
now**, in Bevy's shape and with Bevy's names. The concern is recorded once here
and not again: the first `3d` cards will find `render`'s public surface
incomplete and will grow it. That is acceptable because both folders are ours,
in one tree, and `render`'s API is not frozen by anyone else consuming it.

## Decision

**The Vulkan layer is `render`. The 3D renderer moves out of it into `3d`.
Three more renderers are planned beside `3d`: `text`, `sprite`, `ui`.** All
four depend on `render` and never on each other, except where the ADR that
creates them says otherwise.

| Folder | Owns | Depends on | When |
|---|---|---|---|
| `render` | Vulkan: instance, device, swapchain, GPU resources by generational id (ADR-0018), pipelines and shader loading, render targets and present (ADR-0024), command submission. Knows nothing about scenes, entities or files | `platform`, `math`, `base` | v1 |
| `3d` | The 3D frame: meshes, materials, camera and light into draws, drawn into a render target | `render`, `scene`, `ecs`, `assets`, `math`, `base` | v1 |
| `text` | Glyph rasterisation and atlases, laying out glyph runs into quads | `render`, `assets`, `math`, `base` (expected) | later |
| `sprite` | Textured quads and 2D batches | `render`, `math`, `base` (expected; `ecs` if sprites are components) | later |
| `ui` | Rectangles, layout and draw lists for a GUI, in the world or over the frame | `render`, `text`, `sprite`, `math`, `base` (expected — the one sideways edge among renderers, as in Bevy) | later |

**Keep them low level.** Each renderer turns already-parsed data into draws.
None of them parses a document format: SVG, HTML, rich-text markup and the like
are *later*, are not designed here, and when they arrive they live in `assets`
or a folder of their own — not in the renderer that draws the result. Font
*file* parsing (TrueType/OpenType) is likewise `assets`' job, per ADR-0022's
rule that `assets` reads files into CPU data.

**The module map becomes:**

```
app                     the frame loop, wiring
├── 3d                  the 3D renderer: scene → draws → a render target
│   ├── render          Vulkan. The only folder that names it
│   │   └── platform    window, input, files, time
│   ├── assets          glTF, images, fonts → CPU data
│   └── scene           transform, camera, light
│       └── ecs         entities, components, intent
├── text  · sprite · ui planned; each on render, low level
└── base · math         depend on nothing
```

Nine folders at v1: `base`, `math`, `ecs`, `scene`, `platform`, `assets`,
`render`, `3d`, `app`. The `cmake/voe.cmake` map (ADR-0027) holds these nine;
`text`, `sprite` and `ui` are added to it by the card that creates each, under
this ADR's authority, with their edges as expected above unless that card's ADR
says otherwise.

**Build order, amended:** wave 1 `base`, `math`; wave 2 `platform`, `render`
— the onboarding-invariant test, now cleanly separable from any scene code;
wave 3 `ecs`, `scene`, `assets`, `3d`, `app`.

## Blast radius

**Reversibility: moderate.** Merging `3d` back into `render` is a directory
move plus deleting a `DEPENDS` line. The expensive direction is the one this
ADR pays for up front — designing `render`'s surface with one consumer — and
that cost is paid in `3d`'s early cards growing `render`, not in a rewrite.

## Consequences

- **`render` has no `ecs`, `scene` or `assets` edge.** A GPU resource id
  (ADR-0018) is a `render` type; the component that holds it lives in `3d` or
  a later renderer. Uploading a mesh takes CPU data from `assets` via `3d`;
  `render` never opens a file.
- **ADR-0024's roadmap stands** — 3D → good enough → text → sprite → ui — and
  now has a folder per step.
- **Vulkan bring-up (exit criterion 4) is `render` alone**, with `platform`
  for the window. A cleared frame needs no scene, which is a cleaner spike
  target than before.
- **`render`'s public surface will be wrong at first**, in the sense of
  incomplete. Expected and priced above. What must not happen: `3d` reaching
  into `render`'s internals to work around a gap. The gap gets a card in
  `render`.
- **Nine standalone configures in the check script**, not eight. Seconds.
- **ADR-0022 is amended, not superseded.** Its reasoning for the other seven
  folders, its rejection of an `input` folder, and its build-order logic all
  stand; only "render is not split" is reversed, by the seam it said it was
  waiting to see.

## Rejected options and why

- **Seam now, folder later** — the tech lead's recommendation; rejected by the
  principal in favour of a real folder wall from day one. The principal's
  reasoning: the split is inevitable, the reference engine has it, and a wall
  the build enforces is worth more than a naming rule the check script greps.
- **Never split; text/2D/GUI as modules inside `render`** — rejected. `render`
  would become half the engine and the standalone property would mean nothing.
- **A single `render` with an RHI-style backend abstraction** — not proposed
  and rejected on sight: Vulkan is the only API (given), so an abstraction over
  it has one backend, which is the premature-generality case the standing
  rules name.

## Questions this opens

- **D-035 (new)** — what `render`'s v1 public surface is: resource upload,
  pipeline creation from Slang, render-target and frame API. Decided against
  the first `3d` card, not before.
- **D-027** (renderer internals — ECS, per-frame layout, or an extract step)
  now lives in `3d`, and the extract-step option has a natural home: `3d`
  reads `scene` and produces `render` commands.
- **D-024** (component type descriptions) unaffected.
