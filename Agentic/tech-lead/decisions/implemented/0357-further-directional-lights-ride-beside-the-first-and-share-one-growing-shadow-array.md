# 0357 — Further directional lights ride beside the first and share one growing shadow array
date: 2026-10-05
by: planner

## Decision
How 058 carries out 0349.

1. At most `VOE_RENDER_DIRECTIONAL_LIGHTS` (4) directional lights are drawn, in light table order. Lights
   past the fourth are left out. The first light stays where it is today: `light`, `shadow` and the
   blockers' `sun` mask of a pass camera, and `sun`, `sun_bounces` and `sun_strength` of a bounce begin.
   The others ride in a new `more` list, `voe_render_directional_lights`. Each entry,
   `voe_render_directional_light`, holds its light, its shadow record, the mask of the blockers holding
   its place, its bounces and its bounce strength. An empty `more` is the picture of one light, byte for
   byte, so every old scene and test stands.
2. Light adds up. Each light's direct term is gated by its own mask along its own segment and shadowed
   by its own cascades. The fills add too: each light's fill is weighted by its own reach-based fade
   and its own Room and Indoors gate, and the indirect term is `base × max(E/π, Σ weighted fills)`.
3. One shadow array per frame slot, `slot × 4 + cascade` its layers. `voe_render_shadow.reserved0`
   becomes `slot`. A device starts with one light's layers.
   `voe_render_shadow_lights_ready(device, wanted)` answers how many lights' maps the array holds. A
   want beyond that grows every slot's array at the top of the next frame, with one GPU wait, to at
   most 4 lights. It never shrinks before close. 3d gives slots to the first casting lights in table
   order, up to what is ready. A light without a slot draws unshadowed for that one frame.
4. The relight's sun map becomes an array of 4 layers per frame slot, made at startup on a device with
   `shaderOutputLayer`: 16 MiB a slot. Layer i is sun i, the first light 0 and `more[i − 1]` after it.
   `voe_render_bounce_shadow_pass_begin` names that sun and opens once per sun per begin.
5. Duplicate no longer refuses a light: a copy is a second directional light, which 0349 allows.
   The marker stays on the first light until 060 gives every light its own.

## Reasoning
- Beside the first, not a list that replaces it. Many render and 3d tests set `.light`, `.shadow`
  and `sun_bounces`; zero meaning old keeps them all.
- Grown and not sized at startup. Four lights' cascades at 2048 are 256 MiB a slot. A scene that casts
  with one light must not pay for that (0316).
- The relight map array is made at startup, because growing it too would be a second lazy image for
  12 MiB.
- A fixed cap of 4 matches the shader's fixed loop, and is past a sun, a moon and a cave's light.

## Replaces
Nothing. Amends 0258 (cascades per light), 0273 (Duplicate refusing a light) and 0329 (one relight
map per sun).
