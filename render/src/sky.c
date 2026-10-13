// The sky's five calls: whether a light is shown, the backdrop and the radiance
// along a view, the backdrop's projection onto nine harmonics, and the
// irradiance read back from them (see sky.h for the model, ADR-0412 for the
// curves).
//
// The radiance is the backdrop, then every shown light's glow, then their discs
// laid over in light order; the harmonics read the backdrop alone. Every call
// is plain arithmetic, allocates nothing and keeps no state.
//
// CONSTRAINTS. The projection's 256 directions are worked out on every call
// rather than kept in a table; a table would lift that if a profile names it.
#include "sky.h"

#include <assert.h>
#include <math.h>
#include <stddef.h>

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

bool voe_render_sky_shown(voe_render_light light)
{
	assert(isfinite(light.colour.x) && isfinite(light.colour.y) &&
	       isfinite(light.colour.z));
	return light.unshaded == 0 && light.intensity > 0.0f &&
	       (light.colour.x > 0.0f || light.colour.y > 0.0f ||
		light.colour.z > 0.0f);
}

// G(view) of ADR-0407 point 4 times `strength` (ADR-0412 point 2).
static voe_math_float3 backdrop(const voe_render_sky *sky, voe_math_float3 view)
{
	const float h = view.y;
	// 0406's endpoints with one pow: 1/16 a hard band, 4 + 1/16 nearly flat.
	const voe_math_float3 out =
		h >= 0.0f ?
			voe_math_float3_lerp(sky->horizon, sky->zenith,
					     powf(h, 4.0f * sky->horizon_height +
							     1.0f / 16.0f)) :
			voe_math_float3_lerp(sky->horizon, sky->ground,
					     saturate(-VOE_RENDER_SKY_GROUND_FADE * h));

	return voe_math_float3_scale(out, sky->strength);
}

voe_math_float3 voe_render_sky_gradient(const voe_render_sky *sky,
					voe_math_float3 view)
{
	assert(sky != NULL && unit_error(view) < 1e-3f);
	const voe_math_float3 out = backdrop(sky, view);

	assert(isfinite(out.x) && isfinite(out.y) && isfinite(out.z));
	return out;
}

// The cosine between `view` and the place `light` stands, at −direction.
static float toward(voe_render_light light, voe_math_float3 view)
{
	return -voe_math_float3_dot(
		view, voe_math_float3_normalize(light.direction));
}

// ADR-0412 point 3: the backdrop, each shown light's glow in its colour, then
// each one's disc laid over by its edge, a lerp to DISC_GAIN × its colour.
voe_math_float3 voe_render_sky_radiance(const voe_render_sky *sky,
					const voe_render_light *lights,
					uint32_t count, voe_math_float3 view)
{
	assert(sky != NULL && unit_error(view) < 1e-3f);
	assert(count <= VOE_RENDER_DIRECTIONAL_LIGHTS &&
	       (lights != NULL || count == 0));
	voe_math_float3 out = backdrop(sky, view);

	for (uint32_t i = 0; i < count; i++) {
		const float g = sky->glow_size[i];

		if (!voe_render_sky_shown(lights[i]))
			continue;
		out = voe_math_float3_add(
			out, voe_math_float3_scale(
				     lights[i].colour,
				     g * powf(saturate(toward(lights[i], view)),
					      VOE_RENDER_SKY_GLOW_NARROW -
						      (VOE_RENDER_SKY_GLOW_NARROW -
						       VOE_RENDER_SKY_GLOW_WIDE) *
							      g)));
	}
	for (uint32_t i = 0; i < count; i++) {
		const float r = sky->disc_size[i] * (PI / 180.0f);

		if (!voe_render_sky_shown(lights[i]) || r <= 0.0f || view.y < 0.0f)
			continue;
		out = voe_math_float3_lerp(
			out,
			voe_math_float3_scale(lights[i].colour,
					      VOE_RENDER_SKY_DISC_GAIN),
			smoothstep(cosf(VOE_RENDER_SKY_DISC_EDGE * r), cosf(r),
				   toward(lights[i], view)));
	}
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
			      voe_math_float4 out[9])
{
	assert(sky != NULL && out != NULL);
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
		const voe_math_float3 l = backdrop(sky, v);
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
