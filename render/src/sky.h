// The sky's radiance and the light it casts, worked out on the CPU (ADR-0405
// points 3 and 5). Pure CPU, no Vulkan: a camera pass projects the sky onto
// nine harmonics for the frame block, and the tests hold the model to its claims.
//
//     voe_math_float4 sh[9];
//     voe_render_sky_harmonics(light.direction, sunlight, sh);
//     voe_math_float3 e = voe_render_sky_irradiance(sh, normal);
//
// THE MODEL. Single scattering through a flat atmosphere. Air mass is
// Kasten–Young's of the angle above the horizon, clamped at the horizon. The
// sunlight reaching the air is the sun's × exp(−τ·m(sun)), τ the Rayleigh and
// Mie zenith depths summed per channel. A view direction gathers
// (1 − exp(−τ·m(view))) / τ × (τ_R·P_R + τ_M·P_M) × that sunlight × GAIN, the
// gain standing for the multiple scattering left out; P_R is Rayleigh's phase,
// P_M Henyey–Greenstein at g. Within the disc's angle of the sun a view adds
// that sunlight × DISC_GAIN. Below the horizon a view takes the horizon's
// colour at its own azimuth, darkened linearly in −y to GROUND straight down;
// straight down takes the horizon towards the sun.
//
// THE TWIN. `render/shaders/sky.slangh` is the same function in Slang; a change
// here is made there too, and the two agree within 2/255 as drawn.
//
// THE WORLD IS +Y UP, AS IT COMES. The harmonics are over world axes, the nine
// real coefficients in the order (l, m) = (0,0) (1,−1) (1,0) (1,1) (2,−2)
// (2,−1) (2,0) (2,1) (2,2), each a float4 of rgb and a nought.
//
// THE HARMONICS LEAVE THE DISC OUT. The sun's disc is the sun, which lights a
// surface on its own; and 256 directions would hit or miss a 0.5° disc by luck.
//
// CONSTRAINTS. The projection is 256 radiance evaluations per call, a few tens
// of microseconds; more directions would only follow the Mie peak closer.
#pragma once

#include <math/float3.h>
#include <math/float4.h>

// Zenith optical depths: Rayleigh per channel, Mie the same in all three.
#define VOE_RENDER_SKY_RAYLEIGH_R 0.0464f
#define VOE_RENDER_SKY_RAYLEIGH_G 0.108f
#define VOE_RENDER_SKY_RAYLEIGH_B 0.265f
#define VOE_RENDER_SKY_MIE 0.0252f
// Henyey–Greenstein's asymmetry for the Mie phase.
#define VOE_RENDER_SKY_MIE_G 0.76f
// The scattered light's multiplier, for the multiple scattering left out.
#define VOE_RENDER_SKY_GAIN 4.0f
// The sun's disc: its angular radius in degrees and its multiplier.
#define VOE_RENDER_SKY_DISC_DEGREES 0.5f
#define VOE_RENDER_SKY_DISC_GAIN 50.0f
// Straight down is this fraction of the horizon.
#define VOE_RENDER_SKY_GROUND 0.1f
// Fibonacci-sphere directions the harmonics are projected over.
#define VOE_RENDER_SKY_DIRECTIONS 256

// The radiance seen along unit `view` under a sun whose light travels along
// unit `direction` (the sun at −direction), `sunlight` its colour × intensity.
voe_math_float3 voe_render_sky_radiance(voe_math_float3 direction,
					voe_math_float3 sunlight,
					voe_math_float3 view);

// The sky without its disc over VOE_RENDER_SKY_DIRECTIONS directions, projected
// onto nine real harmonics and convolved with the cosine lobe into `out`.
void voe_render_sky_harmonics(voe_math_float3 direction,
			      voe_math_float3 sunlight, voe_math_float4 out[9]);

// The irradiance E(n) on a surface of unit `normal`, from those nine.
voe_math_float3 voe_render_sky_irradiance(const voe_math_float4 harmonics[9],
					  voe_math_float3 normal);
