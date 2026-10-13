# 31 — Under a sky a low sun lights what faces it
folder: render
after: none
decisions: 0168, 0408, 0410

## Change
Bug 04: a low sun's N·L leaves flat ground near black and under the fill's 0.3 fade, so fill
brightens it. 0410 points 1 to 3. Read the header of `render/shaders/lighting.slangh` and the
comments above `voe_render_sun_radiance`, `voe_render_fill_weight`, `voe_render_sky_light` and
`voe_render_directionals` in it, the comment above `voe_render_sky` in
`render/include/render/device.h`, and `render/tests/sky_light.c`.

- `render/shaders/lighting.slangh`:
  - two named constants beside `VOE_FILL_FADE_REACH`: the terminator's edge (0.05) and the floor
    (0.5) of 0410 point 1;
  - new `float voe_render_sun_reach(float n_dot_l, float shadow, bool lifted)`: `N·L × shadow`,
    or 0410's lifted curve times shadow when `lifted`;
  - `voe_render_directionals` takes each light's reach from it, `lifted` true only for light 0
    where the frame block's sky is `drawn`; that reach feeds the light's radiance, its fill and
    `sky_weight` as the plain one did. Lights 1 and up and a skyless frame are unchanged. The
    water path is not touched.
  - Header: the directional paragraph and the sky paragraph say light 0's reach under a drawn sky
    is lifted (0410) and that a surface the sun reaches past about 3° takes no fill or sky diffuse;
    the comments on the new function and `voe_render_directionals` say the same.
- `render/include/render/device.h`: the `voe_render_sky` comment, where it says what `drawn` does,
  adds that light 0's reach is lifted (0410). Nothing else in the file changes.
- `render/tests/sky_light.c`, a new case and its line in the header's list, the case the editor
  draws: the sky drawn at the test's values, light 0 a light pink (1, 0.75, 0.8) at intensity π,
  its fill colour blue (0.2, 0.4, 1.0), the grey cube's +Z face; the sun 3° up, its direction such
  that N·L on the face is sin 5° (towards the sun `(√(1 − sin²3° − sin²5°), sin 3°, sin 5°)`):
  - at fill 0 the face's summed bytes are at least 0.6 × the summed bytes with the same sun 3° up
    straight behind the camera (N·L near 1), same fill;
  - the face reads byte for byte the same at fill 0, 0.562 and 1.
  Both fail on the code before this card (the face reads about a third, and fill lifts it).
  Existing cases stand; if one read a face with the sun under ~0.3 reach through the sky (case 2's
  sun 2° up is on the face's side at N·L near 1, so it should not), it takes the new reach.
- `render/shaders/shaders.md` (`lighting.slangh` entry) and `render/tests/tests.md` (`sky_light.c`
  entry): changed where what they say changed. Each at most 300 characters.

## Done when
The tests `render/sky_light`, `render/directional_lights`, `render/blocker_kinds`,
`render/unshaded`, `render/haze` and `render/sky` pass after the folder's build.

Human, in the editor on lumberhill_project with the sky on and the sun low: at fill 0.001 the
ground, the cube's sunward side and the slab's top read clearly lit in their colours; raising fill
to 0.5 leaves them as they are and brightens only what the sun does not reach.
