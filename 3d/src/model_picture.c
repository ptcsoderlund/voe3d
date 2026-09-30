// A picture in the model store: the quad written out, the decode chosen by the
// extension, the soft dot drawn in code and the upload of one texture and two
// materials (3d/src/model_picture.h says what each is for).
//
// THE QUAD'S NORMALS LEAN OUTWARD AS A SPHERE'S (ADR-0298 point 6): each corner's
// is (x, y, 1) normalised for the corner's own x and y, so a low sun lights one
// side of a camera-facing particle and not the whole of it evenly. Its UVs put
// (0, 0) at the top-left, row zero of the picture (assets/image.h).
//
// BOTH MATERIALS ARE WHITE, NOT METALLIC AND FULLY ROUGH: the picture is the
// whole of the colour, and the object's colour (0298 point 6) tints it.
#include "model_picture.h"

#include <base/assert.h>

#include <ctype.h>
#include <math.h>
#include <string.h>

// 0.5 and 1 over the length of (0.5, 0.5, 1).
#define LEAN 0.40824829f
#define UP 0.81649658f

// Whether `path` ends with `suffix`, compared without case.
static bool ends_with(const char *path, const char *suffix)
{
	size_t length = strlen(path);
	size_t tail = strlen(suffix);

	VOE_BASE_ASSERT(tail > 0, "an empty suffix");
	if (length < tail)
		return false;
	for (size_t i = 0; i < tail; i++) {
		if (tolower((unsigned char)path[length - tail + i]) != suffix[i])
			return false;
	}
	return true;
}

static bool is_jpeg(const char *path)
{
	return ends_with(path, ".jpg") || ends_with(path, ".jpeg");
}

bool voe_3d_model_picture_is(const char *path)
{
	VOE_BASE_ASSERT(path != NULL, "asking of no path");

	return ends_with(path, ".png") || is_jpeg(path);
}

bool voe_3d_model_picture_decode(const char *path, const uint8_t *bytes,
				 size_t size, voe_base_arena *arena,
				 voe_assets_image *image, voe_base_error *error)
{
	VOE_BASE_ASSERT(voe_3d_model_picture_is(path), "decoding no picture");
	VOE_BASE_ASSERT(arena != NULL && image != NULL,
			"decoding into nothing");

	if (is_jpeg(path))
		return voe_assets_jpeg_decode(bytes, size, arena, image, error);
	return voe_assets_png_decode(bytes, size, arena, image, error);
}

// The alpha falls along a smoothstep of the distance from the centre, so the
// dot has no hard ring where it meets nothing.
voe_assets_image voe_3d_model_picture_dot(voe_base_arena *arena)
{
	const uint32_t side = VOE_3D_MODEL_PICTURE_DOT_SIDE;
	const float half = 0.5f * (float)side;
	voe_assets_image dot = { .width = side, .height = side };

	VOE_BASE_ASSERT(arena != NULL, "a dot in no arena");
	dot.pixels = voe_base_arena_push(arena, (size_t)side * side * 4);
	for (uint32_t y = 0; y < side; y++) {
		for (uint32_t x = 0; x < side; x++) {
			float dx = ((float)x + 0.5f - half) / half;
			float dy = ((float)y + 0.5f - half) / half;
			float near = 1.0f - sqrtf(dx * dx + dy * dy);
			float alpha = near <= 0.0f ? 0.0f : near;
			uint8_t *pixel = &dot.pixels[((size_t)y * side + x) * 4];

			alpha = alpha * alpha * (3.0f - 2.0f * alpha);
			pixel[0] = pixel[1] = pixel[2] = 255;
			pixel[3] = (uint8_t)lroundf(alpha * 255.0f);
		}
	}
	VOE_BASE_ASSERT(dot.pixels[3] == 0, "the dot's corner is not clear");
	return dot;
}

bool voe_3d_model_picture_quad(voe_render_device *device,
			       voe_render_geometry *out, voe_base_error *error)
{
	static const voe_render_vertex vertices[4] = {
		{ { -0.5f, -0.5f, 0.0f }, { -LEAN, -LEAN, UP }, { 0.0f, 1.0f } },
		{ { 0.5f, -0.5f, 0.0f }, { LEAN, -LEAN, UP }, { 1.0f, 1.0f } },
		{ { 0.5f, 0.5f, 0.0f }, { LEAN, LEAN, UP }, { 1.0f, 0.0f } },
		{ { -0.5f, 0.5f, 0.0f }, { -LEAN, LEAN, UP }, { 0.0f, 0.0f } },
	};
	// Counter-clockwise seen from +Z.
	static const uint32_t indices[6] = { 0, 1, 2, 0, 2, 3 };

	VOE_BASE_ASSERT(device != NULL, "a quad on no device");
	VOE_BASE_ASSERT(out != NULL, "a quad into nothing");
	return voe_render_geometry_create(device, vertices, 4, indices, 6, out,
					  error);
}

// Uploads `material` and lists its record, as model_upload.c does.
static bool upload_material(voe_render_device *device,
			    voe_3d_material *material, voe_3d_model_upload *out,
			    voe_base_error *error)
{
	if (!voe_3d_material_upload(device, material, error))
		return false;
	out->shadings[out->shading_count++] = material->shading;
	out->material_count++;
	return true;
}

bool voe_3d_model_picture_upload(voe_render_device *device,
				 voe_base_arena *arena,
				 const voe_assets_image *image,
				 voe_3d_model_upload *out, voe_base_error *error)
{
	const voe_render_texture none = { .index = VOE_RENDER_NO_TEXTURE };
	voe_render_texture texture;

	VOE_BASE_ASSERT(device != NULL && arena != NULL,
			"uploading a picture with no device or arena");
	VOE_BASE_ASSERT(image != NULL && image->pixels != NULL && out != NULL,
			"uploading no picture, or into nothing");

	*out = (voe_3d_model_upload){
		.materials = voe_base_arena_push(arena, 2 * sizeof(*out->materials)),
		.textures = voe_base_arena_push(arena, sizeof(*out->textures)),
		.shadings = voe_base_arena_push(arena, 2 * sizeof(*out->shadings)),
	};
	if (!voe_render_texture_create(device, VOE_RENDER_TEXTURE_COLOUR,
				       VOE_RENDER_SAMPLING_SMOOTH, image->width,
				       image->height, image->pixels, &texture,
				       error))
		return false;
	out->textures[out->texture_count++] = texture;

	for (uint32_t i = 0; i < 2; i++) {
		out->materials[i] = (voe_3d_material){
			.base_colour = { 1.0f, 1.0f, 1.0f, 1.0f },
			.metallic = 0.0f,
			.roughness = 1.0f,
			.alpha_mode = VOE_RENDER_ALPHA_BLENDED,
			.alpha_cutoff = 0.5f,
			.unlit = i == 1,
			.base_colour_texture = texture,
			.metallic_roughness_texture = none,
			.normal_texture = none,
			.occlusion_texture = none,
			.emissive_texture = none,
		};
		if (!upload_material(device, &out->materials[i], out, error))
			return false;
	}
	return true;
}
