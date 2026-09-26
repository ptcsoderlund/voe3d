# 0258 — The sun casts through four stable cascaded shadow maps
date: 2026-09-26
by: planner

## Decision
For 028 (0252), the technique:

1. **Cascaded shadow maps.** The sun renders depth into four cascades
   (`VOE_RENDER_SHADOW_CASCADES`), each a layer of one D32 array image per frame slot, 2048
   texels a side (`VOE_3D_SHADOW_TEXELS`), sized by a new `shadow_size` capacity that may be
   nought (no shadows, one texel of room so the binding stays valid).
2. **A shadow pass is a pass.** `render` gains a shadow pass onto one cascade, opened with the
   light's view as its camera block and closed with `voe_render_pass_end`; mesh draws in it go
   through a depth-only pipeline (the draw vertex stage, no fragment stage, no culling, slope
   depth bias). It counts against `passes`, its draws against `objects`. Views in one frame
   reuse the slot's cascades in turn: shadows, that view's pass, the next view's shadows.
3. **The camera pass reads them.** `voe_render_pass_camera` gains a `voe_render_shadow` record:
   four eye-relative light matrices, four split distances and a count, zero meaning none. The
   fragment stage picks the cascade by view depth, offsets along the normal, and takes a 3×3
   comparison-filtered lookup; the sun's direct light is multiplied by it. The comparison
   sampler's linear filter is on depths, not a picture (as FIELD is on distances).
4. **Stable and far.** `3d` computes cascades on the CPU: splits by the practical scheme with a
   blend of 0.9 toward logarithmic, from the near plane to the lesser of the far plane and
   500 m; each cascade bounds its slice by a sphere, so its size never changes with the view's
   turn, and snaps its centre to whole texels in the light's space, taken in double about the
   world origin, so shadows stand still as the camera moves and 100 km out as near the middle
   (0250). Each box reaches 200 m further toward the sun for casters outside the slice.
5. **Who casts and who receives.** Casters are world-layer, lit, opaque or cutout meshes (a
   cutout casts as solid); receivers are lit surfaces. No light, a blind camera, an unlit
   surface: no shadow (0238). With no ambient term a shadowed surface is as dark as one facing
   away, which is what one sun and no bounce is.
6. **The loop calls it.** `voe_3d_draw_system_shadows` runs between the frame's begin and the
   view's pass and fills the frame's shadow record; the game and the editor's views call it.

## Reasoning
Cascaded maps are the standard dynamic sun shadow and the first step of dynamic lighting: a
later spot or point light is another map in the same shape, and the pass, pipeline and lookup
stay. Stability by sphere and texel snap is the known cure for shimmer; doing the snap in double
is what 0250 demands. Rejected: one map fitted to the scene (blurry near, or small reach);
virtual or ray-traced shadows (far beyond one feature); a map per view (twice the memory for
nothing, the passes are sequential anyway).

## Replaces
nothing. `render/device.h`'s "there is no shadow" and `draw.slang`'s "no shadows" are amended.
