// The sky's three calls: its radiance along a view, its projection onto nine
// harmonics, and the irradiance read back from them (see sky.h for the model).
//
// The scattered part and the disc are kept apart so the harmonics can take the
// first alone. The air mass is one number a direction, the extinction one a
// channel; every call is plain arithmetic, allocates nothing and keeps no state.
//
// CONSTRAINTS. The projection's 256 directions are worked out on every call
// rather than kept in a table; a table would lift that if a profile names it.
#include "sky.h"

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>

#define PI 3.14159265358979f

[[maybe_unused]] static float unit_error(voe_math_float3 v)
{
	return fabsf(voe_math_float3_length(v) - 1.0f);
}

// Kasten–Young's relative air mass at `height`, the sine of the angle above the
// horizon, clamped at the horizon.
static float air_mass(float height)
{
	const float s = fminf(fmaxf(height, 0.0f), 1.0f);
	const float degrees = asinf(s) * (180.0f / PI);
	const float m = 1.0f / (s + 0.50572f * powf(degrees + 6.07995f, -1.6364f));

	assert(m >= 1.0f - 1e-3f && m < 40.0f);
	return m;
}

static voe_math_float3 rayleigh_depth(void)
{
	return (voe_math_float3){ VOE_RENDER_SKY_RAYLEIGH_R,
				  VOE_RENDER_SKY_RAYLEIGH_G,
				  VOE_RENDER_SKY_RAYLEIGH_B };
}

static voe_math_float3 extinction(voe_math_float3 depth, float mass)
{
	return (voe_math_float3){ expf(-depth.x * mass), expf(-depth.y * mass),
				  expf(-depth.z * mass) };
}

// The sunlight that reaches the air, after its path through it.
static voe_math_float3 sunlight_in_air(voe_math_float3 sun,
				       voe_math_float3 sunlight)
{
	const voe_math_float3 rayleigh = rayleigh_depth();
	const voe_math_float3 depth = voe_math_float3_add(
		rayleigh, (voe_math_float3){ VOE_RENDER_SKY_MIE,
					     VOE_RENDER_SKY_MIE,
					     VOE_RENDER_SKY_MIE });

	return voe_math_float3_mul(sunlight,
				   extinction(depth, air_mass(sun.y)));
}

// The scattered light along `view` at or above the horizon, sun at `sun`.
static voe_math_float3 scattered_above(voe_math_float3 sun,
				       voe_math_float3 sunlight,
				       voe_math_float3 view)
{
	const float c = voe_math_float3_dot(view, sun);
	const float g = VOE_RENDER_SKY_MIE_G;
	const float p_r = 3.0f / (16.0f * PI) * (1.0f + c * c);
	const float p_m = (1.0f - g * g) /
			  (4.0f * PI * powf(1.0f + g * g - 2.0f * g * c, 1.5f));
	const voe_math_float3 rayleigh = rayleigh_depth();
	const float mie = VOE_RENDER_SKY_MIE;
	const voe_math_float3 depth = voe_math_float3_add(
		rayleigh, (voe_math_float3){ mie, mie, mie });
	const voe_math_float3 through = extinction(depth, air_mass(view.y));
	const voe_math_float3 gathered = voe_math_float3_div(
		voe_math_float3_sub((voe_math_float3){ 1, 1, 1 }, through), depth);
	const voe_math_float3 phase = voe_math_float3_add(
		voe_math_float3_scale(rayleigh, p_r),
		(voe_math_float3){ mie * p_m, mie * p_m, mie * p_m });
	const voe_math_float3 out = voe_math_float3_scale(
		voe_math_float3_mul(voe_math_float3_mul(gathered, phase),
				    sunlight_in_air(sun, sunlight)),
		VOE_RENDER_SKY_GAIN);

	assert(p_r > 0.0f && p_m > 0.0f);
	return out;
}

// The scattered light along any `view`: below the horizon, the horizon's at
// the view's azimuth (the sun's, straight down) darkened linearly in −y.
static voe_math_float3 scattered(voe_math_float3 sun, voe_math_float3 sunlight,
				 voe_math_float3 view)
{
	if (view.y >= 0.0f)
		return scattered_above(sun, sunlight, view);
	voe_math_float3 flat = { view.x, 0.0f, view.z };

	if (voe_math_float3_length(flat) < 1e-4f)
		flat = (voe_math_float3){ sun.x, 0.0f, sun.z };
	if (voe_math_float3_length(flat) < 1e-4f)
		flat = (voe_math_float3){ 1.0f, 0.0f, 0.0f };
	flat = voe_math_float3_normalize(flat);
	const float darken = 1.0f - (1.0f - VOE_RENDER_SKY_GROUND) * -view.y;

	assert(darken >= VOE_RENDER_SKY_GROUND - 1e-4f && darken <= 1.0f);
	return voe_math_float3_scale(scattered_above(sun, sunlight, flat),
				     darken);
}

voe_math_float3 voe_render_sky_radiance(voe_math_float3 direction,
					voe_math_float3 sunlight,
					voe_math_float3 view)
{
	assert(unit_error(direction) < 1e-3f && unit_error(view) < 1e-3f);
	assert(sunlight.x >= 0.0f && sunlight.y >= 0.0f && sunlight.z >= 0.0f);
	const voe_math_float3 sun = voe_math_float3_neg(direction);
	voe_math_float3 out = scattered(sun, sunlight, view);
	const float disc = cosf(VOE_RENDER_SKY_DISC_DEGREES * (PI / 180.0f));

	if (view.y >= 0.0f && voe_math_float3_dot(view, sun) >= disc)
		out = voe_math_float3_add(
			out, voe_math_float3_scale(sunlight_in_air(sun, sunlight),
						   VOE_RENDER_SKY_DISC_GAIN));
	assert(isfinite(out.x) && isfinite(out.y) && isfinite(out.z));
	return out;
}

// The nine real harmonics at unit `n`, in sky.h's order.
static void harmonics_at(voe_math_float3 n, float y[9])
{
	y[0] = 0.282095f;
	y[1] = 0.488603f * n.y;
	y[2] = 0.488603f * n.z;
	y[3] = 0.488603f * n.x;
	y[4] = 1.092548f * n.x * n.y;
	y[5] = 1.092548f * n.y * n.z;
	y[6] = 0.315392f * (3.0f * n.z * n.z - 1.0f);
	y[7] = 1.092548f * n.x * n.z;
	y[8] = 0.546274f * (n.x * n.x - n.y * n.y);
	assert(y[0] > 0.0f);
}

// Direction i of a Fibonacci sphere of `count`.
static voe_math_float3 fibonacci(uint32_t i, uint32_t count)
{
	assert(i < count);
	const float y = 1.0f - (2.0f * (float)i + 1.0f) / (float)count;
	const float r = sqrtf(fmaxf(0.0f, 1.0f - y * y));
	const float phi = (float)i * PI * (3.0f - sqrtf(5.0f));

	return (voe_math_float3){ cosf(phi) * r, y, sinf(phi) * r };
}

void voe_render_sky_harmonics(voe_math_float3 direction,
			      voe_math_float3 sunlight, voe_math_float4 out[9])
{
	assert(out != NULL && unit_error(direction) < 1e-3f);
	const voe_math_float3 sun = voe_math_float3_neg(direction);
	const uint32_t count = VOE_RENDER_SKY_DIRECTIONS;
	// Each sample's solid angle, times the cosine lobe's band: π, 2π/3, π/4.
	const float weight = 4.0f * PI / (float)count;
	const float band[9] = { PI, 2.0f * PI / 3.0f, 2.0f * PI / 3.0f,
				2.0f * PI / 3.0f, PI / 4.0f, PI / 4.0f,
				PI / 4.0f, PI / 4.0f, PI / 4.0f };

	for (uint32_t k = 0; k < 9; k++)
		out[k] = (voe_math_float4){ 0.0f, 0.0f, 0.0f, 0.0f };
	for (uint32_t i = 0; i < count; i++) {
		const voe_math_float3 v = fibonacci(i, count);
		const voe_math_float3 l = scattered(sun, sunlight, v);
		float y[9];

		harmonics_at(v, y);
		for (uint32_t k = 0; k < 9; k++) {
			const float s = y[k] * weight * band[k];

			out[k].x += l.x * s;
			out[k].y += l.y * s;
			out[k].z += l.z * s;
		}
	}
	assert(isfinite(out[0].x) && out[0].x >= 0.0f);
}

voe_math_float3 voe_render_sky_irradiance(const voe_math_float4 harmonics[9],
					  voe_math_float3 normal)
{
	assert(harmonics != NULL && unit_error(normal) < 1e-3f);
	float y[9];
	voe_math_float3 e = { 0.0f, 0.0f, 0.0f };

	harmonics_at(normal, y);
	for (uint32_t k = 0; k < 9; k++) {
		e.x += harmonics[k].x * y[k];
		e.y += harmonics[k].y * y[k];
		e.z += harmonics[k].z * y[k];
	}
	assert(isfinite(e.x) && isfinite(e.y) && isfinite(e.z));
	return e;
}
