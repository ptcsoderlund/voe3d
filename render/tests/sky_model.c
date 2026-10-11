// The sky model's claims (ADR-0405 points 3 and 5), with no graphics card: the
// radiance and its harmonics are plain arithmetic over one sun.
//
// A sun at elevation e has its light travelling along (−cos e, −sin e, 0), so
// it stands towards +X; a high sun is at 60°, so the zenith is not inside the
// Mie peak about it, and the evening sun at 5°.
//
// A HIGH SUN: THE ZENITH'S BLUE OVER RED ABOVE THE HORIZON'S. AN EVENING SUN:
// THE HORIZON TOWARDS IT REDDER THAN BLUE, THE SKY DARKER OVERALL THAN AT NOON.
// THE SUN'S OWN DIRECTION OVER TEN TIMES ONE 5° BESIDE IT. SUNLIGHT NOUGHT IS
// BLACK EVERYWHERE. STRAIGHT DOWN IS A TENTH OF THE HORIZON BELOW THE SUN.
//
// IRRADIANCE: up above down at noon, and within 5% of a cosine-weighted sum of
// the radiance over 4096 Fibonacci directions, for up at noon and under the
// high sun, and a horizontal normal towards that sun and one away; the sum
// skips the disc as the harmonics do. The check is on the channels summed:
// nine harmonics cannot hold the bright horizon band, and up reads red about 5%
// high under a high sun, more under a low one.
#include "../src/sky.h"

#include <testing/test.h>

#include <math.h>
#include <stdint.h>

#define PI 3.14159265358979f
#define BRUTE_DIRECTIONS 4096u

static const voe_math_float3 white = { 1.0f, 1.0f, 1.0f };
static const voe_math_float3 up = { 0.0f, 1.0f, 0.0f };

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

static voe_math_float3 sky(voe_math_float3 direction, voe_math_float3 view)
{
	return voe_render_sky_radiance(direction, white, view);
}

static voe_math_float3 fibonacci(uint32_t i)
{
	const float y = 1.0f - (2.0f * (float)i + 1.0f) / (float)BRUTE_DIRECTIONS;
	const float r = sqrtf(fmaxf(0.0f, 1.0f - y * y));
	const float phi = (float)i * PI * (3.0f - sqrtf(5.0f));

	return (voe_math_float3){ cosf(phi) * r, y, sinf(phi) * r };
}

// The cosine-weighted radiance on `normal` over 4096 directions, the disc left
// out; without `cosine`, the plain sum: the sky's mean times 4π.
static voe_math_float3 brute_irradiance(voe_math_float3 direction,
					voe_math_float3 normal, bool cosine)
{
	const float disc = cosf(VOE_RENDER_SKY_DISC_DEGREES * (PI / 180.0f));
	const voe_math_float3 sun = voe_math_float3_neg(direction);
	const float weight = 4.0f * PI / (float)BRUTE_DIRECTIONS;
	voe_math_float3 e = { 0.0f, 0.0f, 0.0f };

	for (uint32_t i = 0; i < BRUTE_DIRECTIONS; i++) {
		const voe_math_float3 v = fibonacci(i);
		const float c = cosine ? voe_math_float3_dot(v, normal) : 1.0f;

		if (c <= 0.0f || voe_math_float3_dot(v, sun) >= disc)
			continue;
		e = voe_math_float3_add(e, voe_math_float3_scale(sky(direction, v),
								 c * weight));
	}
	return e;
}

static void a_high_sun_is_bluer_at_the_zenith(void)
{
	const voe_math_float3 sun = sun_at(60.0f);
	const voe_math_float3 zenith = sky(sun, up);
	const voe_math_float3 horizon = sky(sun, toward(0.0f, 90.0f));

	VOE_TEST_CHECK(zenith.z / zenith.x > horizon.z / horizon.x);
	VOE_TEST_CHECK(zenith.z > zenith.x);
}

static void an_evening_sun_is_red_and_low(void)
{
	const voe_math_float3 evening = sun_at(5.0f);
	const voe_math_float3 horizon = sky(evening, toward(0.0f, 0.0f));
	const voe_math_float3 noon = sun_at(90.0f);

	VOE_TEST_CHECK(horizon.x > horizon.z);
	VOE_TEST_CHECK(sum(brute_irradiance(evening, up, false)) <
		       sum(brute_irradiance(noon, up, false)));
}

static void the_sun_outshines_its_neighbourhood(void)
{
	const voe_math_float3 sun = sun_at(40.0f);

	VOE_TEST_CHECK(sum(sky(sun, toward(40.0f, 0.0f))) >
		       10.0f * sum(sky(sun, toward(45.0f, 0.0f))));
}

static void no_sunlight_is_black(void)
{
	const voe_math_float3 black = { 0.0f, 0.0f, 0.0f };
	const voe_math_float3 sun = sun_at(30.0f);
	voe_math_float4 sh[9];
	bool dark = true;

	for (uint32_t i = 0; i < 64; i++) {
		const voe_math_float3 v = fibonacci(i * 64u);

		dark = dark && sum(voe_render_sky_radiance(sun, black, v)) == 0.0f;
	}
	dark = dark && sum(voe_render_sky_radiance(sun, black, toward(30.0f, 0.0f))) == 0.0f;
	voe_render_sky_harmonics(sun, black, sh);
	dark = dark && sum(voe_render_sky_irradiance(sh, up)) == 0.0f;
	VOE_TEST_CHECK(dark);
}

static void straight_down_is_a_tenth(void)
{
	const voe_math_float3 sun = sun_at(30.0f);
	const voe_math_float3 down = sky(sun, (voe_math_float3){ 0, -1, 0 });
	const voe_math_float3 horizon = sky(sun, toward(0.0f, 0.0f));

	VOE_TEST_CHECK_FLOAT(down.x, horizon.x * VOE_RENDER_SKY_GROUND, 1e-5f);
	VOE_TEST_CHECK_FLOAT(down.z, horizon.z * VOE_RENDER_SKY_GROUND, 1e-5f);
	VOE_TEST_CHECK(down.x > 0.0f);
}

static void check_irradiance(voe_math_float3 sun, voe_math_float3 normal)
{
	voe_math_float4 sh[9];

	voe_render_sky_harmonics(sun, white, sh);
	const voe_math_float3 e = voe_render_sky_irradiance(sh, normal);
	const voe_math_float3 b = brute_irradiance(sun, normal, true);

	VOE_TEST_CHECK_FLOAT(sum(e), sum(b), 0.05f * sum(b));
	VOE_TEST_CHECK(e.x > 0.0f && e.y > 0.0f && e.z > 0.0f);
}

static void irradiance_matches_the_sky(void)
{
	const voe_math_float3 noon = sun_at(90.0f);
	const voe_math_float3 high = sun_at(60.0f);
	voe_math_float4 sh[9];

	voe_render_sky_harmonics(noon, white, sh);
	VOE_TEST_CHECK(sum(voe_render_sky_irradiance(sh, up)) >
		       sum(voe_render_sky_irradiance(
			       sh, (voe_math_float3){ 0, -1, 0 })));
	check_irradiance(noon, up);
	check_irradiance(high, up);
	check_irradiance(high, (voe_math_float3){ 1, 0, 0 });
	check_irradiance(high, (voe_math_float3){ -1, 0, 0 });
}

int main(void)
{
	a_high_sun_is_bluer_at_the_zenith();
	an_evening_sun_is_red_and_low();
	the_sun_outshines_its_neighbourhood();
	no_sunlight_is_black();
	straight_down_is_a_tenth();
	irradiance_matches_the_sky();
	return voe_test_result();
}
