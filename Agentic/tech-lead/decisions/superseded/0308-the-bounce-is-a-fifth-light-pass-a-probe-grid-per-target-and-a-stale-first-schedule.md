# 0308 — The bounce is a fifth light pass, a probe grid per target and a stale-first schedule
date: 2026-10-01
by: planner

## Decision
For 046, carrying out 0307:
1. **Bounce map.** A fifth pass of the sun, `voe_render_bounce_pass_begin(device, light view, sun)`, after the
   cascades. It counts against `passes`, its draws against `objects`. Per frame slot: D32 depth, RGBA16F flux and
   RGBA16F normal images, `VOE_RENDER_BOUNCE_TEXELS` 512 square. A fourth graphics pipeline, the draw vertex stage
   plus a fragment writing flux = sun colour × intensity × base colour (factor × texture × object colour), no N·L
   (a texel is projected area), and the world normal. The casters are the cascades' casters.
2. **Its view.** Fitted by 3d to the grid's bounding sphere (radius √3 × half the cube), looking from the sun,
   reaching 200 m toward it, snapped to whole VPL blocks in double; the cascades' basis and snap are shared.
3. **Probe grid.** One per target, the window's included: `VOE_RENDER_BOUNCE_PROBES` 32 a side,
   `VOE_RENDER_BOUNCE_SPACING` 2 m. L1 spherical harmonics, RGB, in three RGBA16F 3D images, one copy, addressed
   toroidally (cell mod 32) and sampled trilinear with repeat; the bounce fades to nothing at the grid's edge. The
   grid's centre is eye + forward × 32 m, in whole cells about the world origin.
4. **Update.** `voe_render_bounce_update(device, target, update)` with the grid's lowest cell, its corner about the
   eye, and stale spheres about the eye. It uses this frame's last bounce pass; false with none. One compute
   shader, `bounce.slang`: reduce the map to 64×64 VPLs (position from depth through the inverse light matrix,
   mean normal, flux × block area), then gather every VPL into each listed probe and blend. A VPL's radiance
   toward a probe is flux × max(0, n·−ω) / (π × max(d², 1 m²)).
5. **Schedule.** CPU, inside render. Cells a scroll brings in are refreshed whole that frame, beyond the budget,
   and replaced. Then stale cells — inside a sphere, or all when the light record changed — then a strided cycle
   over the rest. `VOE_RENDER_BOUNCE_BUDGET` 4096 probes an update, blend `VOE_RENDER_BOUNCE_BLEND` 0.5.
6. **Reading it.** A camera pass reads its target's grid only when that grid was updated this frame. Lit shading
   = direct + base × max(E/π, fill weight × fill), per channel, E from the SH with A0 = π, A1 = 2π/3. Unlit and
   unshaded passes read none.
7. **Stale spheres.** 3d marks, for each caster that moved this step (the previous table's lag 1 differs from lag
   0), a sphere of `VOE_3D_BOUNCE_REACH` 6 m at each place. `voe_3d_frame` gains the target it draws to; zero is
   the window.
8. **Shader parts.** Code shared between shaders lives in `render/shaders/*.slangh`, a dependency of every shader;
   `draw.slang` is split into parts by what each does.

## Reasoning
The sun already has a pass per cascade with its casters; a fifth, smaller one costs a draw list and no new
machinery. 32³ at 2 m covers the 64 m in front of the camera where a bounce is seen; L1 is the least that tells
the ground from a wall. Gathering 4096 VPLs into 4096 probes is 16 M taps, a fraction of a millisecond. Refreshing
entered cells whole keeps a scroll from showing black; stale-first is 0307's "what changed goes first". One grid
per target keeps editor views from scrolling each other's grid.

## Replaces
Nothing. Carries out 0307.
