# 0405 — A sky is a scene row, drawn from the sun, lighting, hazing and exposing the picture
date: 2026-10-11
by: planner

## Decision
For 087.

1. **One row.** `voe_scene_sky` in `scene`, at "Rendering / Sky", needing no transform, a world holding
   at most one (the game's capacity 1). Fields: `light` FLOAT32 (how strongly the sky lights, default 1,
   at least 0), `haze` FLOAT32 (metres at which half of a surface's colour is haze, default 1000, 0 is
   none, at least 0), `exposure` FLOAT32 (stops, default 0, −16 to 16), `tone` UINT32 named `Clip` (0)
   and `Soft` (1), default Soft. Having the row is "the sky on". No row is the old picture, byte for
   byte, so every scene saved before 087 keeps its look.
2. **`render` takes a `voe_render_sky` record in `voe_render_pass_camera`**: `drawn`, `light`, `haze`,
   `exposure`, `tone`, padded to 32 bytes. Zeroed is the old picture: no sky drawn, no sky light, no
   haze, ×1, Clip. The sky's sun is the pass's `light` (direction, colour × intensity); an `unshaded`
   light or one of intensity 0 is a black sky. `3d` fills the record from the first sky row in
   `voe_3d_draw_system_frame`, so the game, the editor's views and its preview all get it.
3. **The sky is analytic, single scattering through a flat atmosphere**, one function written twice,
   `render/src/sky.c` and `render/shaders/sky.slangh`, agreeing within 2/255 as drawn. Zenith optical
   depths Rayleigh (0.0464, 0.108, 0.265), Mie 0.0252 (all three channels), air mass by Kasten–Young of
   the angle above the horizon (clamped at it); sunlight reaching the air is the sun's radiance ×
   exp(−τ·m(sun)); a view direction gathers (1 − exp(−τ·m(view))) / τ × (τ_R·P_R + τ_M·P_M) × that
   sunlight × 4, the 4 standing for the multiple scattering left out; P_R Rayleigh's phase, P_M
   Henyey–Greenstein at g 0.76; the sun's disc within 0.5° adds sunlight × 50. Below the horizon the
   horizon's colour darkens linearly to a tenth of it straight down. So evening is red and low, noon blue.
4. **The sky is drawn at a camera pass's begin when `drawn`**, by a sixth pipeline: one full-screen
   triangle, no depth test or write, before any draw, timed as the span `sky`. Everything later draws
   over it, the blended draws included.
5. **The sky lights lit surfaces.** At pass begin `render` projects the sky's radiance onto nine
   spherical-harmonic coefficients on the CPU (sky.c, 256 directions) into the frame block. A lit
   surface adds `(1 − metal) × base × E_sky(n)/π × occlusion × light` and a reflection `sky(R) ×
   EnvBRDFApprox(F0, roughness, N·V) × occlusion × light`, sky(R) blending towards E_sky(R)/π as
   roughness rises (Karis's analytic split-sum, no cube maps, no LUT). Both reach a surface only when
   its blocker mask holds no Indoors or Room bit; the bounce and fills are unchanged and add beside it.
6. **Haze at every mesh exit**, lit, unlit, water and blended: colour × (1 − f) + sky(v) × f, f = 1 −
   2^(−d/haze), d the view distance, sky(v) the sky along the view direction with its height clamped
   to the horizon; a blended draw hazes its straight colour, alpha kept.
7. **Exposure then tone at every mesh exit and the sky's**: × 2^exposure, then Clip (as is, the target
   clamps) or Soft (Khronos PBR Neutral), a blended draw on its straight colour. The target stays
   8-bit sRGB; there is no HDR target. Elements are never touched.
8. **Overlay draws are drawn as authored**: every draw after a pass's first
   `voe_render_frame_clear_depth` (outline, gizmo, markers) takes no haze, exposure or tone, by a fifth
   push-constant word `plain`.

## Reasoning
- Tone mapping each fragment before an 8-bit target, against an HDR target and a resolve pass: the
  target format and the read-back stay as they are, nothing is added to every frame without a sky, and
  blending before the curve differs only on see-through draws.
- An analytic sky evaluated per pixel, against a cube map rendered when the sun moves: no images, no
  re-render on a day cycle, exact mirror reflections; the CPU twin is the blockers' pattern (0347).
- Spherical harmonics for the sky light, against a hemisphere of two colours: evening light comes from
  the west and SH keeps that for nine float4s.
- Exposure and tone on the sky row, against the camera: the editor's views have no camera row, and a
  scene saved before stays unchanged because it has no row.
- PBR Neutral, against ACES: base colours below about 0.8 stay as authored, so materials read as made.

## Replaces
Nothing. Ends `render/include/render/device.h`'s "no exposure and no tone mapping yet".
