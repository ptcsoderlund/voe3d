# 35 — Every light draws its disc in the sky
folder: render
after: 34
decisions: 0168, 0411, 0412

## Change
Tests for 0412 points 2 to 4, against the code of card 34; fix `render/src/sky.c` or
`render/shaders/sky.slangh` only if a claim fails. Read the headers of `render/src/sky.h`,
`render/tests/sky_model.c`, `render/tests/sky.c` and `render/tests/haze.c`.

- `render/tests/sky_model.c`, new cases (header list with them), the default backdrop (zenith
  (0.15, 0.35, 0.80), horizon (0.60, 0.75, 0.90), ground (0.20, 0.18, 0.16), height 0.25), each
  within 1e-4 unless named:
  - strength 0.5: straight up, the horizon and straight down read half of strength 1's; strength 0
    reads nought there;
  - an orange light (1, 0.5, 0) 30° up, disc 1.5, glow 0.3: the radiance along the light's own
    direction is (2, 1, 0) at strength 1, at strength 0, and at intensity 1 or 50;
  - that light unshaded, of intensity 0, or of disc and glow 0: the radiance everywhere is the
    backdrop;
  - two lights, red 30° up to the north and blue 20° up to the east: each one's own direction
    reads its own 2c, and a view between them is at least the backdrop in every channel;
  - a light's glow at 10° from it is above the backdrop at strength 0 too;
  - the gradient equals the radiance along every view with no light;
  - the harmonics' irradiance within 5% of a brute-force sum of the backdrop over 4096 directions,
    the lights present making no difference.
- `render/tests/sky.c`, one new case: the two lights above drawn by a pass (the second in `more`,
  `disc_size` 3 for both so each covers pixels), the pixel nearest each one's direction within
  2/255 of the CPU twin's radiance there after the pass's exposure and tone, and red over blue at
  the first, blue over red at the second.
- `render/tests/haze.c`, one new case: strength 0.25, the far quad at 2000 m reads within 3/255 of
  (1 − 2^−2) × the backdrop at strength 0.25 along its pixel.
- `render/tests/tests.md` (`sky_model.c`, `sky.c`, `haze.c`): say so, each one sentence of at
  most 300 characters.

## Done when
The tests `render/sky_model`, `render/sky` and `render/haze` pass after the folder's build.
