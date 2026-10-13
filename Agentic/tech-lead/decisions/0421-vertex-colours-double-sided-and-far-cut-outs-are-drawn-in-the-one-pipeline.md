# 0421 — Vertex colours, double-sided and far cut-outs are drawn in the one pipeline
date: 2026-10-13
by: planner

## Decision
For 088's drawing.

1. **Vertex colours**: `voe_render_vertex` gains the colour's complement, four 16-bit UNORM channels
   read as 1 − value, so a zeroed vertex is opaque white and no vertex builder outside the model store
   changes. glTF's COLOR_0 is baked into it; it multiplies the base colour, alpha included, in every
   mesh exit and in the bounce capture.
2. **Double-sided** is the cull mode set per draw as dynamic state (core in Vulkan 1.3) on the solid and
   blended pipelines, through one `render` call; every pass opens culling back faces. No second pipeline.
   A back face is shaded with its normal turned round. Shadow and capture passes already cull nothing.
3. **A far cut-out stays solid**: in the cutout branch the coverage is multiplied by 1 + 0.25 × the
   colour map's level of detail before the cut, in the shader; no alpha-to-coverage, no mip rewrite.
4. **Mirrored normal maps** keep 0278's derivative frame, whose sign is the UV's handedness; a test pins
   that a mirrored half tilts mirrored. No tangent attribute.
5. **"Do not each cost their own draws"** in 088 is 0375 point 2: every imported material is a record of
   the one lit or unlit shader, drawn through the same pipeline whatever file it came from. Copies of a
   mesh in one instanced draw (0375 point 3) are built with 090's forest, whose test counts draws.

## Reasoning
- The complement rather than a flag: a flag needs a word in the object or shading record, neither has
  one free, and a material is shared across meshes with and without colours. Zeroed-is-ordinary is how
  every record in `render` already reads.
- Sixteen bits and not eight: glTF's COLOR_0 is linear, and eight linear bits band the darks.
- Dynamic cull mode rather than a pipeline per side: a pipeline switch per material is what 0375 point 2
  rules out.
- Scaling coverage by level (Ben Golus' fix) costs one level-of-detail query; alpha-to-coverage needs
  multisampling, which no target has, and rewriting mips per cut-out texture needs the cutoff at upload.
- A tangent attribute: MikkTSpace is a dependency or a large card, and the derivative frame already
  takes its handedness from the UV.
- Cut-outs do not cut their shadows: the shadow pipeline has no fragment stage. A tree's shadow of
  whole cards is a later card when it is seen.

## Replaces
Nothing. Amends 0278 point 3 only in that a test now pins mirrored halves.
