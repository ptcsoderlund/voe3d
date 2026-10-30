# 0493 — Fog is placed boxes with a drift, marched at half resolution and lit by the shadowed sun
date: 2026-10-29
by: planner

## Decision
For 102, carrying out 0480 points 4 and 5.

1. **The row.** `voe_scene_fog_volume` at "Rendering / Fog volume", needing a transform, many a
   world, a box placed, turned and scaled by its transform as a light blocker's is. Fields: `size`
   FLOAT3, metres along its local axes before scale, finite and not negative, default (40, 10, 40);
   `thickness` 0 to 1, default 0.4, 0 no fog; `patchiness` 0 to 1, default 0.6, 0 even and 1
   patches with clear air between; `wind_direction` degrees, any finite, towards which it blows, 0
   towards −Z and 90 towards +X, as the clouds'; `wind_speed` metres a second, at least 0, default
   1; `godrays_strength` 0 to 1, default 1. The unsaid row is the default row. An off entity's
   volume is no fog (0427).
2. **The drift is a runtime-only row** `voe_scene_fog_volume_drift` (double `x`, `z`), never saved,
   moved by `voe_scene_fog_volume_system_run(world, seconds)` exactly as 0492 point 2 moves the
   clouds', run beside it in the editor's world step and the game's fixed step.
3. **`render` takes `voe_render_fog`** in `voe_render_pass_camera` after `clouds`: up to
   `VOE_RENDER_FOG_VOLUMES` (8) records and a count, copied whole. A record, 96 bytes: `rows[3]`
   taking a position about the eye to the box's unit space as a light blocker's; `patches` (float3)
   and `density`; `change` (float3) and `patchiness`; `godrays`; three reserved words. `3d` fills
   them: `density` is 0.1 × thickness² per metre, the extinction in the thickest place;
   `patches` is eye − drift wrapped into [0, VOE_RENDER_FOG_PATCHES) = [0, 128 m) on every axis,
   and `change` is eye − (1.5 × drift x, 0.25 × |drift|, 1.5 × drift z) wrapped into
   [0, VOE_RENDER_FOG_CHANGE) = [0, 32 m), both in double. Rows past 8 in table order are left out.
4. **The shape** is the clouds' detail volume (binding 13) read twice, no new texture: the patches
   at one repeat every 128 m about `patches`, eroded by the change at one repeat every 32 m about
   `change`. As the change moves at a different speed and rises, the shapes change as they drift
   rather than slide as one sheet. Density is `density` × the edge fade × lerp(1, patch, patchiness),
   the patch 0 between patches and 1 inside. The edge fade is the product over the three axes of a
   smooth fall from 1 at 60% of the half size to 0 at the face, so the corners round off and no
   box shape shows. Overlapping volumes add.
5. **Drawn after the clouds.** `voe_render_frame_draw_fog`, which `3d`'s run calls right after the
   clouds' draw: it reuses the target's half-size clouds image, free once the clouds are
   composited, and the same march-then-composite sequence, factored out of `clouds_draw.c`. The
   march reaches to a checkerboard depth: a half-size texel with x + y even marches to the nearest
   of its 2×2, odd to the furthest. It steps the union of the view's crossings of the volumes within
   the reach, at most 32 steps shared by length, the first jittered by interleaved gradient noise,
   stopping under 1% transmittance, and writes premultiplied light and transmittance, the light
   hazed as a surface at the fog's mean distance is (`voe_render_haze`). The composite at full
   size weighs the four nearest texels bilinearly and by how close each texel's chosen depth is to
   the pixel's own, so the fog does not bleed across a trunk's edge; then exposes and tones as a
   blended mesh exit, as the clouds' composite does. Timed as the span `fog`; no volumes records
   nothing.
6. **Lit by the first directional light and the fills (0411).** A sample takes the first light's
   colour × intensity × a phase half isotropic and half Henyey–Greenstein at g 0.6, so beams show
   with the sun behind the camera, times lerp(1, S, godrays). S is one compare in that light's
   cascades at the sample (fully lit past the last) times the clouds' shadow there
   (`voe_render_cloud_shadow`, 0492 point 7). The shade is every shown light's fill, as a white
   surface's floor takes it. Godrays 0 is the fog lit evenly; 1 takes only the fill in shadow.
   Light blockers do not gate the fog: buildings shadow it through the cascades.
7. **The editor lines every fog volume's box** as it lines a light blocker's, faint, the selected
   one bright; the volume is picked by its place marker.

## Reasoning
A box per volume follows the light blocker (0347), so the gizmos, a parent and the Inspector need
no case of their own, and the box's rows are the record the shader already knows how to test.
Reusing the clouds image and the detail volume adds no memory and no binding; the march sequence
is shared rather than copied. The checkerboard depth and the depth-weighted upsample keep half
resolution's saving (0492 point 5) while the fog among trunks keeps their edges. Godrays as a lerp
towards the shadowed light give the feature's step 6 directly: the fog stays, its beams fade. The
first light alone keeps the march to one cascade compare a step, the cost the laptop bar asks for;
the span `fog` shows it (0358).
Alternatives considered:
- A fog image of its own per target: rejected, the clouds image is free when the fog marches.
- Full-resolution fog: rejected, four times the cost; the depth-weighted upsample keeps the edges.
- Bilinear upsample to the furthest depth as the clouds do: rejected, fog would halo every trunk.
- Godrays from every directional light: rejected, a cascade lookup per light per step.
- Thickness as metres of visibility, as haze's: rejected, a larger number would be thinner fog.

## Replaces
Nothing. Carries out 0480 points 4 and 5.
