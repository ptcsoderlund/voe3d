// THE SKY LIGHTS NOTHING; ONLY A METAL MIRRORS IT (ADR-0411 point 1, 0412
// point 5). A target of the test's own seen by a camera at the origin looking
// along −Z, read back with voe_render_target_read, the sky drawn at the test's
// values with haze 0, exposure 0 at Clip, light 0's fill nought or a red
// (0.5, 0, 0):
//
// 1. a grey lit cube 3 m ahead, its +Z face read at the centre, the sun 50° up
//    behind it: byte for byte the same with the sky drawn and zeroed, at fill
//    nought and at the red fill;
// 2. that face reads the same bytes whatever the sky's zenith, blue or red;
// 3. a smooth white metal quad tilted 30° up to the camera reads the sRGB of
//    voe_render_sky_radiance (render/src/sky.h) along the reflected view times
//    Karis's Fresnel fit, within 6/255; at roughness 1 it reads nearer E(R)/π,
//    from voe_render_sky_harmonics, than the sky along R;
// 4. that quad as a non-metal, metallic 0 and white, in the low sun: byte for
//    byte the same with the sky zeroed;
// 5. the metal quad inside an Indoors blocker: byte for byte as with the sky
//    zeroed.
//
// A MACHINE WITH NO USABLE VULKAN SKIPS AND SAYS SO. The skip is a pass and it
// prints its reason.
#include "../src/sky.h"

#include <render/device.h>

#include <base/arena.h>
#include <base/error.h>
#include <math/float4x4.h>
#include <platform/window.h>

#include <testing/test.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIDE 64
#define CENTRE (SIDE / 2)
#define MIRROR_TOLERANCE 6
#define PI 3.14159265358979f

#define FIELD_OF_VIEW 1.0471976f
#define NEAR_PLANE 0.1f
#define FAR_PLANE 100.0f
#define CUBE_Z -3.0f
#define QUAD_Z -2.0f
#define INTENSITY 1.0f
// The quad's normal, 30° up from facing the eye.
#define TILT_SIN 0.5f
#define TILT_COS 0.8660254f
#define RED_FILL ((voe_math_float3){ 0.5f, 0.0f, 0.0f })

static const voe_render_capacities CAPACITIES = {
	.vertices = 24 + 4,
	.indices = 36 + 6,
	.geometries = 2,
	.objects = 1,
	.shadings = 4,
	.passes = 1,
	.targets = 1,
};

// Counter-clockwise from outside, four vertices a face, unshaded.c's cube.
#define H 0.5f
static const voe_render_vertex CUBE_VERTICES[24] = {
	{ { -H, H, H }, { 0, 0, 1 }, { 0, 0 } },
	{ { H, H, H }, { 0, 0, 1 }, { 0, 0 } },
	{ { H, -H, H }, { 0, 0, 1 }, { 0, 0 } },
	{ { -H, -H, H }, { 0, 0, 1 }, { 0, 0 } },
	{ { H, H, -H }, { 0, 0, -1 }, { 0, 0 } },
	{ { -H, H, -H }, { 0, 0, -1 }, { 0, 0 } },
	{ { -H, -H, -H }, { 0, 0, -1 }, { 0, 0 } },
	{ { H, -H, -H }, { 0, 0, -1 }, { 0, 0 } },
	{ { H, H, H }, { 1, 0, 0 }, { 0, 0 } },
	{ { H, H, -H }, { 1, 0, 0 }, { 0, 0 } },
	{ { H, -H, -H }, { 1, 0, 0 }, { 0, 0 } },
	{ { H, -H, H }, { 1, 0, 0 }, { 0, 0 } },
	{ { -H, H, -H }, { -1, 0, 0 }, { 0, 0 } },
	{ { -H, H, H }, { -1, 0, 0 }, { 0, 0 } },
	{ { -H, -H, H }, { -1, 0, 0 }, { 0, 0 } },
	{ { -H, -H, -H }, { -1, 0, 0 }, { 0, 0 } },
	{ { -H, H, -H }, { 0, 1, 0 }, { 0, 0 } },
	{ { H, H, -H }, { 0, 1, 0 }, { 0, 0 } },
	{ { H, H, H }, { 0, 1, 0 }, { 0, 0 } },
	{ { -H, H, H }, { 0, 1, 0 }, { 0, 0 } },
	{ { -H, -H, H }, { 0, -1, 0 }, { 0, 0 } },
	{ { H, -H, H }, { 0, -1, 0 }, { 0, 0 } },
	{ { H, -H, -H }, { 0, -1, 0 }, { 0, 0 } },
	{ { -H, -H, -H }, { 0, -1, 0 }, { 0, 0 } },
};

static const uint32_t CUBE_INDICES[36] = {
	3, 2, 1, 3, 1, 0,	 7, 6, 5, 7, 5, 4,
	11, 10, 9, 11, 9, 8,	 15, 14, 13, 15, 13, 12,
	19, 18, 17, 19, 17, 16,	 23, 22, 21, 23, 21, 20,
};

// A metre square about (0, 0, −2), its normal (0, sin, cos) of the tilt, its
// edges +X and (0, cos, −sin): counter-clockwise seen along that normal.
static const voe_render_vertex QUAD_VERTICES[4] = {
	{ { -H, -H * TILT_COS, QUAD_Z + H * TILT_SIN }, { 0, TILT_SIN, TILT_COS }, { 0, 0 } },
	{ { H, -H * TILT_COS, QUAD_Z + H * TILT_SIN }, { 0, TILT_SIN, TILT_COS }, { 0, 0 } },
	{ { H, H * TILT_COS, QUAD_Z - H * TILT_SIN }, { 0, TILT_SIN, TILT_COS }, { 0, 0 } },
	{ { -H, H * TILT_COS, QUAD_Z - H * TILT_SIN }, { 0, TILT_SIN, TILT_COS }, { 0, 0 } },
};

static const uint32_t QUAD_INDICES[6] = { 0, 1, 2, 0, 2, 3 };

struct scene {
	voe_render_device *device;
	voe_base_arena *arena;
	voe_render_target target;
	voe_render_geometry cube;
	voe_render_geometry quad;
	voe_render_shading grey;
	voe_render_shading smooth;
	voe_render_shading rough;
	voe_render_shading plain;
};

// The eye at the origin looking along −Z, +Y up, reverse-Z as sky.c's.
static voe_render_view the_camera(void)
{
	voe_render_view view = { .view = voe_math_float4x4_identity() };
	float focal = 1.0f / tanf(FIELD_OF_VIEW * 0.5f);
	float span = FAR_PLANE - NEAR_PLANE;

	view.projection.m[0][0] = focal;
	view.projection.m[1][1] = focal;
	view.projection.m[2][2] = NEAR_PLANE / span;
	view.projection.m[2][3] = NEAR_PLANE * FAR_PLANE / span;
	view.projection.m[3][2] = -1.0f;
	return view;
}

// ADR-0406 point 1's sky, drawn, at haze 0.
static const voe_render_sky SKY = {
	.zenith = { 0.15f, 0.35f, 0.80f },
	.horizon_height = 0.25f,
	.horizon = { 0.60f, 0.75f, 0.90f },
	.strength = 1.0f,
	.ground = { 0.20f, 0.18f, 0.16f },
	.drawn = 1,
	.disc_size = { 1.5f },
	.glow_size = { 0.5f },
};

static const voe_render_sky ZEROED = { 0 };
static const voe_render_light_blockers NONE = { 0 };

// A white sun whose light travels along `direction`.
static voe_render_light the_sun(voe_math_float3 direction)
{
	return (voe_render_light){ .direction = direction,
				   .intensity = INTENSITY,
				   .colour = { 1.0f, 1.0f, 1.0f } };
}

// One frame of one pass drawing `geometry` in `shading` at z `z`, the centre
// pixel's bytes copied into `out`; false when the read failed.
static bool picture_of(struct scene *scene, voe_render_geometry geometry,
		       voe_render_shading shading, float z,
		       voe_render_light light, voe_render_sky sky,
		       voe_render_light_blockers blockers, uint8_t out[3])
{
	voe_platform_size size = { SIDE, SIDE };
	voe_render_pass_camera camera = {
		.view = the_camera(),
		.light = light,
		.blockers = blockers,
		.sky = sky,
		.exposure = { .tone = VOE_RENDER_TONE_CLIP },
	};
	voe_render_object object = {
		.world = voe_math_float4x4_identity(),
		.normal = voe_math_float4x4_identity(),
		.shading = shading.index,
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
	};
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;
	bool drawing = false;

	object.world.m[2][3] = z;
	VOE_TEST_CHECK(voe_render_frame_begin(scene->device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return false;
	VOE_TEST_CHECK(voe_render_pass_begin(scene->device, scene->target,
					     &camera));
	VOE_TEST_CHECK(voe_render_frame_draw(scene->device, geometry, object));
	voe_render_pass_end(scene->device);
	VOE_TEST_CHECK(voe_render_frame_end(scene->device));
	VOE_TEST_CHECK(voe_render_target_read(scene->device, scene->target,
					      scene->arena, &picture, &error));
	if (picture.pixels == NULL || picture.width != SIDE ||
	    picture.height != SIDE)
		return false;
	memcpy(out, picture.pixels + ((size_t)CENTRE * SIDE + CENTRE) * 4, 3);
	return true;
}

static void print_pixel(const char *what, const uint8_t *pixel)
{
	printf("%s: %d %d %d\n", what, pixel[0], pixel[1], pixel[2]);
}

// `geometry` in `shading` at `z` reads the same bytes under `sky` as under
// `other`.
static void check_same(struct scene *scene, const char *what,
		       voe_render_geometry geometry, voe_render_shading shading,
		       float z, voe_render_light light, voe_render_sky sky,
		       voe_render_sky other, voe_render_light_blockers blockers)
{
	uint8_t a[3];
	uint8_t b[3];

	if (!picture_of(scene, geometry, shading, z, light, sky, blockers, a) ||
	    !picture_of(scene, geometry, shading, z, light, other, blockers, b))
		return;
	print_pixel(what, a);
	print_pixel("  against", b);
	VOE_TEST_CHECK(memcmp(a, b, 3) == 0);
}

// Cases 1 and 2: the grey cube's +Z face under `sun`, at fill nought and red.
static void check_cube(struct scene *scene, voe_render_light sun)
{
	voe_render_sky red_zenith = SKY;
	voe_render_light red = sun;

	red_zenith.zenith = (voe_math_float3){ 0.80f, 0.10f, 0.10f };
	red.fill = RED_FILL;
	check_same(scene, "cube, fill 0, sky drawn", scene->cube, scene->grey,
		   CUBE_Z, sun, SKY, ZEROED, NONE);
	check_same(scene, "cube, red fill, sky drawn", scene->cube, scene->grey,
		   CUBE_Z, red, SKY, ZEROED, NONE);
	check_same(scene, "cube, red fill, blue zenith", scene->cube,
		   scene->grey, CUBE_Z, red, SKY, red_zenith, NONE);
}

// A linear value as the byte an sRGB target stores for it, clamped as it does.
static int srgb_byte(float linear)
{
	float x = fminf(fmaxf(linear, 0.0f), 1.0f);
	float encoded = x <= 0.0031308f ? x * 12.92f :
					  1.055f * powf(x, 1.0f / 2.4f) - 0.055f;

	return (int)lroundf(encoded * 255.0f);
}

// The world direction through the centre pixel's centre.
static voe_math_float3 centre_direction(void)
{
	float focal = 1.0f / tanf(FIELD_OF_VIEW * 0.5f);
	float x = 2.0f * ((float)CENTRE + 0.5f) / SIDE - 1.0f;
	float y = 1.0f - 2.0f * ((float)CENTRE + 0.5f) / SIDE;

	return voe_math_float3_normalize(
		(voe_math_float3){ x / focal, y / focal, -1.0f });
}

// Karis's environment BRDF fit, lighting.slangh's, for a grey f0.
static float environment_brdf(float f0, float roughness, float n_dot_v)
{
	const float r[4] = { 1.0f - roughness, 0.0425f - 0.0275f * roughness,
			     1.04f - 0.572f * roughness,
			     -0.04f + 0.022f * roughness };
	const float a004 = fminf(r[0] * r[0], exp2f(-9.28f * n_dot_v)) * r[0] +
			   r[1];

	return f0 * (-1.04f * a004 + r[2]) + (1.04f * a004 + r[3]);
}

// How far `pixel` is from the bytes of `linear` × `scale`, summed over rgb.
static int distance_to(const uint8_t *pixel, voe_math_float3 linear,
		       float scale)
{
	return abs(pixel[0] - srgb_byte(linear.x * scale)) +
	       abs(pixel[1] - srgb_byte(linear.y * scale)) +
	       abs(pixel[2] - srgb_byte(linear.z * scale));
}

// Case 3: the metal quad mirrors the sky, sharp when smooth, blurred when rough.
static void check_mirror(struct scene *scene, voe_render_light noon)
{
	const voe_math_float3 n = { 0.0f, TILT_SIN, TILT_COS };
	const voe_math_float3 d = centre_direction();
	const voe_math_float3 r = voe_math_float3_sub(
		d, voe_math_float3_scale(n, 2.0f * voe_math_float3_dot(d, n)));
	const voe_math_float3 sky = voe_render_sky_radiance(&SKY, &noon, 1, r);
	const float n_dot_v = -voe_math_float3_dot(n, d);
	voe_math_float4 harmonics[9];
	voe_math_float3 blurred;
	uint8_t pixel[3];

	voe_render_sky_harmonics(&SKY, harmonics);
	blurred = voe_math_float3_scale(voe_render_sky_irradiance(harmonics, r),
					1.0f / PI);
	if (picture_of(scene, scene->quad, scene->smooth, 0.0f, noon, SKY, NONE,
		       pixel)) {
		const float fresnel = environment_brdf(1.0f, 0.0f, n_dot_v);

		print_pixel("smooth metal", pixel);
		printf("want %d %d %d\n", srgb_byte(sky.x * fresnel),
		       srgb_byte(sky.y * fresnel), srgb_byte(sky.z * fresnel));
		VOE_TEST_CHECK(abs(pixel[0] - srgb_byte(sky.x * fresnel)) <=
			       MIRROR_TOLERANCE);
		VOE_TEST_CHECK(abs(pixel[1] - srgb_byte(sky.y * fresnel)) <=
			       MIRROR_TOLERANCE);
		VOE_TEST_CHECK(abs(pixel[2] - srgb_byte(sky.z * fresnel)) <=
			       MIRROR_TOLERANCE);
	}
	if (picture_of(scene, scene->quad, scene->rough, 0.0f, noon, SKY, NONE,
		       pixel)) {
		const float fresnel = environment_brdf(1.0f, 1.0f, n_dot_v);
		const int to_blurred = distance_to(pixel, blurred, fresnel);
		const int to_sky = distance_to(pixel, sky, fresnel);

		print_pixel("rough metal", pixel);
		printf("from E(R)/pi %d, from the sky %d\n", to_blurred, to_sky);
		VOE_TEST_CHECK(to_blurred < to_sky);
	}
}

// Case 5: the metal quad inside a 2 m Indoors box about it mirrors no sky.
static void check_indoors(struct scene *scene, voe_render_light noon)
{
	// The box unturned: rows (a_i / h, −a_i·c / h).
	const voe_render_light_blocker around[1] = { {
		.rows = { { 1.0f, 0.0f, 0.0f, 0.0f },
			  { 0.0f, 1.0f, 0.0f, 0.0f },
			  { 0.0f, 0.0f, 1.0f, -QUAD_Z } },
		.sphere = { 0.0f, 0.0f, QUAD_Z, 1.7320508f },
	} };
	const voe_render_light_blockers indoors = { .blockers = around,
						    .count = 1,
						    .indoors = 1 };

	check_same(scene, "indoors metal, sky drawn", scene->quad,
		   scene->smooth, 0.0f, noon, SKY, ZEROED, indoors);
}

static bool build(struct scene *scene)
{
	voe_render_texture shown = { 0 };
	voe_base_error error = VOE_BASE_OK;
	voe_render_shading_values grey = {
		.base_colour = { 0.5f, 0.5f, 0.5f, 1.0f },
		.roughness = 1.0f,
		.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
	};
	voe_render_shading_values metal = {
		.base_colour = { 1.0f, 1.0f, 1.0f, 1.0f },
		.metallic = 1.0f,
		.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
	};
	bool built = true;

	built &= voe_render_target_create(scene->device, SIDE, SIDE,
					  &scene->target, &shown, &error);
	built &= voe_render_geometry_create(scene->device, CUBE_VERTICES, 24,
					    CUBE_INDICES, 36, &scene->cube,
					    &error);
	built &= voe_render_geometry_create(scene->device, QUAD_VERTICES, 4,
					    QUAD_INDICES, 6, &scene->quad,
					    &error);
	built &= voe_render_shading_create(scene->device, grey, &scene->grey,
					   &error);
	built &= voe_render_shading_create(scene->device, metal, &scene->smooth,
					   &error);
	metal.metallic = 0.0f;
	built &= voe_render_shading_create(scene->device, metal, &scene->plain,
					   &error);
	metal.metallic = 1.0f;
	metal.roughness = 1.0f;
	built &= voe_render_shading_create(scene->device, metal, &scene->rough,
					   &error);
	VOE_TEST_CHECK(built);
	return built;
}

int main(void)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	struct scene scene = { .arena = voe_base_arena_new(1024 * 1024) };
	// 50° up behind the cube, so neither the +Z face nor the quad faces it.
	const voe_render_light noon =
		the_sun((voe_math_float3){ 0.0f, -0.76604444f, 0.64278761f });
	// 2° up in front of the cube, on the +Z face's side.
	const voe_render_light low =
		the_sun((voe_math_float3){ 0.0f, -0.0348995f, -0.9993908f });

	scene.device = voe_render_device_new_headless(scene.arena, size,
						      CAPACITIES, &error);
	if (scene.device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED)
			printf("skip: %s\n", voe_base_error_string(error));
		else
			VOE_TEST_CHECK(scene.device != NULL);
		voe_base_arena_destroy(scene.arena);
		return voe_test_result();
	}
	if (build(&scene)) {
		check_cube(&scene, noon);
		check_mirror(&scene, noon);
		check_same(&scene, "non-metal quad, sky drawn", scene.quad,
			   scene.plain, 0.0f, low, SKY, ZEROED, NONE);
		check_indoors(&scene, noon);
	}
	voe_render_device_destroy(scene.device);
	voe_base_arena_destroy(scene.arena);
	return voe_test_result();
}
