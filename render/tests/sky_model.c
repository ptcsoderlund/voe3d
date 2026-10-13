// The sky model's claims (ADR-0411, ADR-0412 points 2 to 4), with no graphics
// card: the radiance and its harmonics are plain arithmetic over one light.
// Every case is the defaults of ADR-0406 point 1 unless it names a field,
// under a white sun 30° up unless it names a height.
//
// A sun at elevation e has its light travelling along (−cos e, −sin e, 0), so
// it stands towards +X.
//
// THE ENDPOINTS: straight up is `zenith`, the horizon facing away from the sun
// `horizon`, straight down `ground`, each within 1e-4; with a dark light the
// same, and no direction brighter than the brightest authored channel.
//
// HORIZON HEIGHT: at 0 a view 3° up within 0.1 of `zenith` in every channel; at
// 1 a view 30° up nearer `horizon` than `zenith`.
//
// THE SUN: its own direction brighter than any authored channel; sun size 0
// and glow 0, its direction reads the sky under a dark light.
//
// THE GRADIENT: up, the horizon away and the sun's own direction at
// most the radiance per channel, strictly less towards the sun; sun size and
// glow 0, the two equal.
//
// IRRADIANCE within 5% of a cosine-weighted sum of the backdrop over 4096
// Fibonacci directions, for up, down and two horizontal normals, on the
// channels summed; the sky's disc and glow sizes at nought the same harmonics
// byte for byte, the lights present making no difference.
//
// EVERY LIGHT'S DISC (ADR-0412 points 2 and 3), lights of disc 1.5 and glow 0.3,
// each within 1e-4 unless named:
// - strength 0.5 reads half of strength 1's straight up, at the horizon and
//   straight down, under an orange light; strength 0 reads nought there;
// - an orange light (1, 0.5, 0) 30° up reads (2, 1, 0) along its own direction
//   at strength 1 and 0, at intensity 1 and 50;
// - that light unshaded, of intensity 0, or of disc and glow 0: the radiance
//   along 4096 directions is the backdrop;
// - red 30° up to the north (−Z) and blue 20° up to the east (+X): each one's
//   own direction reads its own 2c, a view between them at least the backdrop;
// - the orange light's glow 10° from it is above the backdrop in red and
//   green at strength 1 and 0;
// - with no light the gradient is the radiance along 4096 directions.
#include "../src/sky.h"

#include <testing/test.h>

#include <math.h>
#include <stdint.h>
#include <string.h>

#define PI 3.14159265358979f
#define BRUTE_DIRECTIONS 4096u
#define EXACT 1e-4f

static const voe_math_float3 up = { 0.0f, 1.0f, 0.0f };
static const voe_math_float3 down = { 0.0f, -1.0f, 0.0f };

// ADR-0406 point 1's defaults.
static const voe_render_sky DEFAULTS = {
	.zenith = { 0.15f, 0.35f, 0.80f },
	.horizon_height = 0.25f,
	.horizon = { 0.60f, 0.75f, 0.90f },
	.strength = 1.0f,
	.ground = { 0.20f, 0.18f, 0.16f },
	.drawn = 1,
	.haze = 1000.0f,
	.disc_size = { 1.5f },
	.glow_size = { 0.5f },
};

static voe_math_float3 sun_at(float degrees)
{
	const float e = degrees * (PI / 180.0f);

	return (voe_math_float3){ -cosf(e), -sinf(e), 0.0f };
}

// A white light `degrees` up, or a dark one, its colour nought, when `dark`.
static voe_render_light light_at(float degrees, bool dark)
{
	const float c = dark ? 0.0f : 1.0f;

	return (voe_render_light){ .direction = sun_at(degrees),
				   .intensity = 1.0f,
				   .colour = { c, c, c } };
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
	const voe_render_light light = light_at(degrees, false);

	return voe_render_sky_radiance(sky, &light, 1, view);
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
	const voe_render_light dark = light_at(30.0f, true);
	const voe_math_float3 away = toward(0.0f, 180.0f);
	const float most = fmaxf(brightest(sky->zenith),
				 fmaxf(brightest(sky->horizon),
				       brightest(sky->ground)));

	check_colour(seen(sky, 30.0f, up), sky->zenith, EXACT);
	check_colour(seen(sky, 30.0f, away), sky->horizon, EXACT);
	check_colour(seen(sky, 30.0f, down), sky->ground, EXACT);
	check_colour(voe_render_sky_radiance(sky, &dark, 1, up), sky->zenith,
		     EXACT);
	check_colour(voe_render_sky_radiance(sky, &dark, 1, away),
		     sky->horizon, EXACT);
	check_colour(voe_render_sky_radiance(sky, &dark, 1, down), sky->ground,
		     EXACT);
	for (uint32_t i = 0; i < BRUTE_DIRECTIONS; i++)
		VOE_TEST_CHECK(brightest(voe_render_sky_radiance(
				       sky, &dark, 1, fibonacci(i))) <=
			       most + EXACT);
	VOE_TEST_CHECK(brightest(voe_render_sky_radiance(sky, &dark, 1,
							   toward(30.0f, 0.0f))) <=
		       most + EXACT);
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
	const voe_render_light dark = light_at(30.0f, true);
	const voe_math_float3 at_sun = voe_math_float3_neg(sun_at(30.0f));
	const float most = fmaxf(brightest(sky.zenith),
				 fmaxf(brightest(sky.horizon),
				       brightest(sky.ground)));
	const voe_math_float3 lit = seen(&sky, 30.0f, at_sun);

	VOE_TEST_CHECK(fminf(lit.x, fminf(lit.y, lit.z)) > most);
	sky.disc_size[0] = 0.0f;
	sky.glow_size[0] = 0.0f;
	check_colour(seen(&sky, 30.0f, at_sun),
		     voe_render_sky_radiance(&sky, &dark, 1, at_sun), EXACT);
}

static void the_gradient_leaves_the_sun_out(void)
{
	voe_render_sky sky = DEFAULTS;
	const voe_math_float3 sun = sun_at(30.0f);
	const voe_render_light light = light_at(30.0f, false);
	const voe_math_float3 views[3] = { up, toward(0.0f, 180.0f),
					   voe_math_float3_neg(sun) };

	for (int i = 0; i < 3; i++) {
		const voe_math_float3 g = voe_render_sky_gradient(&sky, views[i]);
		const voe_math_float3 l =
			voe_render_sky_radiance(&sky, &light, 1, views[i]);

		VOE_TEST_CHECK(g.x <= l.x && g.y <= l.y && g.z <= l.z);
		if (i == 2)
			VOE_TEST_CHECK(g.x < l.x && g.y < l.y && g.z < l.z);
	}
	sky.disc_size[0] = 0.0f;
	sky.glow_size[0] = 0.0f;
	for (int i = 0; i < 3; i++)
		check_colour(voe_render_sky_gradient(&sky, views[i]),
			     voe_render_sky_radiance(&sky, &light, 1, views[i]),
			     EXACT);
}

// The cosine-weighted backdrop on `normal` over 4096 directions.
static voe_math_float3 brute_irradiance(voe_math_float3 normal)
{
	const float weight = 4.0f * PI / (float)BRUTE_DIRECTIONS;
	voe_math_float3 e = { 0.0f, 0.0f, 0.0f };

	for (uint32_t i = 0; i < BRUTE_DIRECTIONS; i++) {
		const voe_math_float3 v = fibonacci(i);
		const float c = voe_math_float3_dot(v, normal);

		if (c <= 0.0f)
			continue;
		e = voe_math_float3_add(
			e, voe_math_float3_scale(
				   voe_render_sky_gradient(&DEFAULTS, v),
				   c * weight));
	}
	return e;
}

static void check_irradiance(voe_math_float3 normal)
{
	voe_math_float4 sh[9];

	voe_render_sky_harmonics(&DEFAULTS, sh);
	const voe_math_float3 e = voe_render_sky_irradiance(sh, normal);
	const voe_math_float3 b = brute_irradiance(normal);

	VOE_TEST_CHECK_FLOAT(sum(e), sum(b), 0.05f * sum(b));
	VOE_TEST_CHECK(e.x > 0.0f && e.y > 0.0f && e.z > 0.0f);
}

static void irradiance_matches_the_sky(void)
{
	voe_render_sky bare = DEFAULTS;
	voe_math_float4 with[9];
	voe_math_float4 without[9];

	check_irradiance(up);
	check_irradiance(down);
	check_irradiance((voe_math_float3){ 1, 0, 0 });
	check_irradiance((voe_math_float3){ -1, 0, 0 });
	memset(bare.disc_size, 0, sizeof(bare.disc_size));
	memset(bare.glow_size, 0, sizeof(bare.glow_size));
	voe_render_sky_harmonics(&DEFAULTS, with);
	voe_render_sky_harmonics(&bare, without);
	VOE_TEST_CHECK(memcmp(with, without, sizeof(with)) == 0);
}

static const voe_math_float3 ORANGE = { 1.0f, 0.5f, 0.0f };
static const voe_math_float3 RED = { 1.0f, 0.0f, 0.0f };
static const voe_math_float3 BLUE = { 0.0f, 0.0f, 1.0f };
static const voe_math_float3 NOUGHT = { 0.0f, 0.0f, 0.0f };

// The defaults at `strength`, lights 0 and 1 of disc 1.5 and glow 0.3.
static voe_render_sky lit_sky(float strength)
{
	voe_render_sky sky = DEFAULTS;

	sky.strength = strength;
	for (int i = 0; i < 2; i++) {
		sky.disc_size[i] = 1.5f;
		sky.glow_size[i] = 0.3f;
	}
	return sky;
}

// A light of `colour` standing `degrees` up at `azimuth` from +X.
static voe_render_light light_toward(voe_math_float3 colour, float degrees,
				     float azimuth)
{
	return (voe_render_light){
		.direction = voe_math_float3_neg(toward(degrees, azimuth)),
		.intensity = 1.0f,
		.colour = colour,
	};
}

static void strength_scales_the_backdrop(void)
{
	const voe_render_sky full = lit_sky(1.0f);
	const voe_render_sky half = lit_sky(0.5f);
	const voe_render_sky none = lit_sky(0.0f);
	const voe_render_light orange = light_toward(ORANGE, 30.0f, 0.0f);
	const voe_math_float3 views[3] = { up, toward(0.0f, 180.0f), down };

	for (int i = 0; i < 3; i++) {
		const voe_math_float3 f =
			voe_render_sky_radiance(&full, &orange, 1, views[i]);

		check_colour(voe_render_sky_radiance(&half, &orange, 1, views[i]),
			     voe_math_float3_scale(f, 0.5f), EXACT);
		check_colour(voe_render_sky_radiance(&none, &orange, 1, views[i]),
			     NOUGHT, EXACT);
	}
}

static void a_disc_is_twice_its_colour(void)
{
	const voe_math_float3 at = toward(30.0f, 0.0f);
	const voe_math_float3 twice =
		voe_math_float3_scale(ORANGE, VOE_RENDER_SKY_DISC_GAIN);
	const float strengths[2] = { 1.0f, 0.0f };
	const float intensities[2] = { 1.0f, 50.0f };

	for (int s = 0; s < 2; s++) {
		const voe_render_sky sky = lit_sky(strengths[s]);

		for (int i = 0; i < 2; i++) {
			voe_render_light orange = light_toward(ORANGE, 30.0f, 0.0f);

			orange.intensity = intensities[i];
			check_colour(voe_render_sky_radiance(&sky, &orange, 1, at),
				     twice, EXACT);
		}
	}
}

static void a_light_not_shown_leaves_the_backdrop(void)
{
	const voe_render_sky sky = lit_sky(1.0f);
	const voe_render_light orange = light_toward(ORANGE, 30.0f, 0.0f);
	voe_render_sky bare = sky;
	voe_render_light unshaded = orange;
	voe_render_light dim = orange;

	bare.disc_size[0] = 0.0f;
	bare.glow_size[0] = 0.0f;
	unshaded.unshaded = 1;
	dim.intensity = 0.0f;
	for (uint32_t i = 0; i < BRUTE_DIRECTIONS; i++) {
		const voe_math_float3 v = fibonacci(i);
		const voe_math_float3 b = voe_render_sky_gradient(&sky, v);

		check_colour(voe_render_sky_radiance(&sky, &unshaded, 1, v), b,
			     EXACT);
		check_colour(voe_render_sky_radiance(&sky, &dim, 1, v), b, EXACT);
		check_colour(voe_render_sky_radiance(&bare, &orange, 1, v), b,
			     EXACT);
	}
}

static void each_light_draws_its_own_disc(void)
{
	const voe_render_sky sky = lit_sky(1.0f);
	const voe_render_light lights[2] = {
		light_toward(RED, 30.0f, -90.0f),
		light_toward(BLUE, 20.0f, 0.0f),
	};
	const voe_math_float3 between = voe_math_float3_normalize(
		voe_math_float3_add(toward(30.0f, -90.0f), toward(20.0f, 0.0f)));
	const voe_math_float3 l =
		voe_render_sky_radiance(&sky, lights, 2, between);
	const voe_math_float3 b = voe_render_sky_gradient(&sky, between);

	for (int i = 0; i < 2; i++)
		check_colour(voe_render_sky_radiance(
				     &sky, lights, 2,
				     voe_math_float3_neg(lights[i].direction)),
			     voe_math_float3_scale(lights[i].colour,
						   VOE_RENDER_SKY_DISC_GAIN),
			     EXACT);
	VOE_TEST_CHECK(l.x >= b.x && l.y >= b.y && l.z >= b.z);
}

static void a_glow_shows_in_a_black_sky(void)
{
	const voe_render_light orange = light_toward(ORANGE, 30.0f, 0.0f);
	const voe_math_float3 aside = toward(40.0f, 0.0f);
	const float strengths[2] = { 1.0f, 0.0f };

	for (int s = 0; s < 2; s++) {
		const voe_render_sky sky = lit_sky(strengths[s]);
		const voe_math_float3 l =
			voe_render_sky_radiance(&sky, &orange, 1, aside);
		const voe_math_float3 b = voe_render_sky_gradient(&sky, aside);

		VOE_TEST_CHECK(l.x > b.x && l.y > b.y);
	}
}

static void no_light_is_the_gradient(void)
{
	const voe_render_sky sky = lit_sky(1.0f);

	for (uint32_t i = 0; i < BRUTE_DIRECTIONS; i++)
		check_colour(voe_render_sky_gradient(&sky, fibonacci(i)),
			     voe_render_sky_radiance(&sky, NULL, 0, fibonacci(i)),
			     EXACT);
}

int main(void)
{
	the_endpoints_are_authored();
	horizon_height_moves_the_band();
	the_sun_outshines_the_sky();
	the_gradient_leaves_the_sun_out();
	irradiance_matches_the_sky();
	strength_scales_the_backdrop();
	a_disc_is_twice_its_colour();
	a_light_not_shown_leaves_the_backdrop();
	each_light_draws_its_own_disc();
	a_glow_shows_in_a_black_sky();
	no_light_is_the_gradient();
	return voe_test_result();
}
