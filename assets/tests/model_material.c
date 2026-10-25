// A glTF material to `.material` values and its ORM picture, from
// voe_assets_model values built here, no file: opaque, MASK 0.3, BLEND,
// unlit, double-sided, emissive strength, a uniform and an uneven scale map
// to their values; the ORM picture from a 2×2 metal-roughness picture alone,
// with a 4×4 occlusion picture (both at factors 1), from occlusion alone, and
// from neither; and a picture's green and blue times roughness and metal:
// metal 0 over blue 255 is 0, metal 1 is 255, 0.5 over 200 is 100, and a
// metal of 2 reads as 1.
#include <assets/model_material.h>
#include <base/arena.h>

#include <testing/test.h>

#include <string.h>

static voe_assets_material gltf_material(void)
{
	return (voe_assets_material){
		.base_colour = { 0.2f, 0.4f, 0.6f, 0.5f },
		.metallic = 0.3f,
		.roughness = 0.7f,
		.alpha_mode = VOE_ASSETS_ALPHA_OPAQUE,
		.alpha_cutoff = 0.5f,
		.emissive_strength = 1.0f,
		.colour_scale = { 1.0f, 1.0f },
		.base_colour_image = VOE_ASSETS_MODEL_NONE,
		.metallic_roughness_image = VOE_ASSETS_MODEL_NONE,
		.normal_image = VOE_ASSETS_MODEL_NONE,
		.occlusion_image = VOE_ASSETS_MODEL_NONE,
		.emissive_image = VOE_ASSETS_MODEL_NONE,
	};
}

static void opaque_maps_to_plain_values(void)
{
	const voe_assets_material material = gltf_material();
	const voe_assets_material_file values =
		voe_assets_model_material_values(&material);

	VOE_TEST_CHECK_INT(values.shader, VOE_ASSETS_MATERIAL_LIT);
	VOE_TEST_CHECK_FLOAT(values.colour[0], 0.2f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(values.colour[1], 0.4f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(values.colour[2], 0.6f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(values.roughness, 0.7f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(values.metal, 0.3f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(values.repeat, 1.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(values.cutout, 0.0f, 0.0f);
	VOE_TEST_CHECK(!values.double_sided);
	VOE_TEST_CHECK_FLOAT(values.emission[0], 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(values.emission_strength, 1.0f, 0.0f);
	VOE_TEST_CHECK(values.colormap[0] == '\0' &&
		       values.normalmap[0] == '\0' &&
		       values.ormmap[0] == '\0' &&
		       values.emissionmap[0] == '\0');
}

static void each_gltf_field_maps_to_its_value(void)
{
	voe_assets_material material = gltf_material();

	material.alpha_mode = VOE_ASSETS_ALPHA_CUTOUT;
	material.alpha_cutoff = 0.3f;
	VOE_TEST_CHECK_FLOAT(voe_assets_model_material_values(&material).cutout,
			     0.3f, 1e-6f);

	material.alpha_mode = VOE_ASSETS_ALPHA_BLENDED;
	VOE_TEST_CHECK_FLOAT(voe_assets_model_material_values(&material).cutout,
			     0.5f, 0.0f);

	material = gltf_material();
	material.unlit = true;
	VOE_TEST_CHECK_INT(voe_assets_model_material_values(&material).shader,
			   VOE_ASSETS_MATERIAL_UNLIT);

	material = gltf_material();
	material.double_sided = true;
	VOE_TEST_CHECK(voe_assets_model_material_values(&material).double_sided);

	material = gltf_material();
	material.emissive = (voe_math_float3){ 1.0f, 0.5f, 0.0f };
	material.emissive_strength = 4.0f;
	{
		const voe_assets_material_file values =
			voe_assets_model_material_values(&material);

		VOE_TEST_CHECK_FLOAT(values.emission[0], 1.0f, 0.0f);
		VOE_TEST_CHECK_FLOAT(values.emission[1], 0.5f, 0.0f);
		VOE_TEST_CHECK_FLOAT(values.emission[2], 0.0f, 0.0f);
		VOE_TEST_CHECK_FLOAT(values.emission_strength, 4.0f, 0.0f);
	}

	material = gltf_material();
	material.colour_scale = (voe_math_float2){ 3.0f, 3.0f };
	VOE_TEST_CHECK_FLOAT(voe_assets_model_material_values(&material).repeat,
			     3.0f, 0.0f);
	material.colour_scale = (voe_math_float2){ 2.0f, 3.0f };
	VOE_TEST_CHECK_FLOAT(voe_assets_model_material_values(&material).repeat,
			     1.0f, 0.0f);
}

// A 2×2 metal-roughness picture: red 7, green 10 + texel, blue 20 + texel.
static uint8_t packed_pixels[2 * 2 * 4];
// A 4×4 occlusion picture: red 100 + texel, green and blue 9.
static uint8_t occlusion_pixels[4 * 4 * 4];

static voe_assets_model orm_model(voe_assets_material *materials,
				  voe_assets_image *images)
{
	for (uint32_t i = 0; i < 4; i++) {
		packed_pixels[i * 4 + 0] = 7;
		packed_pixels[i * 4 + 1] = (uint8_t)(10 + i);
		packed_pixels[i * 4 + 2] = (uint8_t)(20 + i);
		packed_pixels[i * 4 + 3] = 255;
	}
	for (uint32_t i = 0; i < 16; i++) {
		occlusion_pixels[i * 4 + 0] = (uint8_t)(100 + i);
		occlusion_pixels[i * 4 + 1] = 9;
		occlusion_pixels[i * 4 + 2] = 9;
		occlusion_pixels[i * 4 + 3] = 255;
	}
	images[0] = (voe_assets_image){ 2, 2, packed_pixels, 0,
					VOE_ASSETS_IMAGE_RGBA8 };
	images[1] = (voe_assets_image){ 4, 4, occlusion_pixels, 0,
					VOE_ASSETS_IMAGE_RGBA8 };
	for (uint32_t i = 0; i < 4; i++)
		materials[i] = gltf_material();
	// Factors of 1 keep the picture as painted.
	for (uint32_t i = 0; i < 2; i++) {
		materials[i].metallic_roughness_image = 0;
		materials[i].roughness = 1.0f;
		materials[i].metallic = 1.0f;
	}
	materials[1].occlusion_image = 1;
	materials[2].occlusion_image = 1;
	materials[2].roughness = 0.6f;
	materials[2].metallic = 0.2f;
	return (voe_assets_model){ .materials = materials,
				   .material_count = 4,
				   .images = images,
				   .image_count = 2 };
}

static void orm_pictures(voe_base_arena *arena)
{
	voe_assets_material materials[4];
	voe_assets_image images[2];
	const voe_assets_model model = orm_model(materials, images);
	voe_assets_image orm = { 0 };

	VOE_TEST_CHECK(voe_assets_model_orm(&model, 0, arena, &orm));
	VOE_TEST_CHECK_INT(orm.width, 2);
	VOE_TEST_CHECK_INT(orm.height, 2);
	for (uint32_t i = 0; i < 4; i++) {
		VOE_TEST_CHECK_INT(orm.pixels[i * 4 + 0], 255);
		VOE_TEST_CHECK_INT(orm.pixels[i * 4 + 1], 10 + i);
		VOE_TEST_CHECK_INT(orm.pixels[i * 4 + 2], 20 + i);
	}

	VOE_TEST_CHECK(voe_assets_model_orm(&model, 1, arena, &orm));
	VOE_TEST_CHECK_INT(orm.width, 4);
	VOE_TEST_CHECK_INT(orm.height, 4);
	for (uint32_t y = 0; y < 4; y++) {
		for (uint32_t x = 0; x < 4; x++) {
			const uint8_t *texel = &orm.pixels[(y * 4 + x) * 4];
			const uint32_t small = (y / 2) * 2 + x / 2;

			VOE_TEST_CHECK_INT(texel[0], 100 + y * 4 + x);
			VOE_TEST_CHECK_INT(texel[1], 10 + small);
			VOE_TEST_CHECK_INT(texel[2], 20 + small);
		}
	}

	VOE_TEST_CHECK(voe_assets_model_orm(&model, 2, arena, &orm));
	VOE_TEST_CHECK_INT(orm.width, 4);
	VOE_TEST_CHECK_INT(orm.pixels[5 * 4 + 0], 105);
	VOE_TEST_CHECK_INT(orm.pixels[5 * 4 + 1], 153);
	VOE_TEST_CHECK_INT(orm.pixels[5 * 4 + 2], 51);

	memset(&orm, 0, sizeof orm);
	VOE_TEST_CHECK(!voe_assets_model_orm(&model, 3, arena, &orm));
	VOE_TEST_CHECK(orm.width == 0 && orm.pixels == NULL);
}

// A 2×2 bark picture: green 10 + texel, blue 255; and a 2×2 picture all 200.
static uint8_t bark_pixels[2 * 2 * 4];
static uint8_t grey_pixels[2 * 2 * 4];

static void orm_multiplies_the_picture_by_the_factors(voe_base_arena *arena)
{
	voe_assets_material materials[4];
	voe_assets_image images[2] = {
		{ 2, 2, bark_pixels, 0, VOE_ASSETS_IMAGE_RGBA8 },
		{ 2, 2, grey_pixels, 0, VOE_ASSETS_IMAGE_RGBA8 }
	};
	const float metallic[4] = { 0.0f, 1.0f, 0.5f, 2.0f };
	const voe_assets_model model = { .materials = materials,
					 .material_count = 4,
					 .images = images,
					 .image_count = 2 };
	voe_assets_image orm = { 0 };

	for (uint32_t i = 0; i < 4; i++) {
		bark_pixels[i * 4 + 0] = 7;
		bark_pixels[i * 4 + 1] = (uint8_t)(10 + i);
		bark_pixels[i * 4 + 2] = 255;
		bark_pixels[i * 4 + 3] = 255;
		memset(&grey_pixels[i * 4], 200, 4);
		materials[i] = gltf_material();
		materials[i].metallic_roughness_image = 0;
		materials[i].roughness = 1.0f;
		materials[i].metallic = metallic[i];
	}
	materials[2].metallic_roughness_image = 1;
	materials[2].roughness = 0.5f;

	VOE_TEST_CHECK(voe_assets_model_orm(&model, 0, arena, &orm));
	for (uint32_t i = 0; i < 4; i++) {
		VOE_TEST_CHECK_INT(orm.pixels[i * 4 + 1], 10 + i);
		VOE_TEST_CHECK_INT(orm.pixels[i * 4 + 2], 0);
	}
	VOE_TEST_CHECK(voe_assets_model_orm(&model, 1, arena, &orm));
	for (uint32_t i = 0; i < 4; i++)
		VOE_TEST_CHECK_INT(orm.pixels[i * 4 + 2], 255);
	VOE_TEST_CHECK(voe_assets_model_orm(&model, 2, arena, &orm));
	for (uint32_t i = 0; i < 4; i++) {
		VOE_TEST_CHECK_INT(orm.pixels[i * 4 + 1], 100);
		VOE_TEST_CHECK_INT(orm.pixels[i * 4 + 2], 100);
	}
	VOE_TEST_CHECK(voe_assets_model_orm(&model, 3, arena, &orm));
	for (uint32_t i = 0; i < 4; i++)
		VOE_TEST_CHECK_INT(orm.pixels[i * 4 + 2], 255);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(4096);

	opaque_maps_to_plain_values();
	each_gltf_field_maps_to_its_value();
	orm_pictures(arena);
	orm_multiplies_the_picture_by_the_factors(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
