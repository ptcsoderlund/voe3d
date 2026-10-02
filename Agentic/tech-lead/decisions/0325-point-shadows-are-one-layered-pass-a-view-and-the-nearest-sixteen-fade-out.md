# 0325 — Point shadows are one layered pass a view, a draw per caster, and the nearest sixteen lamps fade out
date: 2026-10-02
by: planner

## Decision
For 050, carrying out 0301 and 0316:
1. **The maps.** `render` gains the capacity `point_shadow_size`, texels a side of one cube face, which may
   be nought, and `VOE_RENDER_POINT_SHADOWS` 16, the slots. Per frame slot one D32 2D-array image of
   6 × 16 layers, slot s (1-based) at layers 6(s − 1) to 6(s − 1) + 5, faces in the order +X −X +Y −Y +Z −Z.
   Drawing it needs the Vulkan 1.2 feature `shaderOutputLayer`; the device enables it when the card has
   it, and on a card without it treats the capacity as nought with one line at startup.
   `voe_render_point_shadows_ready` says whether the maps exist. `3d` asks for 256 (`VOE_3D_POINT_SHADOW_TEXELS`):
   25 MiB a frame slot.
2. **One pass, one draw per caster.** `voe_render_point_shadow_pass_begin` takes a pass's point lights and
   opens one pass onto every layer, cleared; the lights with a slot are what it casts for. In it,
   `voe_render_frame_draw` is one instanced draw: render finds the faces the caster's bounding sphere
   reaches — the sphere taken from the geometry's own vertices at create, under the object's world
   matrix, against each slotted light's range and each face's 90° pyramid — and draws one instance per
   face, the vertex stage writing the layer; a caster no face reaches draws nothing. The faces go to the
   vertex stage as a 96-bit mask in the push constant, the instance taking the n-th set bit. A hundred
   lamps cost one pass and at most one draw per caster, not a pass per lamp.
3. **A face's projection is computed, not stored.** Each face is a 90° reversed-depth perspective from the
   light's eye-relative position, near `VOE_RENDER_POINT_SHADOW_NEAR` 0.05 m, far the light's range, one
   Slang function in `shaders/point_shadow.slangh` used by the shadow vertex stage and by the lookup, so
   the two cannot disagree. The shadow pass's own point-light region holds its slotted lights by slot.
4. **The record.** `voe_render_point_light` grows to 48 bytes: `shadow`, the slot (0 none), and
   `shadow_strength`, 0 to 1. A lit surface a slotted light reaches picks the face by the major axis of
   (surface − light), pushes the surface one texel along its normal, takes one hardware-filtered compare
   and scales that light's term by lerp(1, visibility, strength). On a device whose maps are not ready the
   pass reads every slot as none.
5. **Which lamps cast: the nearest sixteen, fading by the seventeenth.** `3d` ranks the point lights with
   `cast_shadows` true and intensity above nought by the eye's distance to their sphere,
   max(0, |position| − range). The nearest 16 get slots 1 to 16 in rank order. D is the 17th's distance,
   unbounded with 16 or fewer; a slotted light's strength is 1 − smoothstep(0.75 D, D, its distance),
   and one at strength 0 gets no slot. D moves continuously with the eye, so a shadow fades out before
   its lamp loses its slot and nothing pops.
6. **Order.** `voe_3d_draw_system_point_lights` runs before `voe_3d_draw_system_shadows`, which, after
   the sun's cascades and bounce (each only when the sun casts, as before), opens the point-shadow pass
   when the device is ready and any of `frame->points` has a slot, and draws `voe_3d_draw_casters` into it,
   so a shape or model with `cast_shadows` false casts no lamp's shadow either. A point light bounces
   nothing until 051.
7. **Room.** A view spends one more pass and at most one more object per caster.

## Reasoning
Layered rendering with a culled instance per face is the batched drawing 0301 asks for, uses the core
1.3 renderer with one 1.2 feature desktop cards of the 0318 low end have, and needs nothing from
`render/vulkan`. Ranking by distance to the light's sphere makes the cut-off a continuous function of the
eye, so fading by it removes popping without hysteresis state.
- A pass per light or per face: what 0301 rules out.
- Multiview: every view takes every triangle, no culling per face, and a view count cap of 6–32.
- A 2D atlas with clip distances: the same instancing with a heavier vertex stage and atlas bookkeeping.
- Face matrices in a buffer: two views in one frame would overwrite each other's before the GPU reads.
- A cube-array view: needs `imageCubeArray` and the face picking is the same either way.
- Hysteresis on the chosen set: state across frames per view, and still a pop when it lets go.

## Replaces
Nothing. Amends 0320 point 3 (point lights cast no shadow) and 0324 point 4 (the shadows call opens more
than the cascades).
