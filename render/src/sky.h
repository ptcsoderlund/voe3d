// The sky's radiance and the light it casts, worked out on the CPU (ADR-0406
// point 2, ADR-0407 points 2 to 6). Pure CPU, no Vulkan: a camera pass projects
// the sky onto nine harmonics for the frame block, and the tests hold the
// model to its claims.
//
//     voe_math_float3 sun = voe_render_sky_sun(light);
//     voe_render_sky_harmonics(&sky, light.direction, sun, sh);
//     voe_math_float3 e = voe_render_sky_irradiance(sh, normal);
//
// THE MODEL. An authored gradient and a glow, never scattering. Above the
// horizon the view runs from the horizon colour to `zenith` by a power of its
// height, the power rising with `horizon_height` (0 a hard band, 1 horizon
// colour almost to the top); below it, to `ground` within a few degrees. As the
// sun lowers, `sunset` blends the horizon on the sun's side towards WARM scaled
// to the horizon's brightest channel and dims the whole sky by up to
// EVENING_DIM, then by NIGHT_DIM more once the sun is well below. Added in the
// sun's colour: a glow, its exponent narrowing from GLOW_WIDE to GLOW_NARROW as
// `sun_glow` falls, and a disc of `sun_size` degrees at DISC_GAIN with a
// smoothed edge of DISC_EDGE × its radius, both fading out with night. Those
// two are the only parts brighter than authored. The radiance is the gradient
// plus those two; the gradient alone is the haze's colour (ADR-0409). Defaults
// (ADR-0406 point 1) are the scene's, not here.
//
// THE SUN IS ON unless its light is unshaded, of intensity 0, of colour nought
// or of no direction; voe_render_sky_sun gives its colour, not times its
// intensity, or nought. A sun of nought is the authored gradient: no disc, no
// glow, no sunset, and its direction is never read.
//
// THE TWIN. `render/shaders/sky.slangh` is the same function in Slang; a change
// here is made there too, and the two agree within 2/255 as drawn.
//
// THE WORLD IS +Y UP, AS IT COMES. The harmonics are over world axes, the nine
// real coefficients in the order (l, m) = (0,0) (1,−1) (1,0) (1,1) (2,−2)
// (2,−1) (2,0) (2,1) (2,2), each a float4 of rgb and a nought.
//
// THE HARMONICS LEAVE THE DISC OUT AND KEEP THE GLOW. The disc is the sun,
// which lights a surface on its own, and 256 directions would hit or miss it
// by luck; the glow is wide enough to be sky.
//
// CONSTRAINTS. The projection is 256 radiance evaluations per call, a few tens
// of microseconds; more directions would only follow the glow closer.
#pragma once

#include <render/device.h>

#include <math/float3.h>
#include <math/float4.h>

// The evening horizon's tint, scaled by the authored horizon's brightest.
#define VOE_RENDER_SKY_WARM_R 1.0f
#define VOE_RENDER_SKY_WARM_G 0.38f
#define VOE_RENDER_SKY_WARM_B 0.12f
// The sun's height (sine) below which evening starts, and the depth below the
// horizon (sine) at which night is whole.
#define VOE_RENDER_SKY_EVENING_HEIGHT 0.3f
#define VOE_RENDER_SKY_NIGHT_DEPTH 0.2f
// How much evening and night darken the gradient at their full.
#define VOE_RENDER_SKY_EVENING_DIM 0.4f
#define VOE_RENDER_SKY_NIGHT_DIM 0.9f
// The glow's exponent at `sun_glow` 0 is GLOW_NARROW, at 1 GLOW_WIDE.
#define VOE_RENDER_SKY_GLOW_NARROW 256.0f
#define VOE_RENDER_SKY_GLOW_WIDE 4.0f
// The disc's multiplier and its soft edge, in radii.
#define VOE_RENDER_SKY_DISC_GAIN 4.0f
#define VOE_RENDER_SKY_DISC_EDGE 1.1f
// Below the horizon, ground is reached at a height (sine) of 1 / GROUND_FADE.
#define VOE_RENDER_SKY_GROUND_FADE 10.0f
// Fibonacci-sphere directions the harmonics are projected over.
#define VOE_RENDER_SKY_DIRECTIONS 256

// The sun's colour for the sky: `light`'s colour when on, else nought.
voe_math_float3 voe_render_sky_sun(voe_render_light light);

// The evening-shifted, dimmed gradient of `sky` along unit `view`, no glow and
// no disc: the haze's colour (ADR-0409). Arguments as voe_render_sky_radiance.
voe_math_float3 voe_render_sky_gradient(const voe_render_sky *sky,
					voe_math_float3 direction,
					voe_math_float3 sun,
					voe_math_float3 view);

// The radiance of `sky` seen along unit `view` under a sun whose light travels
// along `direction` (the sun at −direction), `sun` from voe_render_sky_sun:
// voe_render_sky_gradient plus the sun's glow and disc.
voe_math_float3 voe_render_sky_radiance(const voe_render_sky *sky,
					voe_math_float3 direction,
					voe_math_float3 sun,
					voe_math_float3 view);

// `sky` without its disc over VOE_RENDER_SKY_DIRECTIONS directions, projected
// onto nine real harmonics and convolved with the cosine lobe into `out`.
void voe_render_sky_harmonics(const voe_render_sky *sky,
			      voe_math_float3 direction, voe_math_float3 sun,
			      voe_math_float4 out[9]);

// The irradiance E(n) on a surface of unit `normal`, from those nine.
voe_math_float3 voe_render_sky_irradiance(const voe_math_float4 harmonics[9],
					  voe_math_float3 normal);
