# 0305 — Water is a blended plane shaded by wave normals over a depth copy
date: 2026-10-01
by: planner

## Decision
For 045:
1. **Every target keeps a depth copy.** The window's pair and every caller target gain a
   D32_SFLOAT sampled image per frame slot, made, resized and freed with its depth image, named by
   one texture slot as a target's colour is (so a target costs two slots). A new
   `voe_render_frame_copy_depth(device)`, inside an open camera pass, ends the rendering block,
   copies the depth image into the copy with the barriers either side, and resumes the block
   loading colour and depth. The pass's camera block carries the open target's copy slot,
   render-internal, with a sentinel when the pass has made no copy. `render/vulkan` is untouched.
2. **The shading record gains `water`**, the first word of `reserved_c`; the object record gains
   `waves` (height m, length m, seconds, deep m) and `sky` (linear rgb, w reserved). Zero is no
   water and the shader's path is unchanged for every other draw. Sizes asserted, mirrored in
   `draw.slang`.
3. **Waves are normals only**, no vertex moves: four fixed-direction sine waves summed
   analytically, wavelengths `length × {1, 0.61, 0.37, 0.23}`, amplitudes in proportion, over the
   plane's own metres (model XY times the world matrix's first two column lengths), so they hold
   far from the origin. The plane's normal and tangents come from the object's matrices, not the
   vertex normal (the store's quad leans its normals). Each angular frequency is
   `√(9.81·k)` rounded to a whole multiple of 2π/60, so the clock wraps at 60 s with no seam.
4. **The colour**: lit by the pass's sun, shadow and fill as any surface, roughness 0.05 for the
   glints; Schlick fresnel from 0.02 toward `sky`; the water's view-axis thickness to the depth
   copy (both distances from the pass projection's z and w rows, so perspective and orthographic
   both hold) over `deep` sets coverage (clear at the shore, nearly opaque at `deep`) and darkens
   the body colour; premultiplied out through the existing blend. No copy: treated as deep.
5. **`voe_3d_water` is a described component in `3d`** at "Rendering / Water", needing a
   transform: width and length (m, 20), colour (linear, a deep green-blue), sky (a pale blue),
   wave height (0.05 m), wave length (2 m), deep (3 m). Its intent is its replace. A runtime-only
   row, `voe_3d_waves` (the clock, a double kept below 60 s), is added and dropped by
   `voe_3d_water_system_run(world, seconds)`, run where the emitters run.
6. **The plane and its record are the model store's**: the store's existing quad, and one water
   shading record made by `voe_3d_models_load_water`, read through `voe_3d_models_water`. The
   game's loader loads it once a world has water, as it loads the dot.
7. **The draw**: each water with a transform is one world blended draw, world = transform ×
   (quad turned from XY to XZ, facing up) × (width, length); casts no shadow. Before the world's
   blended group, once, when any water is drawn, the draw system calls
   `voe_render_frame_copy_depth`.

## Reasoning
A sky, reflections and refraction are each a feature of their own; a depth copy is the one thing
the shore fade cannot do without, and it is cheap: one copy per pass that has water. Normal-only
waves keep the plane's depth exact for the fade and need no tessellation. Per-object values follow
ADR-0191: the record never changes, so the Inspector's edits go in the object record live. The
store already holds a shared quad and is set on every frame, so no caller gains a parameter.

## Replaces
None.
