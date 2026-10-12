// The sky's four calls: its sun, its radiance along a view, its projection onto
// nine harmonics, and the irradiance read back from them (see sky.h for the
// model, ADR-0407 for the curves).
//
// The disc is a flag of the one radiance function so the harmonics can leave it
// out; every call is plain arithmetic, allocates nothing and keeps no state.
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

static float saturate(float x)
{
	return fminf(fmaxf(x, 0.0f), 1.0f);
}

// Slang's smoothstep: 0 at `low`, 1 at `high`, Hermite between.
static float smoothstep(float low, float high, float x)
{
	const float t = saturate((x - low) / (high - low));

	assert(high > low);
	return t * t * (3.0f - 2.0f * t);
}

static bool lit(voe_math_float3 sun)
{
	return sun.x > 0.0f || sun.y > 0.0f || sun.z > 0.0f;
}

voe_math_float3 voe_render_sky_sun(voe_render_light light)
{
	const voe_math_float3 nought = { 0.0f, 0.0f, 0.0f };
	const bool on = light.unshaded == 0 && light.intensity > 0.0f &&
			lit(light.colour) &&
			voe_math_float3_dot(light.direction, light.direction) > 0.0f;

	assert(isfinite(light.colour.x) && isfinite(light.colour.y) &&
	       isfinite(light.colour.z));
	return on ? light.colour : nought;
}

// The radiance along `view`, the disc added when `disc` (ADR-0407 points 3 to
// 5); `sun` nought reads neither `direction` nor the evening.
static voe_math_float3 radiance(const voe_render_sky *sky,
				voe_math_float3 direction, voe_math_float3 sun,
				voe_math_float3 view, bool disc)
{
	const bool on = lit(sun);
	const voe_math_float3 s =
		on ? voe_math_float3_neg(voe_math_float3_normalize(direction)) :
		     (voe_math_float3){ 0.0f, 1.0f, 0.0f };
	const float e = s.y;
	const float h = view.y;
	const float toward = voe_math_float3_dot(view, s);
	const float k = on ? sky->sunset *
				     saturate((VOE_RENDER_SKY_EVENING_HEIGHT - e) /
					      VOE_RENDER_SKY_EVENING_HEIGHT) :
			     0.0f;
	const float n = on ? sky->sunset * saturate(-e / VOE_RENDER_SKY_NIGHT_DEPTH) :
			     0.0f;
	const float side = 0.5f + 0.5f * toward;
	const float brightest = fmaxf(sky->horizon.x,
				      fmaxf(sky->horizon.y, sky->horizon.z));
	const voe_math_float3 warm = { VOE_RENDER_SKY_WARM_R * brightest,
				       VOE_RENDER_SKY_WARM_G * brightest,
				       VOE_RENDER_SKY_WARM_B * brightest };
	const voe_math_float3 horizon =
		voe_math_float3_lerp(sky->horizon, warm, k * side * side);
	// 0406's endpoints with one pow: 1/16 a hard band, 4 + 1/16 nearly flat.
	voe_math_float3 out =
		h >= 0.0f ?
			voe_math_float3_lerp(horizon, sky->zenith,
					     powf(h, 4.0f * sky->horizon_height +
							     1.0f / 16.0f)) :
			voe_math_float3_lerp(horizon, sky->ground,
					     saturate(-VOE_RENDER_SKY_GROUND_FADE * h));

	out = voe_math_float3_scale(out, (1.0f - VOE_RENDER_SKY_EVENING_DIM * k) *
						 (1.0f - VOE_RENDER_SKY_NIGHT_DIM * n));
	if (!on)
		return out;
	const float glow =
		sky->sun_glow *
		powf(saturate(toward),
		     VOE_RENDER_SKY_GLOW_NARROW -
			     (VOE_RENDER_SKY_GLOW_NARROW - VOE_RENDER_SKY_GLOW_WIDE) *
				     sky->sun_glow) *
		(1.0f - n);

	out = voe_math_float3_add(out, voe_math_float3_scale(sun, glow));
	if (disc && sky->sun_size > 0.0f && h >= 0.0f) {
		const float r = sky->sun_size * (PI / 180.0f);
		const float edge = smoothstep(cosf(VOE_RENDER_SKY_DISC_EDGE * r),
					      cosf(r), toward);

		out = voe_math_float3_add(
			out, voe_math_float3_scale(sun, VOE_RENDER_SKY_DISC_GAIN *
								edge * (1.0f - n)));
	}
	return out;
}

voe_math_float3 voe_render_sky_radiance(const voe_render_sky *sky,
					voe_math_float3 direction,
					voe_math_float3 sun,
					voe_math_float3 view)
{
	assert(sky != NULL && unit_error(view) < 1e-3f);
	assert(sun.x >= 0.0f && sun.y >= 0.0f && sun.z >= 0.0f);
	const voe_math_float3 out = radiance(sky, direction, sun, view, true);

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

void voe_render_sky_harmonics(const voe_render_sky *sky,
			      voe_math_float3 direction, voe_math_float3 sun,
			      voe_math_float4 out[9])
{
	assert(sky != NULL && out != NULL);
	assert(sun.x >= 0.0f && sun.y >= 0.0f && sun.z >= 0.0f);
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
		const voe_math_float3 l = radiance(sky, direction, sun, v, false);
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
