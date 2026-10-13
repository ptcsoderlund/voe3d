# 0406 — The sky is authored, its light is the sun's fill, and exposure is a picture row
date: 2026-10-12
by: tech-lead

## Decision
For 087, after the human's test. Games look like games: nothing physical unless it is cheap. A component
holds only its own settings: the light holds light, the sky holds how the sky looks, and the whole picture's
settings sit on a row of their own. Amends 0405 points 1, 2, 3 and 5; points 4, 6, 7 and 8 hold as written,
"the sky" in them now meaning the sky below.

1. **The sky row holds only the sky's look.** `voe_scene_sky`, still "Rendering / Sky", one a world, having it
   is the sky on, no row is the old picture byte for byte. Fields, all authored:
   - `zenith`, `horizon`, `ground`: linear colours, drawn as authored, as base colours are. Defaults zenith
     (0.15, 0.35, 0.80), horizon (0.60, 0.75, 0.90), ground (0.20, 0.18, 0.16).
   - `horizon_height`: 0 to 1, how far up from the horizon towards the zenith the horizon colour reaches,
     so 0 is a hard band and 1 a sky that is horizon colour almost to the top. Default 0.25.
   - `sun_size`: the disc's angular radius in degrees, 0 to 10, 0 no disc. Default 1.5.
   - `sun_glow`: 0 to 1, how strong and wide the halo around the sun is. Default 0.5.
   - `sunset`: 0 to 1, how much the sky turns to evening as the sun lowers. 0 keeps the authored colours at
     every sun angle; 1 turns the horizon on the sun's side warm and darkens the whole sky as the sun nears
     and passes the horizon, towards night when it is well below. Default 0.5.
   - `haze`: as 0405 (metres at which half of a surface's colour is haze, default 1000, 0 none).
   `light`, `exposure` and `tone` leave the row.
2. **The sky is a gradient and a glow, never a scattering model.** The colour along a view is the authored
   gradient from ground through horizon to zenith, shifted by `sunset` for the sun's height, plus the disc
   and glow in the directional light's colour. The disc and glow are the only part that may be brighter
   than the authored colours. It does not scale with the light's intensity: the sky is as bright as it is
   authored. Its sun is the world's first directional light's direction and colour; an unshaded light or
   one of intensity 0 gives the authored gradient with no disc, no glow and no sunset shift, never a black
   sky. Still one function written twice, C and Slang, agreeing within 2/255; the curves are the planner's,
   so long as they are cheap per pixel and meet the endpoints above. 0405 point 3's physical model,
   constants and ×4 go.
3. **The sky's light on surfaces is the first directional light's fill.** When a world has a sky, that
   light's fill takes the sky's colours instead of its `fill_colour`: the diffuse and the reflection of
   0405 point 5 (spherical harmonics of the drawn sky, Karis's analytic split-sum), scaled by the light's
   `fill_intensity` where 0405 scaled by the sky's `light`, and still reaching a surface only outside
   Indoors and Room blockers. Inside them, and in any world with no sky, the fill is `fill_colour` ×
   `fill_intensity` exactly as before. The other lights' fills and the bounce are unchanged.
4. **Exposure and tone are a picture row.** `voe_scene_picture` in `scene`, at "Rendering / Picture", no
   transform, at most one a world, the first in table order is the one used: `exposure` (stops, −16 to 16,
   default 0) and `tone` (`Clip` 0, `Soft` 1, default Soft), meaning what 0405 point 7 says. It is a world
   row, not a camera field, for 0405's reason: the editor's views have no camera row. No picture row is ×1
   and Clip, so a world with a sky and no picture row is exposed as is. The pass's camera record carries the
   picture beside the sky; `3d` fills both from their first rows, for the game, the preview and the
   editor's views (`voe_3d_draw_system_sky` and its picture twin). Bloom and colour grading would join this
   row later.
5. **No migration.** No scene with a sky row exists outside 087's branch; a sky row saved during 087's test
   may lose its removed fields.

## Reasoning
The human, after testing 0405's sky: "I want games to look like games. Not photorealistic, unless its
cheap", and "directional light should have settings for light and sky should have settings for sky and not
light." 0405's model made the hill's horizon twice as bright as a sunlit white surface and coloured the whole
sky by an orange sun, with no setting that shaped the sky itself.
- An authored gradient, against tuning the physical model (capping the air mass, lowering the gain): tuning
  still gives no control over the colours and keeps the cost of scattering per pixel.
- Keeping the physical model as a second mode, against dropping it: two skies to test and keep in step for a
  look nobody has asked for; it can return as a mode.
- The sky light as the sun's fill, against a `light` field on the sky: the fill is already the light's
  setting for light from all around, and one strength in one place is what the human asked for.
- A picture row, against exposure on the camera: the editor's views have no camera; against keeping it on
  the sky: it is not a sky setting.

## Replaces
Amends 0405 points 1, 2, 3 and 5.
