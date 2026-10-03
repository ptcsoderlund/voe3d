# 0326 — The bounce is a grid of captured probes, relit a level at a time and read with visibility
date: 2026-10-02
by: planner

## Decision
For 051, carrying out 0317:
1. **The fields.** `voe_scene_light` gains `bounce_strength` FLOAT32, last: default and unsaid row 1, finite
   and not negative. `VOE_SCENE_LIGHT_BOUNCES_MAX` is 3, named "0" to "3". `voe_scene_point_light` gains
   `bounces` UINT32 (the sun's maximum and names) and `bounce_strength` FLOAT32, last, defaults 0 and 1. A
   file without a key reads its default, so an 047 sun at 1 stays 1. `voe_render_point_light`'s two
   reserved words become `bounces` and `bounce_strength`.
2. **The grid.** Per target, built at the top of the frame after the first `voe_render_bounce_begin` onto
   it (the GPU idles once, as a resize does) and freed the same way after 300 frames with no begin.
   `VOE_RENDER_BOUNCE_PROBES_XZ` 24 by `_Y` 12 by 24 probes, `VOE_RENDER_BOUNCE_SPACING` 2 m, a probe at
   its cell's centre, addressed toroidally per axis. 3d centres it on eye + forward ×
   `VOE_3D_BOUNCE_AHEAD` 24 m, in whole cells.
3. **A probe sees.** Its picture is a cube of six `VOE_RENDER_BOUNCE_FACE` 8-texel faces out to
   `VOE_RENDER_BOUNCE_REACH` 24 m: linear albedo (RGBA8 sRGB) and world normal with distance (RGBA16F,
   reach where nothing). A capture pass is 0325's layered drawing with the probes as its lights:
   `VOE_RENDER_BOUNCE_CAPTURE` 16 probes on a per-slot scratch of 96 layers, one instanced draw per
   caster over the faces it reaches, nothing culled, then copied into the target's atlases (probe
   (i, j, k) at x = 48i, y = 8(12k + j), 1152 × 2304). At most `VOE_RENDER_BOUNCE_CAPTURE_PASSES` 4 a frame.
   It needs `shaderOutputLayer`; without it nothing bounces.
4. **What is captured.** Render keeps, per grid, which probes hold a picture. Probes a scroll brings in
   lose theirs; probes within a stale sphere (3d's, 6 m where a caster moved) are queued again. The queue
   is taken nearest the eye first. Nothing queued: no capture pass opens.
5. **Settle.** Each probe captured or emptied since the last relight gets its validity, 0 when it holds
   no picture or a quarter of its texels see back faces (it stands inside something), else 1, in an R16F
   3D image; and its distance moments, mean and mean² over each texel's 3×3 of its face, in an RG16F
   atlas laid out as the pictures.
6. **Levels.** A light with bounces n belongs to chain n. Grid L(n, k), k ≤ n, six in all, and the sum S:
   L1 SH RGB in three RGBA16F 3D images each. Relight, one workgroup per valid probe over its 384 texels,
   each weighted by its solid angle: radiance = albedo × (k = 1: Σ over chain n's lights of colour ×
   intensity × bounce strength × max(0, n·l) × its shadow — the sun's cascades where they cover, a lamp's
   falloff and its point-shadow slot; k > 1: E(L(n, k − 1)) / π read as in 7), nought on no hit or a back
   face. Levels k = 1 to 3, then S = Σ L. The lamps are the first `VOE_RENDER_BOUNCE_LAMPS` 16 of the
   update's point lights with bounces. It runs over every valid probe, and only on a frame that captured
   or emptied a probe or whose bouncing lights (the sun record, counts, strengths, the lamp list) differ
   from the last relight. Otherwise only the read runs.
7. **The read.** One function in `bounce_read.slangh`, used by lit shading and the relight: the eight
   probes about the surface pushed 0.25 × spacing along its normal, each weighted by trilinear ×
   validity × (((n · to-probe + 1) / 2)² + 0.2) × Chebyshev visibility from its moments toward the
   surface, normalised; nought with no weight; faded to nothing over the grid's outer cell. Lit = direct
   + base × max(E/π, fill floor); no gain. A pass reads its target's S only when that target was begun
   this frame and its grid is built.
8. **The calls.** `voe_render_bounce_begin(device, target, frame)` with placement, stale spheres, sun,
   its bounces and strength, the shadow record and the point lights; `voe_render_bounce_capture_pass_begin
   (device, &opened)`, a pass, false when `passes` is spent, `opened` false when nothing is left;
   `voe_render_bounce_relight(device)`. 3d makes them from `voe_3d_draw_system_shadows` after the
   point-shadow pass when the sun row bounces with intensity, or a frame point light bounces. The sun
   bounces whether or not it casts, unshadowed when it does not, as its direct light is.
9. 046's bounce map, pass, pipeline, VPLs, gather, schedule, gain and standoff are removed.

## Reasoning
Captured pictures with distance moments are the DDGI shape without ray tracing: rasterised by the
shared six-view drawing, relit without redrawing, leak-stopped by visibility and back-face validity,
and on flat ground every probe sees the same plane, so the ground is even. Levels per chain give each
light its own count for 1 MB of grids. Relighting every valid probe when anything changes is a fixed
cost far below rasterising; only capture is local.
- Octahedral irradiance per probe: borders and seams to filter, where L1 SH is smooth and was in 046.
- A relight list grown about each change: higher levels reach past any radius; a full relight is cheap.
- Building every grid at startup: 0316 says no image while nothing bounces.

## Replaces
Nothing. Carries out 0317; amends 0319 point 4 and 0325 point 6.
