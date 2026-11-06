# Foliage cost

## What is wanted
- The sponsor finds the forest's foliage numbers "kinda bad": view target 1: foliage at 13.31 ms of 16.38 ms, 3,136,035 triangles, 192 draws, in a small editor scene view with "tree asset.glb" (a debug run); they asked whether we do GPU instancing. [intent · tech-lead · 2026-10-30]
- On release the forest still takes "view target 1: foliage" 8.88 ms of 11.27 ms, in a scene view of about 880×975 pixels, 3,243,207 triangles, 167 draws; the sponsor's monitor runs at 164.84 Hz. Work order 114 asks for the forest to be cheap to draw. [intent · tech-lead · 2026-10-30]
- 114's target is at most 3 ms for foliage with the scene view filling the window: "3ms is fine, gotta start somewhere"; a starting target, open to change once measured. [confirmed · tech-lead · 2026-10-31]

## What it does not move with
- "Even with or without shadows, heavy LOD scaling or not. fps is the same." The forest's cost does not move with shadows or with how hard the LODs cut triangles. [confirmed · tech-lead · 2026-10-30]

## Why it costs
- The forest's cost is taken to be fragment overdraw of cut-out leaves, drawn with discard and depth write; 114 lays depth first instead of first adding a counter to measure it, see 0508. [assumed · planner 114 · 2026-11-06]

## Laying depth, then shading
- A lay-step draw counts in the Frame panel's draws, so draws rise while triangles still mean only what the picture holds; see 0508. [assumed · planner 114 · 2026-11-06]
- Slang 2026.17 cannot mark the position Invariant (`invariant`, `__invariant`, `[vk::invariant]` all fail) and `precise` on a matrix multiply emits no NoContraction; lay and shade agree under depth EQUAL only because both run one vertex entry from one module, layout and vertex input. [observed · coder 114-01, planner 114 · 2026-11-06]
- The lay step's keep test reads the same base-colour alpha as the shade step: factor, distance field and vertex colour are formed once and shared by both, so a leaf kept by one is kept by the other. [observed · coder 114-01 · 2026-11-06]
- The lay step's fragment writes no colour under the lay pipeline's write mask 0; returning a colour is the fallback only if validation warns. [assumed · coder 114-01 · 2026-11-06]
- Whether validation warns about that colourless fragment is unchecked: the debug build on the sponsor's machine could not load the validation layer ("best practices checks missing"), so render/prepare passed without it. [open · coder 114-01 · 2026-11-06]
