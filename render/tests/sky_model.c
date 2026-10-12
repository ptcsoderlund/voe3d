// The sky model's claims (ADR-0406 point 2, ADR-0407 points 2 to 6), with no
// graphics card: the radiance and its harmonics are plain arithmetic over one
// sun. Every case is the defaults of ADR-0406 point 1 unless it names a field,
// under a white sun 30° up unless it names a height.
//
// A sun at elevation e has its light travelling along (−cos e, −sin e, 0), so
// it stands towards +X.
//
// THE ENDPOINTS: straight up is `zenith`, the horizon facing away from the sun
// `horizon`, straight down `ground`, each within 1e-4; with no sun the same,
// and no direction brighter than the brightest authored channel.
//
// SUNSET: at 0 a level view 90° round from the sun reads the same with the sun
// 2° up as 60° up. At 1 with the sun 2° up, the horizon towards it is r > g > b
// and no channel above `horizon`'s brightest, glow 0 so the gradient is what
// is read; the zenith darker than at 30°. At 1 with the sun 15° below, the
// zenith under a fifth of `zenith`.
//
// HORIZON HEIGHT: at 0 a view 3° up within 0.1 of `zenith` in every channel; at
// 1 a view 30° up nearer `horizon` than `zenith`.
//
// THE SUN: its own direction brighter than any authored channel; sun size 0
// and glow 0, its direction reads the sky with no sun.
//
// IRRADIANCE within 5% of a cosine-weighted sum of the radiance over 4096
// Fibonacci directions without the disc (sun size 0), for up, down and two
// horizontal normals, on the channels summed.
#include "../src/sky.h"

#include <testing/test.h>

#include <math.h>
#include <stdint.h>

#define PI 3.14159265358979f
#define BRUTE_DIRECTIONS 4096u
#define EXACT 1e-4f

static const voe_math_float3 white = { 1.0f, 1.0f, 1.0f };
static const voe_math_float3 none = { 0.0f, 0.0f, 0.0f };
static const voe_math_float3 up = { 0.0f, 1.0f, 0.0f };
static const voe_math_float3 down = { 0.0f, -1.0f, 0.0f };

// ADR-0406 point 1's defaults.
static const voe_render_sky DEFAULTS = {
	.zenith = { 0.15f, 0.35f, 0.80f },
	.horizon_height = 0.25f,
	.horizon = { 0.60f, 0.75f, 0.90f },
	.sun_size = 1.5f,
	.ground = { 0.20f, 0.18f, 0.16f },
	.sun_glow = 0.5f,
	.drawn = 1,
	.haze = 1000.0f,
	.sunset = 0.5f,
};

static voe_math_float3 sun_at(float degrees)
{
	const float e = degrees * (PI / 180.0f);

	return (voe_math_float3){ -cosf(e), -sinf(e), 0.0f };
}

// The unit direction at `degrees` above the horizon, `azimuth` from +X.
static voe_math_float3 toward(float degrees, float azimuth)
{
	const float e = degrees * (PI / 180.0f);
	const float a = azimuth * (PI / 180.0f);

	return (voe_math_float3){ cosf(e) * cosf(a), sinf(e), cosf(e) * sinf(a) };
}

static float sum(voe_math_float3 c)
{
	return c.x + c.y + c.z;
}

static float brightest(voe_math_float3 c)
{
	return fmaxf(c.x, fmaxf(c.y, c.z));
}

static float distance(voe_math_float3 a, voe_math_float3 b)
{
	return voe_math_float3_length(voe_math_float3_sub(a, b));
}

// `sky` along `view` under a white sun `degrees` up.
static voe_math_float3 seen(const voe_render_sky *sky, float degrees,
			    voe_math_float3 view)
{
	return voe_render_sky_radiance(sky, sun_at(degrees), white, view);
}

static void check_colour(voe_math_float3 got, voe_math_float3 want,
			 float tolerance)
{
	VOE_TEST_CHECK_FLOAT(got.x, want.x, tolerance);
	VOE_TEST_CHECK_FLOAT(got.y, want.y, tolerance);
	VOE_TEST_CHECK_FLOAT(got.z, want.z, tolerance);
}

static voe_math_float3 fibonacci(uint32_t i)
{
	const float y = 1.0f - (2.0f * (float)i + 1.0f) / (float)BRUTE_DIRECTIONS;
	const float r = sqrtf(fmaxf(0.0f, 1.0f - y * y));
	const float phi = (float)i * PI * (3.0f - sqrtf(5.0f));

	return (voe_math_float3){ cosf(phi) * r, y, sinf(phi) * r };
}

static void the_endpoints_are_authored(void)
{
	const voe_render_sky *sky = &DEFAULTS;
	const voe_math_float3 sun = sun_at(30.0f);
	const voe_math_float3 away = toward(0.0f, 180.0f);
	const float most = fmaxf(brightest(sky->zenith),
				 fmaxf(brightest(sky->horizon),
				       brightest(sky->ground)));

	check_colour(seen(sky, 30.0f, up), sky->zenith, EXACT);
	check_colour(seen(sky, 30.0f, away), sky->horizon, EXACT);
	check_colour(seen(sky, 30.0f, down), sky->ground, EXACT);
	check_colour(voe_render_sky_radiance(sky, sun, none, up), sky->zenith,
		     EXACT);
	check_colour(voe_render_sky_radiance(sky, sun, none, away),
		     sky->horizon, EXACT);
	check_colour(voe_render_sky_radiance(sky, sun, none, down), sky->ground,
		     EXACT);
	for (uint32_t i = 0; i < BRUTE_DIRECTIONS; i++)
		VOE_TEST_CHECK(brightest(voe_render_sky_radiance(
				       sky, sun, none, fibonacci(i))) <=
			       most + EXACT);
	VOE_TEST_CHECK(brightest(voe_render_sky_radiance(sky, sun, none,
							   toward(30.0f, 0.0f))) <=
		       most + EXACT);
}

static void sunset_turns_the_evening(void)
{
	voe_render_sky sky = DEFAULTS;
	const voe_math_float3 round = toward(0.0f, 90.0f);

	sky.sunset = 0.0f;
	check_colour(seen(&sky, 2.0f, round), seen(&sky, 60.0f, round), EXACT);
	sky.sunset = 1.0f;
	VOE_TEST_CHECK(sum(seen(&sky, 2.0f, up)) < sum(seen(&sky, 30.0f, up)));
	VOE_TEST_CHECK(sum(seen(&sky, -15.0f, up)) < 0.2f * sum(sky.zenith));
	VOE_TEST_CHECK(brightest(seen(&sky, -15.0f, up)) <
		       0.2f * brightest(sky.zenith));
	sky.sun_glow = 0.0f;
	const voe_math_float3 evening = seen(&sky, 2.0f, toward(0.0f, 0.0f));

	VOE_TEST_CHECK(evening.x > evening.y && evening.y > evening.z);
	VOE_TEST_CHECK(brightest(evening) <= brightest(sky.horizon) + EXACT);
}

static void horizon_height_moves_the_band(void)
{
	voe_render_sky sky = DEFAULTS;
	const voe_math_float3 high = toward(30.0f, 180.0f);

	sky.horizon_height = 0.0f;
	check_colour(seen(&sky, 30.0f, toward(3.0f, 180.0f)), sky.zenith, 0.1f);
	sky.horizon_height = 1.0f;
	VOE_TEST_CHECK(distance(seen(&sky, 30.0f, high), sky.horizon) <
		       distance(seen(&sky, 30.0f, high), sky.zenith));
}

static void the_sun_outshines_the_sky(void)
{
	voe_render_sky sky = DEFAULTS;
	const voe_math_float3 sun = sun_at(30.0f);
	const voe_math_float3 at_sun = voe_math_float3_neg(sun);
	const float most = fmaxf(brightest(sky.zenith),
				 fmaxf(brightest(sky.horizon),
				       brightest(sky.ground)));
	const voe_math_float3 lit = seen(&sky, 30.0f, at_sun);

	VOE_TEST_CHECK(fminf(lit.x, fminf(lit.y, lit.z)) > most);
	sky.sun_size = 0.0f;
	sky.sun_glow = 0.0f;
	check_colour(seen(&sky, 30.0f, at_sun),
		     voe_render_sky_radiance(&sky, sun, none, at_sun), EXACT);
}

// The cosine-weighted radiance on `normal` over 4096 directions, the disc left
// out.
static voe_math_float3 brute_irradiance(voe_math_float3 direction,
					voe_math_float3 normal)
{
	voe_render_sky sky = DEFAULTS;
	const float weight = 4.0f * PI / (float)BRUTE_DIRECTIONS;
	voe_math_float3 e = { 0.0f, 0.0f, 0.0f };

	sky.sun_size = 0.0f;
	for (uint32_t i = 0; i < BRUTE_DIRECTIONS; i++) {
		const voe_math_float3 v = fibonacci(i);
		const float c = voe_math_float3_dot(v, normal);

		if (c <= 0.0f)
			continue;
		e = voe_math_float3_add(
			e, voe_math_float3_scale(voe_render_sky_radiance(
							 &sky, direction, white, v),
						 c * weight));
	}
	return e;
}

static void check_irradiance(voe_math_float3 sun, voe_math_float3 normal)
{
	voe_math_float4 sh[9];

	voe_render_sky_harmonics(&DEFAULTS, sun, white, sh);
	const voe_math_float3 e = voe_render_sky_irradiance(sh, normal);
	const voe_math_float3 b = brute_irradiance(sun, normal);

	VOE_TEST_CHECK_FLOAT(sum(e), sum(b), 0.05f * sum(b));
	VOE_TEST_CHECK(e.x > 0.0f && e.y > 0.0f && e.z > 0.0f);
}

static void irradiance_matches_the_sky(void)
{
	const voe_math_float3 sun = sun_at(30.0f);

	check_irradiance(sun, up);
	check_irradiance(sun, down);
	check_irradiance(sun, (voe_math_float3){ 1, 0, 0 });
	check_irradiance(sun, (voe_math_float3){ -1, 0, 0 });
}

int main(void)
{
	the_endpoints_are_authored();
	sunset_turns_the_evening();
	horizon_height_moves_the_band();
	the_sun_outshines_the_sky();
	irradiance_matches_the_sky();
	return voe_test_result();
}
