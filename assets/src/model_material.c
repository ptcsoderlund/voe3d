// A glTF material turned into `.material` values and its ORM picture, behind
// assets/model_material.h, which says the rules and why.
//
// Nearest texel is read at the destination texel's centre, so a picture half
// the size reads each of its texels for two in a row and is not shifted.
#include <assets/model_material.h>

#include <base/assert.h>

#include <math.h>

static float clamp(float value, float low, float high)
{
	return fminf(fmaxf(value, low), high);
}

static float cutout(const voe_assets_material *material)
{
	switch (material->alpha_mode) {
	case VOE_ASSETS_ALPHA_CUTOUT:
		return clamp(material->alpha_cutoff, 0.0f, 1.0f);
	case VOE_ASSETS_ALPHA_BLENDED:
		return 0.5f;
	case VOE_ASSETS_ALPHA_OPAQUE:
		break;
	}
	return 0.0f;
}

voe_assets_material_file
voe_assets_model_material_values(const voe_assets_material *material)
{
	voe_assets_material_file values = voe_assets_material_default();
	voe_math_float2 scale;

	VOE_BASE_ASSERT(material != NULL, "values of no glTF material");

	scale = material->colour_scale;
	values.shader = material->unlit ? VOE_ASSETS_MATERIAL_UNLIT :
					  VOE_ASSETS_MATERIAL_LIT;
	values.colour[0] = material->base_colour.x;
	values.colour[1] = material->base_colour.y;
	values.colour[2] = material->base_colour.z;
	values.roughness = material->roughness;
	values.metal = material->metallic;
	values.repeat = scale.x == scale.y && scale.x > 0.0f ? scale.x : 1.0f;
	values.cutout = cutout(material);
	values.double_sided = material->double_sided;
	values.emission[0] = fmaxf(material->emissive.x, 0.0f);
	values.emission[1] = fmaxf(material->emissive.y, 0.0f);
	values.emission[2] = fmaxf(material->emissive.z, 0.0f);
	values.emission_strength = fmaxf(material->emissive_strength, 0.0f);
	return values;
}

// The byte of `image` at the texel nearest destination texel (x, y) of a
// width × height picture, channel `channel`.
static uint8_t nearest(const voe_assets_image *image, uint32_t x, uint32_t y,
		       uint32_t width, uint32_t height, uint32_t channel)
{
	const uint64_t sx = ((2 * (uint64_t)x + 1) * image->width) / (2 * width);
	const uint64_t sy =
		((2 * (uint64_t)y + 1) * image->height) / (2 * height);

	return image->pixels[(sy * image->width + sx) * 4 + channel];
}

// `byte` times `factor` clamped to 0–1, rounded to the nearest whole value:
// glTF's texture × factor (0449), and the factor alone as 255 × factor.
static uint8_t scaled_byte(uint8_t byte, float factor)
{
	return (uint8_t)lroundf((float)byte * clamp(factor, 0.0f, 1.0f));
}

static uint64_t pixel_count(const voe_assets_image *image)
{
	return (uint64_t)image->width * image->height;
}

static const voe_assets_image *picture(const voe_assets_model *model,
				       uint32_t image)
{
	if (image == VOE_ASSETS_MODEL_NONE)
		return NULL;
	VOE_BASE_ASSERT(image < model->image_count, "a picture out of range");
	return &model->images[image];
}

bool voe_assets_model_orm(const voe_assets_model *model, uint32_t material,
			  voe_base_arena *arena, voe_assets_image *out)
{
	const voe_assets_material *m;
	const voe_assets_image *occlusion;
	const voe_assets_image *packed;
	const voe_assets_image *size;
	voe_assets_image orm;

	VOE_BASE_ASSERT(model != NULL && arena != NULL && out != NULL,
			"an ORM picture with nothing to make it from or into");
	VOE_BASE_ASSERT(material < model->material_count,
			"an ORM picture of a material out of range");

	m = &model->materials[material];
	occlusion = picture(model, m->occlusion_image);
	packed = picture(model, m->metallic_roughness_image);
	if (occlusion == NULL && packed == NULL)
		return false;
	size = occlusion;
	if (size == NULL ||
	    (packed != NULL && pixel_count(packed) > pixel_count(size)))
		size = packed;

	orm = (voe_assets_image){ .width = size->width, .height = size->height };
	orm.pixels = voe_base_arena_push(arena,
					 (size_t)orm.width * orm.height * 4);
	for (uint32_t y = 0; y < orm.height; y++) {
		for (uint32_t x = 0; x < orm.width; x++) {
			uint8_t *texel =
				&orm.pixels[((size_t)y * orm.width + x) * 4];
			const uint8_t green = packed != NULL ?
						      nearest(packed, x, y,
							      orm.width,
							      orm.height, 1) :
						      255;
			const uint8_t blue = packed != NULL ?
						     nearest(packed, x, y,
							     orm.width,
							     orm.height, 2) :
						     255;

			texel[0] = occlusion != NULL ?
					   nearest(occlusion, x, y, orm.width,
						   orm.height, 0) :
					   255;
			texel[1] = scaled_byte(green, m->roughness);
			texel[2] = scaled_byte(blue, m->metallic);
			texel[3] = 255;
		}
	}
	*out = orm;
	return true;
}
