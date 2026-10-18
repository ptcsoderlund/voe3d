// glTF's meshes, read into the arrays in assets/model.h: attributes, primitives
// and the meshes holding them, all primitives in one array. Called by
// model_gltf.c through model_gltf_read.h after the materials, whose count a
// primitive's material index is checked against; that header's rules hold here.
//
// TRIANGLE LISTS ONLY, AND AN UNINDEXED PRIMITIVE IS GIVEN INDICES ON THE WAY
// IN. Strips, fans, lines and points are refused with their mode number in the
// message. Generating the indices for a primitive that has none costs one array
// and means everything downstream — the pool, the draw, the importer — has
// exactly one shape to handle. Indices are all an expansion adds, so every
// attribute, COLOR_0 included, comes through it as the file had it.
//
// COLOR_0 IS READ AS glTF ALLOWS IT AND NO WIDER: VEC3 or VEC4 of floats, or of
// unsigned bytes or shorts marked normalized, all turned into float 0 to 1.
// Any other component type or shape is unsupported, with both numbers in the
// message.
#include "model_gltf_read.h"

#include <base/report.h>

#include <string.h>

// The only primitive mode this engine draws.
#define MODE_TRIANGLES 4

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
		uint32_t mesh = voe_assets_gltf_object_at(gltf, gltf->meshes, i);
		uint32_t primitives;
		uint32_t count;

		if (mesh == VOE_ASSETS_JSON_NONE)
			return voe_assets_gltf_malformed(
				gltf, "a mesh that is not an object");
		if (!voe_assets_gltf_array_member(gltf, mesh, "primitives",
						  &primitives, &count))
			return false;
		if (count == 0)
			return voe_assets_gltf_malformed(
				gltf, "a mesh with no primitives in it");
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
		return voe_assets_gltf_malformed(
			gltf,
			"an attribute naming an accessor in a file that describes none");
	if (!voe_assets_json_uint(gltf->json, token, accessor_count - 1, &index))
		return voe_assets_gltf_malformed(
			gltf, "an attribute whose accessor index is out of range");
	if (!voe_assets_gltf_resolve_accessor(gltf, index, &view))
		return false;
	if (view.components != components)
		return voe_assets_gltf_malformed(
			gltf,
			"an attribute whose accessor has the wrong number of components");
	if (expected != 0 && view.count != expected)
		return voe_assets_gltf_malformed(
			gltf,
			"a primitive whose attributes do not all have the same number of vertices");

	for (uint32_t i = 0; i < view.count; i++) {
		float values[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

		if (!voe_assets_gltf_read_floats(gltf, &view, i, values))
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

// COLOR_0's one element as a float4: floats as they are, a normalized unsigned
// byte or short divided by its largest value, and alpha 1 for a VEC3.
static voe_math_float4 read_colour(const struct view *view, uint32_t index)
{
	const uint8_t *element = view->base + (size_t)index * view->stride;
	float values[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

	for (uint32_t c = 0; c < view->components; c++) {
		if (view->component == COMPONENT_FLOAT) {
			memcpy(&values[c], element + (size_t)c * sizeof(float),
			       sizeof(float));
		} else if (view->component == COMPONENT_UNSIGNED_SHORT) {
			uint16_t value;

			memcpy(&value, element + (size_t)c * sizeof(value),
			       sizeof(value));
			values[c] = (float)value / 65535.0f;
		} else {
			values[c] = (float)element[c] / 255.0f;
		}
	}
	return (voe_math_float4){ values[0], values[1], values[2], values[3] };
}

// COLOR_0 into `*out`, or NULL when the primitive has none. The shapes glTF
// allows a colour are the ones read; anything else names itself and is refused.
static bool read_colours(struct gltf *gltf, uint32_t attributes,
			 uint32_t accessor_count, uint32_t expected,
			 const voe_math_float4 **out)
{
	uint32_t token = voe_assets_json_member(gltf->json, attributes,
						"COLOR_0");
	uint32_t index;
	uint32_t flag;
	bool normalized;
	bool readable;
	struct view view;
	voe_math_float4 *colours;

	*out = NULL;
	if (token == VOE_ASSETS_JSON_NONE)
		return true;
	if (accessor_count == 0 ||
	    !voe_assets_json_uint(gltf->json, token, accessor_count - 1, &index))
		return voe_assets_gltf_malformed(
			gltf, "a COLOR_0 accessor index that is out of range");
	if (!voe_assets_gltf_resolve_accessor(gltf, index, &view))
		return false;

	flag = voe_assets_json_member(
		gltf->json, voe_assets_gltf_object_at(gltf, gltf->accessors, index),
		"normalized");
	normalized = flag != VOE_ASSETS_JSON_NONE &&
		     voe_assets_json_kind_of(gltf->json, flag) ==
			     VOE_ASSETS_JSON_TRUE;
	readable = view.component == COMPONENT_FLOAT ||
		   (normalized && (view.component == COMPONENT_UNSIGNED_BYTE ||
				   view.component == COMPONENT_UNSIGNED_SHORT));
	if (!readable || (view.components != 3 && view.components != 4)) {
		VOE_BASE_ERROR("assets",
			       "glTF: a COLOR_0 of %u components of component type %u, %s; this reader reads VEC3 or VEC4 of floats (5126) or of normalized unsigned bytes (5121) or shorts (5123)",
			       view.components, view.component,
			       normalized ? "normalized" : "not normalized");
		gltf->error = VOE_BASE_ERROR_UNSUPPORTED;
		return false;
	}
	if (view.count != expected)
		return voe_assets_gltf_malformed(
			gltf,
			"a primitive whose attributes do not all have the same number of vertices");

	colours = voe_base_arena_push(gltf->arena,
				      (size_t)view.count * sizeof(*colours));
	for (uint32_t i = 0; i < view.count; i++)
		colours[i] = read_colour(&view, i);
	*out = colours;
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

	if (!voe_assets_gltf_uint_member(gltf, primitive, "mode", UINT32_MAX,
					 MODE_TRIANGLES, &mode))
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
		return voe_assets_gltf_malformed(gltf,
						 "a primitive with no attributes");

	position = voe_assets_json_member(gltf->json, attributes, "POSITION");
	if (position == VOE_ASSETS_JSON_NONE)
		return voe_assets_gltf_malformed(
			gltf, "a primitive with no POSITION attribute");
	if (accessor_count == 0)
		return voe_assets_gltf_malformed(
			gltf,
			"a primitive naming an accessor in a file that describes none");
	{
		uint32_t index;

		if (!voe_assets_json_uint(gltf->json, position,
					  accessor_count - 1, &index))
			return voe_assets_gltf_malformed(
				gltf,
				"a POSITION accessor index that is out of range");
		if (!voe_assets_gltf_resolve_accessor(gltf, index, &positions))
			return false;
	}
	if (positions.components != 3)
		return voe_assets_gltf_malformed(
			gltf, "a POSITION accessor that is not a VEC3");

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

	if (!read_colours(gltf, attributes, accessor_count, positions.count,
			  &out->colours))
		return false;

	indices_token = voe_assets_json_member(gltf->json, primitive, "indices");
	if (indices_token == VOE_ASSETS_JSON_NONE) {
		// An unindexed primitive, given the indices it implies. One
		// shape downstream instead of two — see this file's header.
		if (positions.count % 3 != 0)
			return voe_assets_gltf_malformed(
				gltf,
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
			return voe_assets_gltf_malformed(
				gltf,
				"an indices accessor index that is out of range");
		if (!voe_assets_gltf_resolve_accessor(gltf, index, &view))
			return false;
		if (view.components != 1)
			return voe_assets_gltf_malformed(
				gltf, "an indices accessor that is not a SCALAR");
		if (view.component != COMPONENT_UNSIGNED_BYTE &&
		    view.component != COMPONENT_UNSIGNED_SHORT &&
		    view.component != COMPONENT_UNSIGNED_INT)
			return voe_assets_gltf_unsupported(
				gltf,
				"an indices accessor whose component type is not an unsigned byte, short or int",
				"the file numbers its triangles' corners with numbers of type %u, which the engine does not read; export it with Blender's own glTF exporter",
				view.component);
		if (view.count % 3 != 0)
			return voe_assets_gltf_malformed(
				gltf,
				"an index count that is not a multiple of three");

		indices = voe_base_arena_push(gltf->arena,
					      (size_t)view.count *
						      sizeof(*indices));
		for (uint32_t i = 0; i < view.count; i++) {
			indices[i] = voe_assets_gltf_read_index(&view, i);

			// EVERY INDEX IS CHECKED AGAINST THE VERTEX COUNT, AND
			// THIS IS THE CHECK THAT MATTERS MOST IN THE FILE. An
			// index past the end of the vertices is a read past the
			// end of a GPU buffer, which is a hostile file's
			// shortest path to something interesting.
			if (indices[i] >= out->vertex_count)
				return voe_assets_gltf_malformed(
					gltf,
					"an index pointing past the end of the primitive's own vertices");
		}
		out->indices = indices;
		out->index_count = view.count;
	}

	if (voe_assets_json_member(gltf->json, primitive, "material") !=
	    VOE_ASSETS_JSON_NONE) {
		if (material_count == 0)
			return voe_assets_gltf_malformed(
				gltf,
				"a primitive naming a material in a file that describes none");
		if (!voe_assets_gltf_uint_member(gltf, primitive, "material",
						 material_count - 1, 0,
						 &out->material))
			return false;
	}

	return true;
}

bool voe_assets_gltf_read_meshes(struct gltf *gltf, voe_assets_model *model)
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
		uint32_t mesh = voe_assets_gltf_object_at(gltf, gltf->meshes, i);
		uint32_t array;
		uint32_t primitive_count;

		if (mesh == VOE_ASSETS_JSON_NONE)
			return voe_assets_gltf_malformed(
				gltf, "a mesh that is not an object");
		if (!voe_assets_gltf_array_member(gltf, mesh, "primitives",
						  &array, &primitive_count))
			return false;

		meshes[i].first_primitive = written;
		meshes[i].primitive_count = primitive_count;

		for (uint32_t p = 0; p < primitive_count; p++) {
			uint32_t primitive =
				voe_assets_gltf_object_at(gltf, array, p);

			if (primitive == VOE_ASSETS_JSON_NONE)
				return voe_assets_gltf_malformed(
					gltf, "a primitive that is not an object");
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
