// glTF's description, read into the arrays in assets/model.h. Accessors, buffer
// views, materials, images, meshes and the node tree.
//
// EVERY LENGTH, COUNT AND INDEX READ OUT OF THIS FILE IS CHECKED BEFORE IT IS
// USED, AND THAT IS THE RULE THE WHOLE FILE FOLLOWS. A glTF says how many
// vertices an accessor has, where in a buffer view it starts and how far apart
// its elements are, and every one of those numbers is a file's claim rather than
// a fact. So: an index is read with a limit beside it, an offset and a length are
// added in sixty-four bits and compared against the chunk that has to hold them,
// and every index inside an index buffer is compared against the vertex count it
// has to be inside. png.c states the same rule for the same reason.
//
// THERE IS NO RECURSION ANYWHERE IN HERE (rule 14). The node tree is read as a
// flat array with a parent count beside it, which is also what proves the tree is
// a tree: a node claimed as a child twice is refused, so the walk the importer
// does cannot meet a cycle. The JSON underneath is read by a parser with an
// explicit stack for the same reason — see json.h.
//
// THE ONLY BUFFER IS THE `.glb`'S BINARY CHUNK. A glTF buffer with a `uri` names
// an external file or carries base64, and neither is read here: `platform` has
// no file API and `.glb` was chosen precisely so that there is nothing to
// resolve. A file that asks is refused as unsupported, by name.
//
// TRIANGLE LISTS ONLY, AND AN UNINDEXED PRIMITIVE IS GIVEN INDICES ON THE WAY
// IN. Strips, fans, lines and points are refused with their mode number in the
// message. Generating the indices for a primitive that has none costs one array
// and means everything downstream — the pool, the draw, the importer — has
// exactly one shape to handle.
//
// TEXTURES RESOLVE TO IMAGES HERE, WHICH IS WHERE DEDUPLICATION COMES FROM. glTF
// puts a sampler and an image behind a texture and a material points at the
// texture; a material here points at the image. Two textures over one picture
// therefore become one entry, one decode and — once `3d` has uploaded it — one
// texture id.
//
// SKINS AND ANIMATIONS ARE NOT READ AND ARE NOT REFUSED. A skinned mesh's
// JOINTS_0 and WEIGHTS_0 attributes are ignored, so the mesh arrives in its bind
// pose; animation channels are not looked at. Refusing would turn most files
// people have into an error, and the card that reads them is a later one.
#include "model_gltf.h"

#include <base/assert.h>
#include <base/report.h>

#include <string.h>

// glTF's component types, which are OpenGL's enumerants.
#define COMPONENT_BYTE 5120
#define COMPONENT_UNSIGNED_BYTE 5121
#define COMPONENT_SHORT 5122
#define COMPONENT_UNSIGNED_SHORT 5123
#define COMPONENT_UNSIGNED_INT 5125
#define COMPONENT_FLOAT 5126

// The only primitive mode this engine draws.
#define MODE_TRIANGLES 4

// The only texture coordinate set read. A material asking for a second one is
// refused rather than quietly given the first.
#define TEXCOORD_SET 0

struct gltf {
	const voe_assets_json *json;
	const uint8_t *bin;
	size_t bin_size;
	voe_base_arena *arena;

	// The arrays at the top of the document, or VOE_ASSETS_JSON_NONE. Read
	// once, because every lookup below needs one of them.
	uint32_t accessors;
	uint32_t views;
	uint32_t buffers;
	uint32_t materials;
	uint32_t textures;
	uint32_t images;
	uint32_t meshes;
	uint32_t nodes;
	uint32_t scenes;

	voe_base_error error;
};

static bool refuse(struct gltf *gltf, voe_base_error error,
		   const char *message)
{
	VOE_BASE_ERROR("assets", "glTF: %s", message);
	gltf->error = error;
	return false;
}

static bool malformed(struct gltf *gltf, const char *message)
{
	return refuse(gltf, VOE_BASE_ERROR_MALFORMED, message);
}

static bool unsupported(struct gltf *gltf, const char *message)
{
	return refuse(gltf, VOE_BASE_ERROR_UNSUPPORTED, message);
}

// A member that has to be an array if it is there at all. count comes back as
// nought when it is not there, so a caller loops over nothing rather than
// branching.
static bool array_member(struct gltf *gltf, uint32_t object, const char *name,
			 uint32_t *token, uint32_t *count)
{
	uint32_t found = voe_assets_json_member(gltf->json, object, name);

	*token = VOE_ASSETS_JSON_NONE;
	*count = 0;
	if (found == VOE_ASSETS_JSON_NONE)
		return true;
	if (voe_assets_json_kind_of(gltf->json, found) !=
	    VOE_ASSETS_JSON_ARRAY)
		return malformed(gltf, "a property that has to be an array and is not");

	*token = found;
	*count = voe_assets_json_count(gltf->json, found);
	return true;
}

// An object out of one of the document's arrays, by index. The index has already
// been checked against the array's length by whoever read it.
static uint32_t object_at(struct gltf *gltf, uint32_t array, uint32_t index)
{
	uint32_t element;

	// An array the document does not have at all, which is every one of
	// them for a file that describes nothing of that kind. Answered here
	// once so that no caller has to check before asking.
	if (array == VOE_ASSETS_JSON_NONE)
		return VOE_ASSETS_JSON_NONE;

	element = voe_assets_json_element(gltf->json, array, index);
	if (element == VOE_ASSETS_JSON_NONE)
		return VOE_ASSETS_JSON_NONE;
	if (voe_assets_json_kind_of(gltf->json, element) !=
	    VOE_ASSETS_JSON_OBJECT)
		return VOE_ASSETS_JSON_NONE;
	return element;
}

// An unsigned member with a limit, and a default when it is absent. False only
// when it is there and is not a whole number within the limit — which is
// malformed, because every count and index in a glTF is one.
static bool uint_member(struct gltf *gltf, uint32_t object, const char *name,
			uint32_t limit, uint32_t fallback, uint32_t *out)
{
	uint32_t token = voe_assets_json_member(gltf->json, object, name);

	*out = fallback;
	if (token == VOE_ASSETS_JSON_NONE)
		return true;
	if (!voe_assets_json_uint(gltf->json, token, limit, out))
		return malformed(gltf,
				 "a count or an index that is not a whole number inside its range");
	return true;
}

static bool float_member(struct gltf *gltf, uint32_t object, const char *name,
			 float fallback, float *out)
{
	uint32_t token = voe_assets_json_member(gltf->json, object, name);

	*out = fallback;
	if (token == VOE_ASSETS_JSON_NONE)
		return true;
	if (!voe_assets_json_float(gltf->json, token, out))
		return malformed(gltf, "a factor that is not a number");
	return true;
}

// A fixed-length array of numbers — a colour, a translation, a matrix. Absent is
// not an error and leaves `out` as the caller set it.
static bool floats_member(struct gltf *gltf, uint32_t object, const char *name,
			  uint32_t length, float *out, bool *found)
{
	uint32_t token = voe_assets_json_member(gltf->json, object, name);

	*found = false;
	if (token == VOE_ASSETS_JSON_NONE)
		return true;
	if (voe_assets_json_kind_of(gltf->json, token) != VOE_ASSETS_JSON_ARRAY)
		return malformed(gltf, "a vector that is not an array");
	if (voe_assets_json_count(gltf->json, token) != length)
		return malformed(gltf, "a vector of the wrong length");

	for (uint32_t i = 0; i < length; i++) {
		if (!voe_assets_json_float(
			    gltf->json,
			    voe_assets_json_element(gltf->json, token, i),
			    &out[i]))
			return malformed(gltf,
					 "a vector with something that is not a number in it");
	}

	*found = true;
	return true;
}

// ------------------------------------------------------------- accessors

// Where an accessor's elements actually are: the first byte of the first one,
// how far apart they are, how many there are and what each component is.
//
// EVERYTHING IN HERE HAS BEEN CHECKED AGAINST THE BINARY CHUNK BEFORE IT IS
// FILLED IN. A caller may walk `count` elements at `stride` from `base` without
// checking anything itself, and that is the whole point of resolving an accessor
// in one place.
struct view {
	const uint8_t *base;
	size_t stride;
	uint32_t count;
	uint32_t component;
	uint32_t components;
};

static uint32_t component_size(uint32_t component)
{
	switch (component) {
	case COMPONENT_BYTE:
	case COMPONENT_UNSIGNED_BYTE:
		return 1;
	case COMPONENT_SHORT:
	case COMPONENT_UNSIGNED_SHORT:
		return 2;
	case COMPONENT_UNSIGNED_INT:
	case COMPONENT_FLOAT:
		return 4;
	default:
		return 0;
	}
}

// glTF's element types, as the number of components in one.
static uint32_t components_of(const voe_assets_json *json, uint32_t token)
{
	if (voe_assets_json_is(json, token, "SCALAR"))
		return 1;
	if (voe_assets_json_is(json, token, "VEC2"))
		return 2;
	if (voe_assets_json_is(json, token, "VEC3"))
		return 3;
	if (voe_assets_json_is(json, token, "VEC4"))
		return 4;
	// MAT2, MAT3 and MAT4 are the rest of glTF's list and nothing here reads
	// one, so they come back as zero and are refused by name.
	return 0;
}

// The one buffer this reader allows is the container's binary chunk. Checked
// once, from the first accessor that names it.
static bool check_buffer(struct gltf *gltf, uint32_t index)
{
	uint32_t buffer;
	uint32_t length;

	if (index != 0)
		return unsupported(gltf,
				   "a buffer other than the file's own binary chunk, which means an external file");

	buffer = object_at(gltf, gltf->buffers, 0);
	if (buffer == VOE_ASSETS_JSON_NONE)
		return malformed(gltf, "a buffer view naming a buffer the file does not describe");
	if (voe_assets_json_member(gltf->json, buffer, "uri") !=
	    VOE_ASSETS_JSON_NONE)
		return unsupported(gltf,
				   "a buffer with a URI in it: this reader opens no files and decodes no data URIs");
	if (!uint_member(gltf, buffer, "byteLength", UINT32_MAX, 0, &length))
		return false;
	if (length > gltf->bin_size)
		return malformed(gltf,
				 "a buffer that claims to be longer than the binary chunk holding it");
	return true;
}

static bool resolve_accessor(struct gltf *gltf, uint32_t index,
			     struct view *out)
{
	uint32_t accessor = object_at(gltf, gltf->accessors, index);
	uint32_t type_token;
	uint32_t view_index;
	uint32_t view;
	uint32_t buffer_index;
	uint32_t accessor_offset;
	uint32_t view_offset;
	uint32_t view_length;
	uint32_t stride;
	uint32_t element;
	uint64_t first;
	uint64_t last;

	if (accessor == VOE_ASSETS_JSON_NONE)
		return malformed(gltf, "an accessor the file does not describe");

	// A sparse accessor is a base array with a list of overrides on top of
	// it. It is a different reader and producing the base array alone would
	// be a quietly wrong mesh.
	if (voe_assets_json_member(gltf->json, accessor, "sparse") !=
	    VOE_ASSETS_JSON_NONE)
		return unsupported(gltf, "a sparse accessor");

	if (!uint_member(gltf, accessor, "componentType", UINT32_MAX, 0,
			 &out->component))
		return false;
	element = component_size(out->component);
	if (element == 0)
		return unsupported(gltf,
				   "an accessor whose component type this reader does not know");

	type_token = voe_assets_json_member(gltf->json, accessor, "type");
	if (type_token == VOE_ASSETS_JSON_NONE)
		return malformed(gltf, "an accessor with no type");
	out->components = components_of(gltf->json, type_token);
	if (out->components == 0)
		return unsupported(gltf,
				   "an accessor of a type this reader does not read, which means a matrix");
	element *= out->components;

	// An element is at least one byte, so an accessor cannot have more
	// elements than the binary chunk has bytes. That is the limit, and it is
	// what keeps the arithmetic below inside sixty-four bits by
	// construction.
	if (!uint_member(gltf, accessor, "count",
			 (uint32_t)(gltf->bin_size > UINT32_MAX ?
					    UINT32_MAX :
					    gltf->bin_size),
			 0, &out->count))
		return false;
	if (out->count == 0)
		return malformed(gltf, "an accessor with no elements in it");

	if (!uint_member(gltf, accessor, "byteOffset", UINT32_MAX, 0,
			 &accessor_offset))
		return false;

	// An accessor with no buffer view is defined as all zeros. Nothing here
	// wants a mesh made of zeros, and pretending otherwise would draw a
	// point at the origin instead of saying so.
	if (voe_assets_json_member(gltf->json, accessor, "bufferView") ==
	    VOE_ASSETS_JSON_NONE)
		return unsupported(gltf, "an accessor with no buffer view");
	if (gltf->views == VOE_ASSETS_JSON_NONE ||
	    voe_assets_json_count(gltf->json, gltf->views) == 0)
		return malformed(gltf,
				 "an accessor naming a buffer view in a file that describes none");
	if (!uint_member(gltf, accessor, "bufferView",
			 voe_assets_json_count(gltf->json, gltf->views) - 1, 0,
			 &view_index))
		return false;

	view = object_at(gltf, gltf->views, view_index);
	if (view == VOE_ASSETS_JSON_NONE)
		return malformed(gltf, "a buffer view the file does not describe");

	if (!uint_member(gltf, view, "buffer", UINT32_MAX, 0, &buffer_index) ||
	    !check_buffer(gltf, buffer_index))
		return false;
	if (!uint_member(gltf, view, "byteOffset", UINT32_MAX, 0, &view_offset))
		return false;
	if (!uint_member(gltf, view, "byteLength", UINT32_MAX, 0, &view_length))
		return false;
	if (view_length == 0)
		return malformed(gltf, "a buffer view of no bytes");

	// The view has to be inside the chunk before anything inside the view is
	// looked at. Added as sixty-four-bit values, because two thirty-two-bit
	// ones that each fit can still sum to something that does not.
	if ((uint64_t)view_offset + view_length > gltf->bin_size)
		return malformed(gltf,
				 "a buffer view reaching past the end of the binary chunk");

	if (!uint_member(gltf, view, "byteStride", UINT32_MAX, element, &stride))
		return false;
	if (stride < element)
		return malformed(gltf,
				 "a buffer view whose stride is shorter than one element");

	// The last element's last byte, which is where an off-by-one in a stride
	// shows up. count is at least one, so this cannot underflow.
	first = (uint64_t)accessor_offset;
	last = first + (uint64_t)(out->count - 1) * stride + element;
	if (last > view_length)
		return malformed(gltf,
				 "an accessor reaching past the end of its buffer view");

	out->base = gltf->bin + view_offset + accessor_offset;
	out->stride = stride;
	return true;
}

// One element's components as floats. The only conversions here are the ones
// glTF allows for the attributes this reader wants, and every other component
// type is refused with its own message rather than reinterpreted.
static bool read_floats(struct gltf *gltf, const struct view *view,
			uint32_t index, float *out)
{
	const uint8_t *element = view->base + (size_t)index * view->stride;

	if (view->component != COMPONENT_FLOAT)
		return unsupported(gltf,
				   "an attribute stored as something other than floats, which means a normalized or quantized accessor");

	for (uint32_t i = 0; i < view->components; i++) {
		// memcpy and not a cast: the offset inside the buffer is the
		// file's and nothing guarantees a float is aligned there.
		float value;

		memcpy(&value, element + (size_t)i * sizeof(value),
		       sizeof(value));
		out[i] = value;
	}
	return true;
}

// One index, widened. All three of glTF's index types are read, because a file
// picks whichever is narrowest and a loader that only read one would refuse most
// files for no reason.
static uint32_t read_index(const struct view *view, uint32_t index)
{
	const uint8_t *element = view->base + (size_t)index * view->stride;

	switch (view->component) {
	case COMPONENT_UNSIGNED_BYTE:
		return element[0];
	case COMPONENT_UNSIGNED_SHORT: {
		uint16_t value;

		memcpy(&value, element, sizeof(value));
		return value;
	}
	default: {
		uint32_t value;

		memcpy(&value, element, sizeof(value));
		return value;
	}
	}
}

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
		return malformed(gltf, "a texture reference that is not an object");
	if (texture_count == 0)
		return malformed(gltf,
				 "a material naming a texture in a file that describes none");

	if (!uint_member(gltf, reference, "index", texture_count - 1,
			 texture_count, &index))
		return false;
	if (index >= texture_count)
		return malformed(gltf, "a texture reference with no index in it");

	// A second set of texture coordinates is a second attribute nothing
	// here reads. Silently using the first would put a picture on the wrong
	// part of the surface.
	if (!uint_member(gltf, reference, "texCoord", UINT32_MAX, TEXCOORD_SET,
			 &set))
		return false;
	if (set != TEXCOORD_SET)
		return unsupported(gltf,
				   "a material reading a texture coordinate set other than the first");

	texture = object_at(gltf, gltf->textures, index);
	if (texture == VOE_ASSETS_JSON_NONE)
		return malformed(gltf, "a texture the file does not describe");

	// A texture with no source is an extension's — KHR_texture_basisu and
	// its relatives put the image somewhere else — and this reader would
	// otherwise draw it untextured without saying why.
	if (voe_assets_json_member(gltf->json, texture, "source") ==
	    VOE_ASSETS_JSON_NONE)
		return unsupported(gltf,
				   "a texture whose image is provided by an extension");
	if (image_count == 0)
		return malformed(gltf,
				 "a texture naming an image in a file that describes none");
	if (!uint_member(gltf, texture, "source", image_count - 1, image_count,
			 image))
		return false;
	if (*image >= image_count)
		return malformed(gltf, "a texture whose image index is out of range");
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
		return malformed(gltf, "an alphaMode that is not a string");

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

static bool read_materials(struct gltf *gltf, voe_assets_model *model)
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
		uint32_t material = object_at(gltf, gltf->materials, i);
		uint32_t pbr;
		// glTF's defaults, and they are the ones a material that says
		// nothing is defined to have: white, fully metallic, fully
		// rough, no emission, opaque, and a cutoff of a half that only
		// a cutout material ever reads.
		float base[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		float emissive[3] = { 0.0f, 0.0f, 0.0f };
		bool found;

		if (material == VOE_ASSETS_JSON_NONE)
			return malformed(gltf, "a material that is not an object");

		materials[i] = (voe_assets_material){
			.base_colour = { 1.0f, 1.0f, 1.0f, 1.0f },
			.metallic = 1.0f,
			.roughness = 1.0f,
			.alpha_mode = VOE_ASSETS_ALPHA_OPAQUE,
			.alpha_cutoff = 0.5f,
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
				return malformed(gltf,
						 "a pbrMetallicRoughness that is not an object");
			if (!floats_member(gltf, pbr, "baseColorFactor", 4,
					   base, &found) ||
			    !float_member(gltf, pbr, "metallicFactor", 1.0f,
					  &materials[i].metallic) ||
			    !float_member(gltf, pbr, "roughnessFactor", 1.0f,
					  &materials[i].roughness) ||
			    !read_texture_reference(gltf, pbr,
						    "baseColorTexture",
						    &materials[i].base_colour_image) ||
			    !read_texture_reference(gltf, pbr,
						    "metallicRoughnessTexture",
						    &materials[i].metallic_roughness_image))
				return false;
			materials[i].base_colour = (voe_math_float4){
				base[0], base[1], base[2], base[3]
			};
		}

		if (!read_alpha_mode(gltf, material, &materials[i].alpha_mode) ||
		    !float_member(gltf, material, "alphaCutoff", 0.5f,
				  &materials[i].alpha_cutoff) ||
		    !floats_member(gltf, material, "emissiveFactor", 3,
				   emissive, &found) ||
		    !read_texture_reference(gltf, material, "normalTexture",
					    &materials[i].normal_image) ||
		    !read_texture_reference(gltf, material, "occlusionTexture",
					    &materials[i].occlusion_image) ||
		    !read_texture_reference(gltf, material, "emissiveTexture",
					    &materials[i].emissive_image))
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

static bool read_images(struct gltf *gltf, voe_assets_model *model)
{
	voe_assets_image *images;
	uint32_t count = gltf->images == VOE_ASSETS_JSON_NONE ?
				 0 :
				 voe_assets_json_count(gltf->json, gltf->images);

	model->images = NULL;
	model->image_count = 0;
	if (count == 0)
		return true;

	images = voe_base_arena_push(gltf->arena,
				     (size_t)count * sizeof(*images));

	for (uint32_t i = 0; i < count; i++) {
		uint32_t image = object_at(gltf, gltf->images, i);
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
			return malformed(gltf, "an image that is not an object");

		// The same refusal a buffer with a URI gets, and the same
		// reason: nothing here opens a file or decodes base64.
		if (voe_assets_json_member(gltf->json, image, "uri") !=
		    VOE_ASSETS_JSON_NONE)
			return unsupported(gltf,
					   "an image with a URI in it: this reader opens no files and decodes no data URIs");
		if (voe_assets_json_member(gltf->json, image, "bufferView") ==
		    VOE_ASSETS_JSON_NONE)
			return malformed(gltf,
					 "an image with neither a URI nor a buffer view");
		if (view_count == 0)
			return malformed(gltf,
					 "an image naming a buffer view in a file that describes none");
		if (!uint_member(gltf, image, "bufferView", view_count - 1, 0,
				 &view_index))
			return false;

		view = object_at(gltf, gltf->views, view_index);
		if (view == VOE_ASSETS_JSON_NONE)
			return malformed(gltf,
					 "a buffer view the file does not describe");
		if (!uint_member(gltf, view, "buffer", UINT32_MAX, 0,
				 &buffer_index) ||
		    !check_buffer(gltf, buffer_index))
			return false;
		if (!uint_member(gltf, view, "byteOffset", UINT32_MAX, 0,
				 &offset) ||
		    !uint_member(gltf, view, "byteLength", UINT32_MAX, 0,
				 &length))
			return false;
		if (length == 0)
			return malformed(gltf, "an image of no bytes");
		if ((uint64_t)offset + length > gltf->bin_size)
			return malformed(gltf,
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
			return unsupported(gltf,
					   "an image that is neither a PNG nor a JPEG");
		}
	}

	model->images = images;
	model->image_count = count;
	return true;
}

// ---------------------------------------------------------------- meshes

// How many primitives the whole file has, so the array can be one push. Counted
// rather than grown because an arena does not reallocate (rule 11) and two
// pushes are not guaranteed to be adjacent.
static bool count_primitives(struct gltf *gltf, uint32_t *total)
{
	uint32_t meshes = gltf->meshes == VOE_ASSETS_JSON_NONE ?
				  0 :
				  voe_assets_json_count(gltf->json, gltf->meshes);

	*total = 0;
	for (uint32_t i = 0; i < meshes; i++) {
		uint32_t mesh = object_at(gltf, gltf->meshes, i);
		uint32_t primitives;
		uint32_t count;

		if (mesh == VOE_ASSETS_JSON_NONE)
			return malformed(gltf, "a mesh that is not an object");
		if (!array_member(gltf, mesh, "primitives", &primitives, &count))
			return false;
		if (count == 0)
			return malformed(gltf, "a mesh with no primitives in it");
		*total += count;
	}
	return true;
}

// One attribute into an array of float2 or float3, checking that it has as many
// elements as the primitive's positions did.
static bool read_attribute(struct gltf *gltf, uint32_t attributes,
			   const char *name, uint32_t components,
			   uint32_t expected, void *out, bool *present)
{
	uint32_t token = voe_assets_json_member(gltf->json, attributes, name);
	uint32_t accessor_count = gltf->accessors == VOE_ASSETS_JSON_NONE ?
					  0 :
					  voe_assets_json_count(gltf->json,
								gltf->accessors);
	uint32_t index;
	struct view view;

	*present = false;
	if (token == VOE_ASSETS_JSON_NONE)
		return true;
	if (accessor_count == 0)
		return malformed(gltf,
				 "an attribute naming an accessor in a file that describes none");
	if (!voe_assets_json_uint(gltf->json, token, accessor_count - 1, &index))
		return malformed(gltf, "an attribute whose accessor index is out of range");
	if (!resolve_accessor(gltf, index, &view))
		return false;
	if (view.components != components)
		return malformed(gltf,
				 "an attribute whose accessor has the wrong number of components");
	if (expected != 0 && view.count != expected)
		return malformed(gltf,
				 "a primitive whose attributes do not all have the same number of vertices");

	for (uint32_t i = 0; i < view.count; i++) {
		float values[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

		if (!read_floats(gltf, &view, i, values))
			return false;
		if (components == 2)
			((voe_math_float2 *)out)[i] =
				(voe_math_float2){ values[0], values[1] };
		else
			((voe_math_float3 *)out)[i] = (voe_math_float3){
				values[0], values[1], values[2]
			};
	}

	*present = true;
	return true;
}

static bool read_primitive(struct gltf *gltf, uint32_t primitive,
			   uint32_t material_count,
			   voe_assets_primitive *out)
{
	uint32_t accessor_count = gltf->accessors == VOE_ASSETS_JSON_NONE ?
					  0 :
					  voe_assets_json_count(gltf->json,
								gltf->accessors);
	uint32_t attributes;
	uint32_t mode;
	uint32_t position;
	uint32_t indices_token;
	struct view positions;
	voe_math_float3 *points;
	voe_math_float3 *normals;
	voe_math_float2 *uvs;
	uint32_t *indices;
	bool present;

	*out = (voe_assets_primitive){ .material = VOE_ASSETS_MODEL_NONE };

	if (!uint_member(gltf, primitive, "mode", UINT32_MAX, MODE_TRIANGLES,
			 &mode))
		return false;
	if (mode != MODE_TRIANGLES) {
		// The number is in the message because glTF's modes are
		// numbers and a person reading this has to look it up.
		VOE_BASE_ERROR("assets",
			       "glTF: primitive mode %u; this engine draws triangle lists (mode %u) only",
			       mode, MODE_TRIANGLES);
		gltf->error = VOE_BASE_ERROR_UNSUPPORTED;
		return false;
	}

	attributes = voe_assets_json_member(gltf->json, primitive, "attributes");
	if (attributes == VOE_ASSETS_JSON_NONE ||
	    voe_assets_json_kind_of(gltf->json, attributes) !=
		    VOE_ASSETS_JSON_OBJECT)
		return malformed(gltf, "a primitive with no attributes");

	position = voe_assets_json_member(gltf->json, attributes, "POSITION");
	if (position == VOE_ASSETS_JSON_NONE)
		return malformed(gltf, "a primitive with no POSITION attribute");
	if (accessor_count == 0)
		return malformed(gltf,
				 "a primitive naming an accessor in a file that describes none");
	{
		uint32_t index;

		if (!voe_assets_json_uint(gltf->json, position,
					  accessor_count - 1, &index))
			return malformed(gltf,
					 "a POSITION accessor index that is out of range");
		if (!resolve_accessor(gltf, index, &positions))
			return false;
	}
	if (positions.components != 3)
		return malformed(gltf, "a POSITION accessor that is not a VEC3");

	out->vertex_count = positions.count;
	points = voe_base_arena_push(gltf->arena,
				     (size_t)positions.count * sizeof(*points));
	normals = voe_base_arena_push(gltf->arena,
				      (size_t)positions.count *
					      sizeof(*normals));
	uvs = voe_base_arena_push(gltf->arena,
				  (size_t)positions.count * sizeof(*uvs));

	if (!read_attribute(gltf, attributes, "POSITION", 3, 0, points,
			    &present))
		return false;
	out->positions = points;

	if (!read_attribute(gltf, attributes, "NORMAL", 3, positions.count,
			    normals, &present))
		return false;
	out->normals = present ? normals : NULL;

	if (!read_attribute(gltf, attributes, "TEXCOORD_0", 2, positions.count,
			    uvs, &present))
		return false;
	out->uvs = present ? uvs : NULL;

	indices_token = voe_assets_json_member(gltf->json, primitive, "indices");
	if (indices_token == VOE_ASSETS_JSON_NONE) {
		// An unindexed primitive, given the indices it implies. One
		// shape downstream instead of two — see this file's header.
		if (positions.count % 3 != 0)
			return malformed(gltf,
					 "a primitive with no indices whose vertex count is not a multiple of three");
		indices = voe_base_arena_push(gltf->arena,
					      (size_t)positions.count *
						      sizeof(*indices));
		for (uint32_t i = 0; i < positions.count; i++)
			indices[i] = i;
		out->indices = indices;
		out->index_count = positions.count;
	} else {
		struct view view;
		uint32_t index;

		if (!voe_assets_json_uint(gltf->json, indices_token,
					  accessor_count - 1, &index))
			return malformed(gltf,
					 "an indices accessor index that is out of range");
		if (!resolve_accessor(gltf, index, &view))
			return false;
		if (view.components != 1)
			return malformed(gltf,
					 "an indices accessor that is not a SCALAR");
		if (view.component != COMPONENT_UNSIGNED_BYTE &&
		    view.component != COMPONENT_UNSIGNED_SHORT &&
		    view.component != COMPONENT_UNSIGNED_INT)
			return unsupported(gltf,
					   "an indices accessor whose component type is not an unsigned byte, short or int");
		if (view.count % 3 != 0)
			return malformed(gltf,
					 "an index count that is not a multiple of three");

		indices = voe_base_arena_push(gltf->arena,
					      (size_t)view.count *
						      sizeof(*indices));
		for (uint32_t i = 0; i < view.count; i++) {
			indices[i] = read_index(&view, i);

			// EVERY INDEX IS CHECKED AGAINST THE VERTEX COUNT, AND
			// THIS IS THE CHECK THAT MATTERS MOST IN THE FILE. An
			// index past the end of the vertices is a read past the
			// end of a GPU buffer, which is a hostile file's
			// shortest path to something interesting.
			if (indices[i] >= out->vertex_count)
				return malformed(gltf,
						 "an index pointing past the end of the primitive's own vertices");
		}
		out->indices = indices;
		out->index_count = view.count;
	}

	if (voe_assets_json_member(gltf->json, primitive, "material") !=
	    VOE_ASSETS_JSON_NONE) {
		if (material_count == 0)
			return malformed(gltf,
					 "a primitive naming a material in a file that describes none");
		if (!uint_member(gltf, primitive, "material",
				 material_count - 1, 0, &out->material))
			return false;
	}

	return true;
}

static bool read_meshes(struct gltf *gltf, voe_assets_model *model)
{
	uint32_t count = gltf->meshes == VOE_ASSETS_JSON_NONE ?
				 0 :
				 voe_assets_json_count(gltf->json, gltf->meshes);
	voe_assets_mesh *meshes;
	voe_assets_primitive *primitives;
	uint32_t total = 0;
	uint32_t written = 0;

	model->meshes = NULL;
	model->mesh_count = 0;
	model->primitives = NULL;
	model->primitive_count = 0;
	if (count == 0)
		return true;

	if (!count_primitives(gltf, &total))
		return false;

	meshes = voe_base_arena_push(gltf->arena,
				     (size_t)count * sizeof(*meshes));
	primitives = voe_base_arena_push(gltf->arena,
					 (size_t)total * sizeof(*primitives));

	for (uint32_t i = 0; i < count; i++) {
		uint32_t mesh = object_at(gltf, gltf->meshes, i);
		uint32_t array;
		uint32_t primitive_count;

		if (mesh == VOE_ASSETS_JSON_NONE)
			return malformed(gltf, "a mesh that is not an object");
		if (!array_member(gltf, mesh, "primitives", &array,
				  &primitive_count))
			return false;

		meshes[i].first_primitive = written;
		meshes[i].primitive_count = primitive_count;

		for (uint32_t p = 0; p < primitive_count; p++) {
			uint32_t primitive = object_at(gltf, array, p);

			if (primitive == VOE_ASSETS_JSON_NONE)
				return malformed(gltf,
						 "a primitive that is not an object");
			if (!read_primitive(gltf, primitive,
					    model->material_count,
					    &primitives[written]))
				return false;
			written++;
		}
	}

	model->meshes = meshes;
	model->mesh_count = count;
	model->primitives = primitives;
	model->primitive_count = total;
	return true;
}

// ----------------------------------------------------------------- nodes

// glTF stores a matrix column-major and this engine stores one row-major, so
// every element moves. Layout only — no coordinate changes, and none may be
// added (ADR-0033, ADR-0035). Confusing the two is the bug that makes a model
// look mirrored and correct depending on what it is compared against.
static voe_math_float4x4 transposed(const float *column_major)
{
	voe_math_float4x4 matrix;

	for (uint32_t row = 0; row < 4; row++) {
		for (uint32_t column = 0; column < 4; column++)
			matrix.m[row][column] = column_major[column * 4 + row];
	}
	return matrix;
}

// Translation, rotation and scale composed the way the transform component
// composes them: T · R · S, read right to left, so a point is scaled first.
static voe_math_float4x4 composed(const float *translation,
				  const float *rotation, const float *scale)
{
	voe_math_quat quaternion = { rotation[0], rotation[1], rotation[2],
				     rotation[3] };
	voe_math_float3 position = { translation[0], translation[1],
				     translation[2] };
	voe_math_float3 sizes = { scale[0], scale[1], scale[2] };

	return voe_math_float4x4_mul(
		voe_math_float4x4_from_translation(position),
		voe_math_float4x4_mul(voe_math_float4x4_from_quat(quaternion),
				      voe_math_float4x4_from_scale(sizes)));
}

static bool read_nodes(struct gltf *gltf, voe_assets_model *model)
{
	uint32_t count = gltf->nodes == VOE_ASSETS_JSON_NONE ?
				 0 :
				 voe_assets_json_count(gltf->json, gltf->nodes);
	voe_assets_node *nodes;
	uint8_t *claimed;
	uint32_t roots;
	uint32_t root_count = 0;
	uint32_t *root_list;

	model->nodes = NULL;
	model->node_count = 0;
	model->roots = NULL;
	model->root_count = 0;
	if (count == 0)
		return true;

	nodes = voe_base_arena_push(gltf->arena,
				    (size_t)count * sizeof(*nodes));
	// One byte per node saying whether something already claims it as a
	// child. It is what makes the tree a tree — see below.
	claimed = voe_base_arena_push(gltf->arena, count);

	for (uint32_t i = 0; i < count; i++) {
		uint32_t node = object_at(gltf, gltf->nodes, i);
		uint32_t children;
		uint32_t child_count;
		float matrix[16];
		float translation[3] = { 0.0f, 0.0f, 0.0f };
		float rotation[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		float scale[3] = { 1.0f, 1.0f, 1.0f };
		bool has_matrix;
		bool found;
		uint32_t *child_list;

		if (node == VOE_ASSETS_JSON_NONE)
			return malformed(gltf, "a node that is not an object");

		nodes[i].mesh = VOE_ASSETS_MODEL_NONE;
		if (voe_assets_json_member(gltf->json, node, "mesh") !=
		    VOE_ASSETS_JSON_NONE) {
			if (model->mesh_count == 0)
				return malformed(gltf,
						 "a node naming a mesh in a file that describes none");
			if (!uint_member(gltf, node, "mesh",
					 model->mesh_count - 1, 0,
					 &nodes[i].mesh))
				return false;
		}

		// A node gives either a matrix or the three parts, and glTF says
		// a matrix wins. Composing the parts here means the importer
		// sees one shape.
		if (!floats_member(gltf, node, "matrix", 16, matrix,
				   &has_matrix))
			return false;
		if (has_matrix) {
			nodes[i].local = transposed(matrix);
		} else {
			if (!floats_member(gltf, node, "translation", 3,
					   translation, &found) ||
			    !floats_member(gltf, node, "rotation", 4, rotation,
					   &found) ||
			    !floats_member(gltf, node, "scale", 3, scale,
					   &found))
				return false;
			nodes[i].local = composed(translation, rotation, scale);
		}

		if (!array_member(gltf, node, "children", &children,
				  &child_count))
			return false;

		nodes[i].children = NULL;
		nodes[i].child_count = child_count;
		if (child_count == 0)
			continue;

		child_list = voe_base_arena_push(
			gltf->arena, (size_t)child_count * sizeof(*child_list));
		for (uint32_t c = 0; c < child_count; c++) {
			uint32_t child;

			if (!voe_assets_json_uint(
				    gltf->json,
				    voe_assets_json_element(gltf->json,
							    children, c),
				    count - 1, &child))
				return malformed(gltf,
						 "a child index that is not a node this file describes");

			// A NODE CLAIMED TWICE IS NOT A TREE, AND REFUSING IT
			// HERE IS WHAT MAKES A WALK OF IT SAFE. Two parents is
			// how a file describes a cycle — a is b's child and b
			// is a's — and a walk of that never ends. glTF forbids
			// it; a hostile or broken file does not care, and the
			// importer's depth limit should not be the only thing
			// standing in the way.
			if (child == i)
				return malformed(gltf, "a node that is its own child");
			if (claimed[child])
				return malformed(gltf,
						 "a node claimed as a child by two parents, which is not a tree");
			claimed[child] = 1;
			child_list[c] = child;
		}
		nodes[i].children = child_list;
	}

	model->nodes = nodes;
	model->node_count = count;

	// The file's own answer, when it gives one: the default scene's nodes.
	if (gltf->scenes != VOE_ASSETS_JSON_NONE &&
	    voe_assets_json_count(gltf->json, gltf->scenes) > 0) {
		uint32_t scene_count = voe_assets_json_count(gltf->json,
							     gltf->scenes);
		uint32_t which;
		uint32_t scene;

		if (!uint_member(gltf, 0, "scene", scene_count - 1, 0, &which))
			return false;
		scene = object_at(gltf, gltf->scenes, which);
		if (scene == VOE_ASSETS_JSON_NONE)
			return malformed(gltf, "a scene that is not an object");
		if (!array_member(gltf, scene, "nodes", &roots, &root_count))
			return false;

		root_list = voe_base_arena_push(
			gltf->arena,
			(size_t)(root_count == 0 ? 1 : root_count) *
				sizeof(*root_list));
		for (uint32_t i = 0; i < root_count; i++) {
			if (!voe_assets_json_uint(
				    gltf->json,
				    voe_assets_json_element(gltf->json, roots,
							    i),
				    count - 1, &root_list[i]))
				return malformed(gltf,
						 "a scene naming a node this file does not describe");
			if (claimed[root_list[i]])
				return malformed(gltf,
						 "a scene naming a node that is already something else's child");
		}
		model->roots = root_list;
		model->root_count = root_count;
		return true;
	}

	// No scene: every node nothing claims. A file without a scene is
	// unusual and legal, and this is the only reading that draws all of it.
	root_list = voe_base_arena_push(gltf->arena,
					(size_t)count * sizeof(*root_list));
	for (uint32_t i = 0; i < count; i++) {
		if (!claimed[i])
			root_list[root_count++] = i;
	}
	model->roots = root_list;
	model->root_count = root_count;
	return true;
}

// ------------------------------------------------------------- the reader

// glTF 2.0 and nothing else. The version is a string in the file — "2.0" today —
// and a 1.0 file is a different format with the same extension, which is exactly
// the kind of thing to say out loud rather than to fail on halfway through.
static bool check_version(struct gltf *gltf)
{
	uint32_t asset = voe_assets_json_member(gltf->json, 0, "asset");
	uint32_t version;
	const voe_assets_json_token *token;

	if (asset == VOE_ASSETS_JSON_NONE ||
	    voe_assets_json_kind_of(gltf->json, asset) != VOE_ASSETS_JSON_OBJECT)
		return malformed(gltf, "a glTF with no asset block");

	version = voe_assets_json_member(gltf->json, asset, "version");
	if (version == VOE_ASSETS_JSON_NONE ||
	    voe_assets_json_kind_of(gltf->json, version) !=
		    VOE_ASSETS_JSON_STRING)
		return malformed(gltf, "a glTF that does not say which version it is");

	// Any 2.x, because the minor version is additive by glTF's own rules.
	token = &gltf->json->tokens[version];
	if (token->end - token->start < 1 ||
	    gltf->json->text[token->start] != '2')
		return unsupported(gltf,
				   "a glTF that is not version 2, which is a different format with the same extension");
	return true;
}

// An extension the file says it cannot be read without. Ignoring one produces a
// model that is quietly wrong — a compressed mesh read as though it were not, a
// texture that is not where it says — so it is refused, and the message names
// which one so that the answer is "this file needs X" rather than "no".
static bool check_extensions(struct gltf *gltf)
{
	uint32_t required;
	uint32_t count;

	if (!array_member(gltf, 0, "extensionsRequired", &required, &count))
		return false;
	if (count == 0)
		return true;

	for (uint32_t i = 0; i < count; i++) {
		uint32_t name = voe_assets_json_element(gltf->json, required, i);
		const voe_assets_json_token *token;

		if (name == VOE_ASSETS_JSON_NONE ||
		    voe_assets_json_kind_of(gltf->json, name) !=
			    VOE_ASSETS_JSON_STRING)
			return malformed(gltf,
					 "a required extension that is not a string");

		token = &gltf->json->tokens[name];
		VOE_BASE_ERROR("assets",
			       "glTF: this file requires the extension %.*s, which this reader does not implement",
			       (int)(token->end - token->start),
			       gltf->json->text + token->start);
	}

	gltf->error = VOE_BASE_ERROR_UNSUPPORTED;
	return false;
}

bool voe_assets_model_from_gltf(const voe_assets_json *json,
				const uint8_t *bin, size_t bin_size,
				voe_base_arena *arena, voe_assets_model *model,
				voe_base_error *error)
{
	struct gltf gltf = {
		.json = json,
		.bin = bin,
		.bin_size = bin_size,
		.arena = arena,
		.error = VOE_BASE_OK,
	};
	voe_assets_model read = { 0 };
	uint32_t ignored;

	VOE_BASE_ASSERT(json != NULL, "reading a glTF from no document");
	VOE_BASE_ASSERT(arena != NULL, "reading a glTF without an arena");
	VOE_BASE_ASSERT(model != NULL, "reading a glTF into nothing");

	if (json->count == 0 ||
	    voe_assets_json_kind_of(json, 0) != VOE_ASSETS_JSON_OBJECT) {
		gltf.error = VOE_BASE_ERROR_MALFORMED;
		VOE_BASE_ERROR("assets", "glTF: the description is not an object");
		if (error != NULL)
			*error = gltf.error;
		return false;
	}

	if (!array_member(&gltf, 0, "accessors", &gltf.accessors, &ignored) ||
	    !array_member(&gltf, 0, "bufferViews", &gltf.views, &ignored) ||
	    !array_member(&gltf, 0, "buffers", &gltf.buffers, &ignored) ||
	    !array_member(&gltf, 0, "materials", &gltf.materials, &ignored) ||
	    !array_member(&gltf, 0, "textures", &gltf.textures, &ignored) ||
	    !array_member(&gltf, 0, "images", &gltf.images, &ignored) ||
	    !array_member(&gltf, 0, "meshes", &gltf.meshes, &ignored) ||
	    !array_member(&gltf, 0, "nodes", &gltf.nodes, &ignored) ||
	    !array_member(&gltf, 0, "scenes", &gltf.scenes, &ignored))
		goto failed;

	// The order is what each step needs and nothing else: a primitive's
	// material index is checked against the materials, a node's mesh index
	// against the meshes. The images are decoded last because they are the
	// expensive part and a file that is wrong somewhere else should say so
	// before it spends that.
	if (!check_version(&gltf) || !check_extensions(&gltf) ||
	    !read_materials(&gltf, &read) || !read_meshes(&gltf, &read) ||
	    !read_nodes(&gltf, &read) || !read_images(&gltf, &read))
		goto failed;

	*model = read;
	return true;

failed:
	if (error != NULL)
		*error = gltf.error;
	return false;
}
