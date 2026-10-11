# 03 — Render works out the sky on the CPU
folder: render
after: 02
decisions: 0168, 0405

## Change
The C half of 0405 point 3's twin and point 5's projection, pure CPU, called by nothing yet but its test
(cards 04 and 05 call it). Read `render/src/light_bins.h` and `render/src/light_bins.c` as the pattern of
an internal pure-CPU part, and `render/tests/light_bins.c` for its test.

- `render/src/sky.h` (new, internal, header comment): the model in phrases (0405 point 3), that
  `render/shaders/sky.slangh` (card 04) is its twin and a change is made in both, and the constants
  as named `#define`s: the two zenith optical depths, g, the gain 4, the disc's 0.5° and 50, the
  ground's tenth, 256 directions.
  - `voe_math_float3 voe_render_sky_radiance(voe_math_float3 direction, voe_math_float3 sunlight,
    voe_math_float3 view)`: `direction` where the light goes (the sun is at −direction), `sunlight`
    the light's colour × intensity, `view` unit; the radiance seen along `view`.
  - `void voe_render_sky_harmonics(voe_math_float3 direction, voe_math_float3 sunlight,
    voe_math_float4 out[9])`: the radiance over 256 Fibonacci-sphere directions projected onto nine
    real SH coefficients and convolved with the cosine lobe, so the next call gives irradiance.
  - `voe_math_float3 voe_render_sky_irradiance(const voe_math_float4 harmonics[9], voe_math_float3
    normal)`: E(n) from the nine, as the shader will read it.
- `render/src/sky.c` (new, header comment): the three.
- `render/tests/sky_model.c` (new, no device): sun overhead, the zenith's blue over red above the
  horizon's; sun 5° above the horizon, the horizon towards it redder than blue, and darker overall
  than at noon; the sun's own direction over ten times a direction 5° beside it; sunlight nought is
  black everywhere; straight down a tenth of the horizon below the sun's side; irradiance up above
  down at noon and within 5% of a brute-force cosine-weighted sum of the radiance over 4096
  directions worked out in the test, for up, a horizontal normal towards the sun and one away.
- `render/src/src.md`, `render/tests/tests.md`: an entry each for the new files. Each at most 300
  characters.

## Done when
The test `render/sky_model` passes after the folder's build.
