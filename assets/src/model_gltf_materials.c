// glTF's materials and images, read into the arrays in assets/model.h: texture
// references, the alpha mode, names, the three material extensions read (unlit,
// emissive strength, the colour texture's transform scale), the materials, then
// the images named and decoded. Called by model_gltf.c through
// model_gltf_read.h; its header's rules hold here.
//
// TEXTURES RESOLVE TO IMAGES HERE, WHICH IS WHERE DEDUPLICATION COMES FROM. glTF
// puts a sampler and an image behind a texture and a material points at the
// texture; a material here points at the image. Two textures over one picture
// therefore become one entry, one decode and — once `3d` has uploaded it — one
// texture id.
#include "model_gltf_read.h"

#include <base/report.h>

#include <string.h>

// The only texture coordinate set read. A material asking for a second one is
// refused rather than quietly given the first.
#define TEXCOORD_SET 0

// ------------------------------------------------------------- materials

// A glTF texture reference: `{ "index": n, "texCoord": k }`. It comes back as an
// index into the model's images, because that is where the deduplication is —
// see this file's header.
static bool read_texture_reference(struct gltf *gltf, uint32_t object,
				   const char *name, uint32_t *image)
{
	uint32_t reference = voe_assets_json_member(gltf->json, object, name);
	uint32_t texture_count = gltf->textures == VOE_ASSETS_JSON_NONE ?
					 0 :
					 voe_assets_json_count(gltf->json,
							       gltf->textures);
	uint32_t image_count = gltf->images == VOE_ASSETS_JSON_NONE ?
				       0 :
				       voe_assets_json_count(gltf->json,
							     gltf->images);
	uint32_t index;
	uint32_t set;
	uint32_t texture;

	*image = VOE_ASSETS_MODEL_NONE;
	if (reference == VOE_ASSETS_JSON_NONE)
		return true;
	if (voe_assets_json_kind_of(gltf->json, reference) !=
	    VOE_ASSETS_JSON_OBJECT)
		return voe_assets_gltf_malformed(
			gltf, "a texture reference that is not an object");
	if (texture_count == 0)
		return voe_assets_gltf_malformed(
			gltf,
			"a material naming a texture in a file that describes none");

	if (!voe_assets_gltf_uint_member(gltf, reference, "index",
					 texture_count - 1, texture_count,
					 &index))
		return false;
	if (index >= texture_count)
		return voe_assets_gltf_malformed(
			gltf, "a texture reference with no index in it");

	// A second set of texture coordinates is a second attribute nothing
	// here reads. Silently using the first would put a picture on the wrong
	// part of the surface.
	if (!voe_assets_gltf_uint_member(gltf, reference, "texCoord",
					 UINT32_MAX, TEXCOORD_SET, &set))
		return false;
	if (set != TEXCOORD_SET)
		return voe_assets_gltf_unsupported(
			gltf,
			"a material reading a texture coordinate set other than the first");

	texture = voe_assets_gltf_object_at(gltf, gltf->textures, index);
	if (texture == VOE_ASSETS_JSON_NONE)
		return voe_assets_gltf_malformed(
			gltf, "a texture the file does not describe");

	// A texture with no source is an extension's — KHR_texture_basisu and
	// its relatives put the image somewhere else — and this reader would
	// otherwise draw it untextured without saying why.
	if (voe_assets_json_member(gltf->json, texture, "source") ==
	    VOE_ASSETS_JSON_NONE)
		return voe_assets_gltf_unsupported(
			gltf, "a texture whose image is provided by an extension");
	if (image_count == 0)
		return voe_assets_gltf_malformed(
			gltf,
			"a texture naming an image in a file that describes none");
	if (!voe_assets_gltf_uint_member(gltf, texture, "source",
					 image_count - 1, image_count, image))
		return false;
	if (*image >= image_count)
		return voe_assets_gltf_malformed(
			gltf, "a texture whose image index is out of range");
	return true;
}

// A glTF `name` into `out`: "" when absent, the raw span otherwise. One that does
// not fit is cut back past any continuation byte, so it ends on a whole UTF-8
// character (0247); the walk back is at most three bytes on valid text and at
// most the name's length on anything else.
bool voe_assets_gltf_read_name(struct gltf *gltf, uint32_t object,
			       voe_assets_model_name *out)
{
	uint32_t token = voe_assets_json_member(gltf->json, object, "name");
	const voe_assets_json_token *text;
	size_t length;

	out->text[0] = '\0';
	if (token == VOE_ASSETS_JSON_NONE)
		return true;
	if (voe_assets_json_kind_of(gltf->json, token) != VOE_ASSETS_JSON_STRING)
		return voe_assets_gltf_malformed(gltf,
						 "a name that is not a string");

	text = &gltf->json->tokens[token];
	length = text->end - text->start;
	if (length >= VOE_ASSETS_MODEL_NAME) {
		length = VOE_ASSETS_MODEL_NAME - 1;
		while (length > 0 &&
		       ((uint8_t)gltf->json->text[text->start + length] &
			0xc0) == 0x80)
			length--;
	}
	memcpy(out->text, gltf->json->text + text->start, length);
	out->text[length] = '\0';
	return true;
}

// glTF's `doubleSided`, false when absent.
static bool read_double_sided(struct gltf *gltf, uint32_t material, bool *out)
{
	uint32_t token = voe_assets_json_member(gltf->json, material,
						"doubleSided");
	uint32_t kind;

	*out = false;
	if (token == VOE_ASSETS_JSON_NONE)
		return true;
	kind = voe_assets_json_kind_of(gltf->json, token);
	if (kind != VOE_ASSETS_JSON_TRUE && kind != VOE_ASSETS_JSON_FALSE)
		return voe_assets_gltf_malformed(
			gltf, "a doubleSided that is neither true nor false");
	*out = kind == VOE_ASSETS_JSON_TRUE;
	return true;
}

// The object `name` under `object`'s `extensions` into `out`, or
// VOE_ASSETS_JSON_NONE when either is absent. Either one present and not an
// object is malformed.
static bool read_extension(struct gltf *gltf, uint32_t object,
			   const char *name, uint32_t *out)
{
	uint32_t extensions = voe_assets_json_member(gltf->json, object,
						     "extensions");

	*out = VOE_ASSETS_JSON_NONE;
	if (extensions == VOE_ASSETS_JSON_NONE)
		return true;
	if (voe_assets_json_kind_of(gltf->json, extensions) !=
	    VOE_ASSETS_JSON_OBJECT)
		return voe_assets_gltf_malformed(
			gltf, "an extensions property that is not an object");

	*out = voe_assets_json_member(gltf->json, extensions, name);
	if (*out != VOE_ASSETS_JSON_NONE &&
	    voe_assets_json_kind_of(gltf->json, *out) != VOE_ASSETS_JSON_OBJECT) {
		*out = VOE_ASSETS_JSON_NONE;
		return voe_assets_gltf_malformed(
			gltf, "an extension that is not an object");
	}
	return true;
}

// KHR_materials_unlit, which is its presence, and
// KHR_materials_emissive_strength's `emissiveStrength`, 1 without it.
static bool read_material_extensions(struct gltf *gltf, uint32_t material,
				     voe_assets_material *out)
{
	uint32_t unlit;
	uint32_t strength;

	if (!read_extension(gltf, material, "KHR_materials_unlit", &unlit) ||
	    !read_extension(gltf, material, "KHR_materials_emissive_strength",
			    &strength))
		return false;
	out->unlit = unlit != VOE_ASSETS_JSON_NONE;
	out->emissive_strength = 1.0f;
	if (strength == VOE_ASSETS_JSON_NONE)
		return true;
	return voe_assets_gltf_float_member(gltf, strength, "emissiveStrength",
					    1.0f, &out->emissive_strength);
}

// KHR_texture_transform's `scale` on the base colour texture, 1 1 without it.
// Called after read_texture_reference has checked the reference is an object.
static bool read_colour_scale(struct gltf *gltf, uint32_t pbr,
			      voe_math_float2 *out)
{
	uint32_t texture = voe_assets_json_member(gltf->json, pbr,
						  "baseColorTexture");
	uint32_t transform;
	float scale[2] = { 1.0f, 1.0f };
	bool found;

	*out = (voe_math_float2){ 1.0f, 1.0f };
	if (texture == VOE_ASSETS_JSON_NONE)
		return true;
	if (!read_extension(gltf, texture, "KHR_texture_transform",
			    &transform))
		return false;
	if (transform == VOE_ASSETS_JSON_NONE)
		return true;
	if (!voe_assets_gltf_floats_member(gltf, transform, "scale", 2, scale,
					   &found))
		return false;
	*out = (voe_math_float2){ scale[0], scale[1] };
	return true;
}

// glTF's `alphaMode`, which is a string and not a number, into the engine's
// enum. Absent is OPAQUE, which is what the specification says a material that
// omits it means.
//
// AN UNKNOWN MODE IS REFUSED AND NOT DEFAULTED. A file naming a fourth mode is a
// file this reader does not understand, and drawing it opaque would be a
// silently wrong picture rather than an answer — so the name is printed and the
// read stops, the same way an unknown required extension does.
static bool read_alpha_mode(struct gltf *gltf, uint32_t material,
			    voe_assets_alpha_mode *out)
{
	uint32_t token = voe_assets_json_member(gltf->json, material,
						"alphaMode");
	const voe_assets_json_token *text;

	*out = VOE_ASSETS_ALPHA_OPAQUE;
	if (token == VOE_ASSETS_JSON_NONE)
		return true;
	if (voe_assets_json_kind_of(gltf->json, token) != VOE_ASSETS_JSON_STRING)
		return voe_assets_gltf_malformed(
			gltf, "an alphaMode that is not a string");

	if (voe_assets_json_is(gltf->json, token, "OPAQUE"))
		return true;
	if (voe_assets_json_is(gltf->json, token, "MASK")) {
		*out = VOE_ASSETS_ALPHA_CUTOUT;
		return true;
	}
	if (voe_assets_json_is(gltf->json, token, "BLEND")) {
		*out = VOE_ASSETS_ALPHA_BLENDED;
		return true;
	}

	// The name is in the message because glTF's modes are words and the
	// whole of the answer is which word this file used.
	text = &gltf->json->tokens[token];
	VOE_BASE_ERROR("assets",
		       "glTF: alphaMode %.*s; this reader knows OPAQUE, MASK and BLEND only",
		       (int)(text->end - text->start),
		       gltf->json->text + text->start);
	gltf->error = VOE_BASE_ERROR_UNSUPPORTED;
	return false;
}

bool voe_assets_gltf_read_materials(struct gltf *gltf, voe_assets_model *model)
{
	voe_assets_material *materials;
	uint32_t count = gltf->materials == VOE_ASSETS_JSON_NONE ?
				 0 :
				 voe_assets_json_count(gltf->json,
						       gltf->materials);

	model->materials = NULL;
	model->material_count = 0;
	if (count == 0)
		return true;

	materials = voe_base_arena_push(gltf->arena,
					(size_t)count * sizeof(*materials));

	for (uint32_t i = 0; i < count; i++) {
		uint32_t material =
			voe_assets_gltf_object_at(gltf, gltf->materials, i);
		uint32_t pbr;
		// glTF's defaults, and they are the ones a material that says
		// nothing is defined to have: white, fully metallic, fully
		// rough, no emission, opaque, and a cutoff of a half that only
		// a cutout material ever reads.
		float base[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		float emissive[3] = { 0.0f, 0.0f, 0.0f };
		bool found;

		if (material == VOE_ASSETS_JSON_NONE)
			return voe_assets_gltf_malformed(
				gltf, "a material that is not an object");

		materials[i] = (voe_assets_material){
			.base_colour = { 1.0f, 1.0f, 1.0f, 1.0f },
			.metallic = 1.0f,
			.roughness = 1.0f,
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

		pbr = voe_assets_json_member(gltf->json, material,
					     "pbrMetallicRoughness");
		if (pbr != VOE_ASSETS_JSON_NONE) {
			if (voe_assets_json_kind_of(gltf->json, pbr) !=
			    VOE_ASSETS_JSON_OBJECT)
				return voe_assets_gltf_malformed(
					gltf,
					"a pbrMetallicRoughness that is not an object");
			if (!voe_assets_gltf_floats_member(gltf, pbr,
							   "baseColorFactor", 4,
							   base, &found) ||
			    !voe_assets_gltf_float_member(
				    gltf, pbr, "metallicFactor", 1.0f,
				    &materials[i].metallic) ||
			    !voe_assets_gltf_float_member(
				    gltf, pbr, "roughnessFactor", 1.0f,
				    &materials[i].roughness) ||
			    !read_texture_reference(gltf, pbr,
						    "baseColorTexture",
						    &materials[i].base_colour_image) ||
			    !read_texture_reference(gltf, pbr,
						    "metallicRoughnessTexture",
						    &materials[i].metallic_roughness_image) ||
			    !read_colour_scale(gltf, pbr,
					       &materials[i].colour_scale))
				return false;
			materials[i].base_colour = (voe_math_float4){
				base[0], base[1], base[2], base[3]
			};
		}

		if (!read_alpha_mode(gltf, material, &materials[i].alpha_mode) ||
		    !voe_assets_gltf_float_member(gltf, material, "alphaCutoff",
						  0.5f,
						  &materials[i].alpha_cutoff) ||
		    !voe_assets_gltf_floats_member(gltf, material,
						   "emissiveFactor", 3,
						   emissive, &found) ||
		    !read_texture_reference(gltf, material, "normalTexture",
					    &materials[i].normal_image) ||
		    !read_texture_reference(gltf, material, "occlusionTexture",
					    &materials[i].occlusion_image) ||
		    !read_texture_reference(gltf, material, "emissiveTexture",
					    &materials[i].emissive_image) ||
		    !voe_assets_gltf_read_name(gltf, material,
					       &materials[i].name) ||
		    !read_double_sided(gltf, material,
				       &materials[i].double_sided) ||
		    !read_material_extensions(gltf, material, &materials[i]))
			return false;
		materials[i].emissive = (voe_math_float3){ emissive[0],
							   emissive[1],
							   emissive[2] };
	}

	model->materials = materials;
	model->material_count = count;
	return true;
}

// ---------------------------------------------------------------- images

// A PNG's eight-byte signature and a JPEG's start-of-image marker.
static bool looks_like_png(const uint8_t *bytes, size_t size)
{
	static const uint8_t signature[8] = { 0x89, 'P', 'N', 'G',
					      '\r', '\n', 0x1a, '\n' };

	return size >= sizeof(signature) &&
	       memcmp(bytes, signature, sizeof(signature)) == 0;
}

static bool looks_like_jpeg(const uint8_t *bytes, size_t size)
{
	return size >= 2 && bytes[0] == 0xff && bytes[1] == 0xd8;
}

bool voe_assets_gltf_read_images(struct gltf *gltf, voe_assets_model *model)
{
	voe_assets_image *images;
	voe_assets_model_name *names;
	uint32_t count = gltf->images == VOE_ASSETS_JSON_NONE ?
				 0 :
				 voe_assets_json_count(gltf->json, gltf->images);

	model->images = NULL;
	model->image_names = NULL;
	model->image_count = 0;
	if (count == 0)
		return true;

	images = voe_base_arena_push(gltf->arena,
				     (size_t)count * sizeof(*images));
	names = voe_base_arena_push(gltf->arena,
				    (size_t)count * sizeof(*names));

	for (uint32_t i = 0; i < count; i++) {
		uint32_t image = voe_assets_gltf_object_at(gltf, gltf->images, i);
		uint32_t view_index;
		uint32_t view;
		uint32_t buffer_index;
		uint32_t offset;
		uint32_t length;
		const uint8_t *bytes;
		uint32_t view_count = gltf->views == VOE_ASSETS_JSON_NONE ?
					      0 :
					      voe_assets_json_count(gltf->json,
								    gltf->views);

		if (image == VOE_ASSETS_JSON_NONE)
			return voe_assets_gltf_malformed(
				gltf, "an image that is not an object");
		if (!voe_assets_gltf_read_name(gltf, image, &names[i]))
			return false;

		// The same refusal a buffer with a URI gets, and the same
		// reason: nothing here opens a file or decodes base64.
		if (voe_assets_json_member(gltf->json, image, "uri") !=
		    VOE_ASSETS_JSON_NONE)
			return voe_assets_gltf_unsupported(
				gltf,
				"an image with a URI in it: this reader opens no files and decodes no data URIs");
		if (voe_assets_json_member(gltf->json, image, "bufferView") ==
		    VOE_ASSETS_JSON_NONE)
			return voe_assets_gltf_malformed(
				gltf,
				"an image with neither a URI nor a buffer view");
		if (view_count == 0)
			return voe_assets_gltf_malformed(
				gltf,
				"an image naming a buffer view in a file that describes none");
		if (!voe_assets_gltf_uint_member(gltf, image, "bufferView",
						 view_count - 1, 0, &view_index))
			return false;

		view = voe_assets_gltf_object_at(gltf, gltf->views, view_index);
		if (view == VOE_ASSETS_JSON_NONE)
			return voe_assets_gltf_malformed(
				gltf, "a buffer view the file does not describe");
		if (!voe_assets_gltf_uint_member(gltf, view, "buffer",
						 UINT32_MAX, 0,
						 &buffer_index) ||
		    !voe_assets_gltf_check_buffer(gltf, buffer_index))
			return false;
		if (!voe_assets_gltf_uint_member(gltf, view, "byteOffset",
						 UINT32_MAX, 0, &offset) ||
		    !voe_assets_gltf_uint_member(gltf, view, "byteLength",
						 UINT32_MAX, 0, &length))
			return false;
		if (length == 0)
			return voe_assets_gltf_malformed(gltf,
							 "an image of no bytes");
		if ((uint64_t)offset + length > gltf->bin_size)
			return voe_assets_gltf_malformed(
				gltf,
				"an image reaching past the end of the binary chunk");

		bytes = gltf->bin + offset;
		if (looks_like_png(bytes, length)) {
			if (!voe_assets_png_decode(bytes, length, gltf->arena,
						   &images[i], &gltf->error))
				return false;
		} else if (looks_like_jpeg(bytes, length)) {
			if (!voe_assets_jpeg_decode(bytes, length, gltf->arena,
						    &images[i], &gltf->error))
				return false;
		} else {
			// The mimeType is not trusted for this — it is a claim
			// and the magic number is the fact — but a file that
			// says what it thought it was is worth repeating.
			return voe_assets_gltf_unsupported(
				gltf, "an image that is neither a PNG nor a JPEG");
		}
	}

	model->images = images;
	model->image_names = names;
	model->image_count = count;
	return true;
}
