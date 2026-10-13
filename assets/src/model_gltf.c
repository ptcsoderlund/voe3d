// glTF's description, read into the arrays in assets/model.h. This file holds
// the helpers every part shares (model_gltf_read.h), accessors and buffer views,
// the version and extension checks, and the entry point that calls the parts in
// order: model_gltf_materials.c (materials and images), model_gltf_meshes.c
// (meshes and primitives) and model_gltf_nodes.c (the node tree).
//
// EVERY LENGTH, COUNT AND INDEX READ OUT OF A FILE IS CHECKED BEFORE IT IS
// USED, AND THAT IS THE RULE EVERY PART FOLLOWS. A glTF says how many
// vertices an accessor has, where in a buffer view it starts and how far apart
// its elements are, and every one of those numbers is a file's claim rather than
// a fact. So: an index is read with a limit beside it, an offset and a length are
// added in sixty-four bits and compared against the chunk that has to hold them,
// and every index inside an index buffer is compared against the vertex count it
// has to be inside. png.c states the same rule for the same reason.
//
// THERE IS NO RECURSION IN ANY PART (rule 14). The node tree is read as a flat
// array with a parent count beside it — see model_gltf_nodes.c. The JSON
// underneath is read by a parser with an explicit stack for the same reason —
// see json.h.
//
// THE ONLY BUFFER IS THE `.glb`'S BINARY CHUNK. A glTF buffer with a `uri` names
// an external file or carries base64, and neither is read here: `platform` has
// no file API and `.glb` was chosen precisely so that there is nothing to
// resolve. A file that asks is refused as unsupported, by name.
//
// SKINS AND ANIMATIONS ARE NOT READ AND ARE NOT REFUSED. A skinned mesh's
// JOINTS_0 and WEIGHTS_0 attributes are ignored, so the mesh arrives in its bind
// pose; animation channels are not looked at. Refusing would turn most files
// people have into an error, and the card that reads them is a later one.
#include "model_gltf.h"
#include "model_gltf_read.h"

#include <base/assert.h>
#include <base/report.h>

#include <string.h>

static bool refuse(struct gltf *gltf, voe_base_error error,
		   const char *message)
{
	VOE_BASE_ERROR("assets", "glTF: %s", message);
	gltf->error = error;
	return false;
}

bool voe_assets_gltf_malformed(struct gltf *gltf, const char *message)
{
	return refuse(gltf, VOE_BASE_ERROR_MALFORMED, message);
}

bool voe_assets_gltf_unsupported(struct gltf *gltf, const char *message)
{
	return refuse(gltf, VOE_BASE_ERROR_UNSUPPORTED, message);
}

// A member that has to be an array if it is there at all. count comes back as
// nought when it is not there, so a caller loops over nothing rather than
// branching.
bool voe_assets_gltf_array_member(struct gltf *gltf, uint32_t object,
				  const char *name, uint32_t *token,
				  uint32_t *count)
{
	uint32_t found = voe_assets_json_member(gltf->json, object, name);

	*token = VOE_ASSETS_JSON_NONE;
	*count = 0;
	if (found == VOE_ASSETS_JSON_NONE)
		return true;
	if (voe_assets_json_kind_of(gltf->json, found) !=
	    VOE_ASSETS_JSON_ARRAY)
		return voe_assets_gltf_malformed(
			gltf, "a property that has to be an array and is not");

	*token = found;
	*count = voe_assets_json_count(gltf->json, found);
	return true;
}

// An object out of one of the document's arrays, by index. The index has already
// been checked against the array's length by whoever read it.
uint32_t voe_assets_gltf_object_at(struct gltf *gltf, uint32_t array,
				   uint32_t index)
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
bool voe_assets_gltf_uint_member(struct gltf *gltf, uint32_t object,
				 const char *name, uint32_t limit,
				 uint32_t fallback, uint32_t *out)
{
	uint32_t token = voe_assets_json_member(gltf->json, object, name);

	*out = fallback;
	if (token == VOE_ASSETS_JSON_NONE)
		return true;
	if (!voe_assets_json_uint(gltf->json, token, limit, out))
		return voe_assets_gltf_malformed(
			gltf,
			"a count or an index that is not a whole number inside its range");
	return true;
}

bool voe_assets_gltf_float_member(struct gltf *gltf, uint32_t object,
				  const char *name, float fallback, float *out)
{
	uint32_t token = voe_assets_json_member(gltf->json, object, name);

	*out = fallback;
	if (token == VOE_ASSETS_JSON_NONE)
		return true;
	if (!voe_assets_json_float(gltf->json, token, out))
		return voe_assets_gltf_malformed(gltf,
						 "a factor that is not a number");
	return true;
}

// A fixed-length array of numbers — a colour, a translation, a matrix. Absent is
// not an error and leaves `out` as the caller set it.
bool voe_assets_gltf_floats_member(struct gltf *gltf, uint32_t object,
				   const char *name, uint32_t length,
				   float *out, bool *found)
{
	uint32_t token = voe_assets_json_member(gltf->json, object, name);

	*found = false;
	if (token == VOE_ASSETS_JSON_NONE)
		return true;
	if (voe_assets_json_kind_of(gltf->json, token) != VOE_ASSETS_JSON_ARRAY)
		return voe_assets_gltf_malformed(gltf,
						 "a vector that is not an array");
	if (voe_assets_json_count(gltf->json, token) != length)
		return voe_assets_gltf_malformed(gltf,
						 "a vector of the wrong length");

	for (uint32_t i = 0; i < length; i++) {
		if (!voe_assets_json_float(
			    gltf->json,
			    voe_assets_json_element(gltf->json, token, i),
			    &out[i]))
			return voe_assets_gltf_malformed(
				gltf,
				"a vector with something that is not a number in it");
	}

	*found = true;
	return true;
}

// ------------------------------------------------------------- accessors

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
bool voe_assets_gltf_check_buffer(struct gltf *gltf, uint32_t index)
{
	uint32_t buffer;
	uint32_t length;

	if (index != 0)
		return voe_assets_gltf_unsupported(
			gltf,
			"a buffer other than the file's own binary chunk, which means an external file");

	buffer = voe_assets_gltf_object_at(gltf, gltf->buffers, 0);
	if (buffer == VOE_ASSETS_JSON_NONE)
		return voe_assets_gltf_malformed(
			gltf,
			"a buffer view naming a buffer the file does not describe");
	if (voe_assets_json_member(gltf->json, buffer, "uri") !=
	    VOE_ASSETS_JSON_NONE)
		return voe_assets_gltf_unsupported(
			gltf,
			"a buffer with a URI in it: this reader opens no files and decodes no data URIs");
	if (!voe_assets_gltf_uint_member(gltf, buffer, "byteLength", UINT32_MAX,
					 0, &length))
		return false;
	if (length > gltf->bin_size)
		return voe_assets_gltf_malformed(
			gltf,
			"a buffer that claims to be longer than the binary chunk holding it");
	return true;
}

bool voe_assets_gltf_resolve_accessor(struct gltf *gltf, uint32_t index,
				      struct view *out)
{
	uint32_t accessor = voe_assets_gltf_object_at(gltf, gltf->accessors,
						      index);
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
		return voe_assets_gltf_malformed(
			gltf, "an accessor the file does not describe");

	// A sparse accessor is a base array with a list of overrides on top of
	// it. It is a different reader and producing the base array alone would
	// be a quietly wrong mesh.
	if (voe_assets_json_member(gltf->json, accessor, "sparse") !=
	    VOE_ASSETS_JSON_NONE)
		return voe_assets_gltf_unsupported(gltf, "a sparse accessor");

	if (!voe_assets_gltf_uint_member(gltf, accessor, "componentType",
					 UINT32_MAX, 0, &out->component))
		return false;
	element = component_size(out->component);
	if (element == 0)
		return voe_assets_gltf_unsupported(
			gltf,
			"an accessor whose component type this reader does not know");

	type_token = voe_assets_json_member(gltf->json, accessor, "type");
	if (type_token == VOE_ASSETS_JSON_NONE)
		return voe_assets_gltf_malformed(gltf,
						 "an accessor with no type");
	out->components = components_of(gltf->json, type_token);
	if (out->components == 0)
		return voe_assets_gltf_unsupported(
			gltf,
			"an accessor of a type this reader does not read, which means a matrix");
	element *= out->components;

	// An element is at least one byte, so an accessor cannot have more
	// elements than the binary chunk has bytes. That is the limit, and it is
	// what keeps the arithmetic below inside sixty-four bits by
	// construction.
	if (!voe_assets_gltf_uint_member(gltf, accessor, "count",
					 (uint32_t)(gltf->bin_size > UINT32_MAX ?
							    UINT32_MAX :
							    gltf->bin_size),
					 0, &out->count))
		return false;
	if (out->count == 0)
		return voe_assets_gltf_malformed(
			gltf, "an accessor with no elements in it");

	if (!voe_assets_gltf_uint_member(gltf, accessor, "byteOffset",
					 UINT32_MAX, 0, &accessor_offset))
		return false;

	// An accessor with no buffer view is defined as all zeros. Nothing here
	// wants a mesh made of zeros, and pretending otherwise would draw a
	// point at the origin instead of saying so.
	if (voe_assets_json_member(gltf->json, accessor, "bufferView") ==
	    VOE_ASSETS_JSON_NONE)
		return voe_assets_gltf_unsupported(
			gltf, "an accessor with no buffer view");
	if (gltf->views == VOE_ASSETS_JSON_NONE ||
	    voe_assets_json_count(gltf->json, gltf->views) == 0)
		return voe_assets_gltf_malformed(
			gltf,
			"an accessor naming a buffer view in a file that describes none");
	if (!voe_assets_gltf_uint_member(
		    gltf, accessor, "bufferView",
		    voe_assets_json_count(gltf->json, gltf->views) - 1, 0,
		    &view_index))
		return false;

	view = voe_assets_gltf_object_at(gltf, gltf->views, view_index);
	if (view == VOE_ASSETS_JSON_NONE)
		return voe_assets_gltf_malformed(
			gltf, "a buffer view the file does not describe");

	if (!voe_assets_gltf_uint_member(gltf, view, "buffer", UINT32_MAX, 0,
					 &buffer_index) ||
	    !voe_assets_gltf_check_buffer(gltf, buffer_index))
		return false;
	if (!voe_assets_gltf_uint_member(gltf, view, "byteOffset", UINT32_MAX,
					 0, &view_offset))
		return false;
	if (!voe_assets_gltf_uint_member(gltf, view, "byteLength", UINT32_MAX,
					 0, &view_length))
		return false;
	if (view_length == 0)
		return voe_assets_gltf_malformed(gltf,
						 "a buffer view of no bytes");

	// The view has to be inside the chunk before anything inside the view is
	// looked at. Added as sixty-four-bit values, because two thirty-two-bit
	// ones that each fit can still sum to something that does not.
	if ((uint64_t)view_offset + view_length > gltf->bin_size)
		return voe_assets_gltf_malformed(
			gltf,
			"a buffer view reaching past the end of the binary chunk");

	if (!voe_assets_gltf_uint_member(gltf, view, "byteStride", UINT32_MAX,
					 element, &stride))
		return false;
	if (stride < element)
		return voe_assets_gltf_malformed(
			gltf,
			"a buffer view whose stride is shorter than one element");

	// The last element's last byte, which is where an off-by-one in a stride
	// shows up. count is at least one, so this cannot underflow.
	first = (uint64_t)accessor_offset;
	last = first + (uint64_t)(out->count - 1) * stride + element;
	if (last > view_length)
		return voe_assets_gltf_malformed(
			gltf,
			"an accessor reaching past the end of its buffer view");

	out->base = gltf->bin + view_offset + accessor_offset;
	out->stride = stride;
	return true;
}

// One element's components as floats. The only conversions here are the ones
// glTF allows for the attributes this reader wants, and every other component
// type is refused with its own message rather than reinterpreted.
bool voe_assets_gltf_read_floats(struct gltf *gltf, const struct view *view,
				 uint32_t index, float *out)
{
	const uint8_t *element = view->base + (size_t)index * view->stride;

	if (view->component != COMPONENT_FLOAT)
		return voe_assets_gltf_unsupported(
			gltf,
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
uint32_t voe_assets_gltf_read_index(const struct view *view, uint32_t index)
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
		return voe_assets_gltf_malformed(gltf,
						 "a glTF with no asset block");

	version = voe_assets_json_member(gltf->json, asset, "version");
	if (version == VOE_ASSETS_JSON_NONE ||
	    voe_assets_json_kind_of(gltf->json, version) !=
		    VOE_ASSETS_JSON_STRING)
		return voe_assets_gltf_malformed(
			gltf, "a glTF that does not say which version it is");

	// Any 2.x, because the minor version is additive by glTF's own rules.
	token = &gltf->json->tokens[version];
	if (token->end - token->start < 1 ||
	    gltf->json->text[token->start] != '2')
		return voe_assets_gltf_unsupported(
			gltf,
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

	if (!voe_assets_gltf_array_member(gltf, 0, "extensionsRequired",
					  &required, &count))
		return false;
	if (count == 0)
		return true;

	for (uint32_t i = 0; i < count; i++) {
		uint32_t name = voe_assets_json_element(gltf->json, required, i);
		const voe_assets_json_token *token;

		if (name == VOE_ASSETS_JSON_NONE ||
		    voe_assets_json_kind_of(gltf->json, name) !=
			    VOE_ASSETS_JSON_STRING)
			return voe_assets_gltf_malformed(
				gltf, "a required extension that is not a string");

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

	if (!voe_assets_gltf_array_member(&gltf, 0, "accessors",
					  &gltf.accessors, &ignored) ||
	    !voe_assets_gltf_array_member(&gltf, 0, "bufferViews", &gltf.views,
					  &ignored) ||
	    !voe_assets_gltf_array_member(&gltf, 0, "buffers", &gltf.buffers,
					  &ignored) ||
	    !voe_assets_gltf_array_member(&gltf, 0, "materials",
					  &gltf.materials, &ignored) ||
	    !voe_assets_gltf_array_member(&gltf, 0, "textures", &gltf.textures,
					  &ignored) ||
	    !voe_assets_gltf_array_member(&gltf, 0, "images", &gltf.images,
					  &ignored) ||
	    !voe_assets_gltf_array_member(&gltf, 0, "meshes", &gltf.meshes,
					  &ignored) ||
	    !voe_assets_gltf_array_member(&gltf, 0, "nodes", &gltf.nodes,
					  &ignored) ||
	    !voe_assets_gltf_array_member(&gltf, 0, "scenes", &gltf.scenes,
					  &ignored))
		goto failed;

	// The order is what each step needs and nothing else: a primitive's
	// material index is checked against the materials, a node's mesh index
	// against the meshes. The images are decoded last because they are the
	// expensive part and a file that is wrong somewhere else should say so
	// before it spends that.
	if (!check_version(&gltf) || !check_extensions(&gltf) ||
	    !voe_assets_gltf_read_materials(&gltf, &read) ||
	    !voe_assets_gltf_read_meshes(&gltf, &read) ||
	    !voe_assets_gltf_read_nodes(&gltf, &read) ||
	    !voe_assets_gltf_read_images(&gltf, &read))
		goto failed;

	*model = read;
	return true;

failed:
	if (error != NULL)
		*error = gltf.error;
	return false;
}
