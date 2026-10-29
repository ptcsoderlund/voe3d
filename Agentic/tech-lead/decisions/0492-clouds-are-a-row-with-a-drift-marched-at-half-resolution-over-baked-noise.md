# 0492 — Clouds are a row with a drift, marched at half resolution over baked noise
date: 2026-10-29
by: planner

## Decision
For 101, carrying out 0480 points 1 to 3.

1. **The row.** `voe_scene_clouds` at "Rendering / Clouds", no transform, one a world, the first row
   whose entity is on used (0440). Fields: `kind` UINT32 named Flat (0) and Volumetric (1), default
   Flat (0316: the cheap one); `coverage` 0 to 1, default 0.5, 0 no cloud anywhere and 1 cloud over
   every point of the layer; `height` the layer's bottom, world metres, any finite, default 1500;
   `thickness` metres, at least 0, default 600, read by Volumetric only; `wind_direction` degrees,
   any finite, the way the wind blows towards, 0 towards −Z and 90 towards +X; `wind_speed` metres a
   second, at least 0, default 10; `shadow_strength` 0 to 1, default 0.6. The unsaid row is the
   default row.
2. **The drift is a runtime-only row** `voe_scene_clouds_drift` (double `x`, `z`: metres the clouds
   have moved), never saved. `voe_scene_clouds_system_run(world, seconds)` drains the replaces, adds
   a drift at 0 to each row lacking one, drops orphans and moves each by wind × seconds. It runs
   where water's run does, the editor's world step and the game's fixed step, so clouds drift in
   edit, Play and a shipped game alike.
3. **`render` takes `voe_render_clouds`**, 32 bytes, in `voe_render_pass_camera` after `exposure`:
   `kind` (0 none, the old picture byte for byte; 1 flat; 2 volumetric), `coverage`, `bottom` (the
   layer's bottom in metres above the eye, height − eye y, as `haze_top`), `thickness`, `origin`
   (float2: the eye's place in the cloud field, eye x − drift x and eye z − drift z, wrapped into
   [0, VOE_RENDER_CLOUDS_PERIOD) in double by `3d`), `shadow_strength`, a reserved word. The field
   repeats exactly every VOE_RENDER_CLOUDS_PERIOD, 8192 m, on x and z, so a float origin keeps its
   precision however far the eye or the wind goes.
4. **The field is baked, not computed per sample.** A 256² R8 tiling weather map over the period
   (cloud shapes, Perlin fBm) and a 64³ R8 tiling detail volume repeating every 256 m (inverted
   Worley fBm), made on the CPU at device open, the same bytes every run, bound at bindings 13
   (detail) and 14 (weather) through one repeating linear sampler. Coverage takes the weather to a
   base density, its end points as in point 1; Volumetric shapes it by a height profile over
   [bottom, bottom + thickness] and erodes it by the detail; Flat is the same at one plane, y =
   bottom, as an opacity.
5. **Drawn after the world.** `voe_render_frame_draw_clouds`, which `3d`'s run calls after the
   world's blended draws and before the overlay's depth clear: it copies depth if the pass has not,
   marches at half resolution into an RGBA16F image kept per target and frame slot beside the depth
   copy (premultiplied light, transmittance), each texel to the furthest depth of its 2×2, at most
   64 steps, start jittered by interleaved gradient noise, ending under 1% transmittance; then
   composites at full resolution, bilinear, a pixel nearer than where its ray first meets the layer
   taking none. The clouds are veiled by the haze as the sky is (0418 point 2), over the part of the
   path to the cloud below the haze top, then exposed and toned as a blended mesh exit. Timed as the
   span `clouds`; kind none records nothing.
6. **Lit by the lights alone (0411).** Each shown directional light, colour × intensity, through a
   forward-leaning Henyey–Greenstein phase, attenuated towards the first light by three samples of
   the shape without detail and towards the others by the depth below the layer's top; the shade is
   the lights' fills plus a share of each light scattered many times inside. Never the sky, so a low
   orange sun colours both sides and the far side is darker.
7. **Shadows.** On every lit surface, water included, each directional light's direct term is
   multiplied by 1 − shadow_strength × (1 − T), T the transmittance along the light through the
   shape without detail (four samples Volumetric, the plane Flat), 1 above the layer or with the
   light below the horizon. It is not in the sun's reach that weighs the fill (0276), so the fill
   and the objects' shadows are unchanged. The bounce does not take it. 102's godrays read the same
   function.
8. **Reflections.** A metal's sky reflection (0412 point 5) and water's sky colour take the clouds
   along the reflected direction: one sample where it meets the layer's middle, lit as point 6,
   never veiled (0418 point 5).

## Reasoning
Clouds that can be flown into must cover a tower inside them, so they are drawn over the world with
its depth, not behind it in the sky stage. Half resolution is a quarter of the marching at the
1440p bar (0388) and the steps bound it; the breakdown's span says what it costs (0358). Baked
tiling noise turns a sample into a texture read, and one field read by the march, the shadows and
the reflections keeps all three the same clouds (0480 point 2). The drift as a runtime row follows
water's clock (0305), so the editor and a game step it the same way and nothing unsaved is saved.
Alternatives considered:
- Full-resolution marching: rejected, four times the cost for detail the bilinear upsample keeps.
- Noise computed in the shader: rejected, tens of noise evaluations a step at every pixel.
- A cloud shadow map drawn from the sun: rejected, a pass and an image for what a few reads of the
  field give, and at its resolution it would disagree with the clouds seen.
- Cloud shadow inside the sun's reach: rejected, the fill would rise under a cloud, which the
  feature's step 4 forbids.
- A clock in `3d` instead of a drift: rejected, wind changed mid-flight would jump the clouds.

## Replaces
Nothing. Carries out 0480 points 1 to 3.
