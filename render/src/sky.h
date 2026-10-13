// The sky's backdrop, its lights' glows and discs, and the harmonics a rough
// metal reflects, worked out on the CPU (ADR-0411, ADR-0412 points 2 to 4).
// Pure CPU, no Vulkan: a camera pass projects the backdrop onto nine harmonics
// for the frame block, and the tests hold the model to its claims.
//
//     voe_math_float3 l = voe_render_sky_radiance(&sky, lights, count, view);
//     voe_render_sky_harmonics(&sky, sh);
//     voe_math_float3 e = voe_render_sky_irradiance(sh, normal);
//
// THE MODEL. The backdrop is `strength` times an authored gradient, never
// scattering: above the horizon the view runs from `horizon` to `zenith` by a
// power of its height, the power rising with `horizon_height` (0 a hard band, 1
// horizon colour almost to the top); below it, to `ground` within a few
// degrees. It does not follow any light. A SHOWN light, not unshaded, of intensity above 0 and colour not
// nought, adds in its colour (not times intensity or `strength`) a glow, its
// exponent narrowing from GLOW_WIDE to GLOW_NARROW as its `glow_size` falls.
// The glows of every light come first; then each shown light's disc of
// `disc_size` degrees, at or above the horizon, is laid over what is there by
// a smoothed edge of DISC_EDGE × its radius: a lerp to DISC_GAIN × its colour,
// so a disc's centre is 2c whatever the sky. Defaults are the scene's.
//
// THE TWIN. `render/shaders/sky.slangh` is the same arithmetic in Slang; a
// change here is made there too, and the two agree within 2/255 as drawn.
//
// THE WORLD IS +Y UP, AS IT COMES. The harmonics are over world axes, the nine
// real coefficients in the order (l, m) = (0,0) (1,−1) (1,0) (1,1) (2,−2)
// (2,−1) (2,0) (2,1) (2,2), each a float4 of rgb and a nought. They are of the
// backdrop alone, no glow and no disc (0412 point 4).
//
// CONSTRAINTS. The projection is 256 backdrop evaluations per call, a few tens
// of microseconds; more directions would gain nothing on a smooth gradient.
#pragma once

#include <render/device.h>

#include <math/float3.h>
#include <math/float4.h>

#include <stdint.h>

// The glow's exponent at `glow_size` 0 is GLOW_NARROW, at 1 GLOW_WIDE.
#define VOE_RENDER_SKY_GLOW_NARROW 256.0f
#define VOE_RENDER_SKY_GLOW_WIDE 4.0f
// The disc's centre as a multiple of its light's colour, and its soft edge, in
// radii.
#define VOE_RENDER_SKY_DISC_GAIN 2.0f
#define VOE_RENDER_SKY_DISC_EDGE 1.1f
// Below the horizon, ground is reached at a height (sine) of 1 / GROUND_FADE.
#define VOE_RENDER_SKY_GROUND_FADE 10.0f
// Fibonacci-sphere directions the harmonics are projected over.
#define VOE_RENDER_SKY_DIRECTIONS 256

// Whether `light` is in the sky: not unshaded, of intensity above 0 and of a
// colour not nought (ADR-0412 point 3).
bool voe_render_sky_shown(voe_render_light light);

// The backdrop of `sky` along unit `view`, `strength` included, no glow and no
// disc.
voe_math_float3 voe_render_sky_gradient(const voe_render_sky *sky,
					voe_math_float3 view);

// The radiance of `sky` seen along unit `view`: the backdrop, every shown
// light's glow, then their discs laid over. `lights[i]` is entry i of the
// sky's sizes, `count` at most VOE_RENDER_DIRECTIONAL_LIGHTS; a shown light's
// direction is where its light travels (it stands at −direction), not nothing.
voe_math_float3 voe_render_sky_radiance(const voe_render_sky *sky,
					const voe_render_light *lights,
					uint32_t count, voe_math_float3 view);

// The backdrop of `sky` over VOE_RENDER_SKY_DIRECTIONS directions, projected
// onto nine real harmonics and convolved with the cosine lobe into `out`.
void voe_render_sky_harmonics(const voe_render_sky *sky,
			      voe_math_float4 out[9]);

// The irradiance E(n) on a surface of unit `normal`, from those nine.
voe_math_float3 voe_render_sky_irradiance(const voe_math_float4 harmonics[9],
					  voe_math_float3 normal);
