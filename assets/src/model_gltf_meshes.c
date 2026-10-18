// glTF's meshes, read into the arrays in assets/model.h: attributes, primitives
// and the meshes holding them, all primitives in one array. Called by
// model_gltf.c through model_gltf_read.h after the materials, whose count a
// primitive's material index is checked against; that header's rules hold here.
//
// EACH MESH IS READ UNDER ITS PART, EACH PRIMITIVE UNDER THE MATERIAL IT WEARS
// (ADR-0445): "the part `twig`: the material `bark`: …", or "part 2" and "no
// material" without a name, so a refusal below says neither. Damage is the
// damaged-file shape; a form the engine does not read says what to change in
// Blender's export.
//
// TRIANGLE LISTS ONLY, AND AN UNINDEXED PRIMITIVE IS GIVEN INDICES ON THE WAY
// IN. Strips, fans, lines and points are refused, their mode number in the
// format's words. Generating the indices for a primitive that has none costs one
// array and means everything downstream — the pool, the draw, the importer — has
// exactly one shape to handle. Indices are all an expansion adds, so every
// attribute, COLOR_0 included, comes through it as the file had it.
//
// POSITION, NORMAL AND TEXCOORD_0 ARE READ AS FLOATS ONLY, AND COLOR_0 AS glTF
// ALLOWS IT AND NO WIDER: VEC3 or VEC4 of floats, or of unsigned bytes or shorts
// marked normalized, all turned into float 0 to 1. Any other stored form is
// unsupported, saying which attribute and how it is stored.
#include "model_gltf_read.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

// The only primitive mode this engine draws.
#define MODE_TRIANGLES 4

// A component type in the artist's words. resolve_accessor has refused every
// type but these six.
static const char *stored_as(uint32_t component)
{
	switch (component) {
	case COMPONENT_BYTE:
		return "signed bytes";
	case COMPONENT_UNSIGNED_BYTE:
		return "unsigned bytes";
	case COMPONENT_SHORT:
		return "signed shorts";
	case COMPONENT_UNSIGNED_SHORT:
		return "unsigned shorts";
	case COMPONENT_UNSIGNED_INT:
		return "unsigned ints";
	case COMPONENT_FLOAT:
		return "floats";
	default:
		return "numbers";
	}
}

// Whether accessor `index`, already resolved, says `"normalized": true`.
static bool is_normalized(struct gltf *gltf, uint32_t index)
{
	uint32_t flag = voe_assets_json_member(
		gltf->json, voe_assets_gltf_object_at(gltf, gltf->accessors, index),
		"normalized");

	return flag != VOE_ASSETS_JSON_NONE &&
	       voe_assets_json_kind_of(gltf->json, flag) == VOE_ASSETS_JSON_TRUE;
}

// An attribute stored in a form the engine does not read: `what` it is to the
// artist and how it is stored, what is read, and the export options that change
// it. Always false.
static bool refuse_stored(struct gltf *gltf, const char *what,
			  const char *attribute, const struct view *view,
			  bool normalized, const char *read)
{
	char words[128];

	(void)snprintf(words, sizeof(words),
		       "%s of %" PRIu32 " components of componentType %" PRIu32
		       "%s",
		       attribute, view->components, view->component,
		       normalized ? ", normalized" : "");
	return voe_assets_gltf_unsupported(
		gltf, words,
		"its %s are stored as %" PRIu32
		" %s%s each, and the engine reads %s; untick Compression or Quantize in Blender's glTF export",
		what, view->components, normalized ? "normalized " : "",
		stored_as(view->component), read);
}

// A mesh's primitives array, refused when the mesh is not an object or has none.
static bool mesh_primitives(struct gltf *gltf, uint32_t mesh, uint32_t *array,
			    uint32_t *count)
{
	if (mesh == VOE_ASSETS_JSON_NONE)
		return voe_assets_gltf_malformed(gltf,
						 "a mesh that is not an object");
	if (!voe_assets_gltf_array_member(gltf, mesh, "primitives", array,
					  count))
		return false;
	if (*count == 0)
		return voe_assets_gltf_malformed(
			gltf, "a mesh with no primitives in it");
	return true;
}

// Mesh `index`'s part as the subject, its name read first; false, with nothing
// pushed, when that name is not a string.
static bool push_part(struct gltf *gltf, uint32_t mesh, uint32_t index)
{
	voe_assets_model_name name = { .text = "" };

	if (mesh != VOE_ASSETS_JSON_NONE &&
	    !voe_assets_gltf_read_name(gltf, mesh, &name))
		return false;
	voe_assets_gltf_subject_push(gltf, "part", &name, index);
	return true;
}

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
		bool found;

		if (!push_part(gltf, mesh, i))
			return false;
		found = mesh_primitives(gltf, mesh, &primitives, &count);
		voe_assets_gltf_subject_pop(gltf);
		if (!found)
			return false;
		*total += count;
	}
	return true;
}

// One attribute into an array of float2 or float3, checking that it has as many
// elements as the primitive's positions did. `what` is its name to the artist.
static bool read_attribute(struct gltf *gltf, uint32_t attributes,
			   const char *name, const char *what,
			   uint32_t components, uint32_t expected, void *out,
			   bool *present)
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
			gltf, "%s names an accessor in a file that describes none",
			name);
	if (!voe_assets_json_uint(gltf->json, token, accessor_count - 1, &index))
		return voe_assets_gltf_malformed(
			gltf, "%s's accessor index is out of range", name);
	if (!voe_assets_gltf_resolve_accessor(gltf, index, &view))
		return false;
	if (view.components != components)
		return voe_assets_gltf_malformed(
			gltf,
			"%s's accessor has %" PRIu32 " components, not %" PRIu32,
			name, view.components, components);
	if (view.component != COMPONENT_FLOAT)
		return refuse_stored(gltf, what, name, &view,
				     is_normalized(gltf, index), "them as floats");
	if (expected != 0 && view.count != expected)
		return voe_assets_gltf_malformed(
			gltf, "%s has %" PRIu32 " elements where POSITION has %" PRIu32,
			name, view.count, expected);

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

	normalized = is_normalized(gltf, index);
	readable = view.component == COMPONENT_FLOAT ||
		   (normalized && (view.component == COMPONENT_UNSIGNED_BYTE ||
				   view.component == COMPONENT_UNSIGNED_SHORT));
	if (!readable || (view.components != 3 && view.components != 4))
		return refuse_stored(
			gltf, "vertex colours", "COLOR_0", &view, normalized,
			"3 or 4 floats, or 3 or 4 normalized unsigned bytes or shorts");
	if (view.count != expected)
		return voe_assets_gltf_malformed(
			gltf,
			"COLOR_0 has %" PRIu32 " elements where POSITION has %" PRIu32,
			view.count, expected);

	colours = voe_base_arena_push(gltf->arena,
				      (size_t)view.count * sizeof(*colours));
	for (uint32_t i = 0; i < view.count; i++)
		colours[i] = read_colour(&view, i);
	*out = colours;
	return true;
}

// The primitive's indices into `out`, or the ones an unindexed primitive
// implies: one shape downstream instead of two — see this file's header.
static bool read_indices(struct gltf *gltf, uint32_t primitive,
			 uint32_t accessor_count, voe_assets_primitive *out)
{
	uint32_t token = voe_assets_json_member(gltf->json, primitive, "indices");
	uint32_t *indices;
	struct view view;
	uint32_t index;

	if (token == VOE_ASSETS_JSON_NONE) {
		if (out->vertex_count % 3 != 0)
			return voe_assets_gltf_malformed(
				gltf,
				"no indices and %" PRIu32
				" vertices, not a multiple of three",
				out->vertex_count);
		indices = voe_base_arena_push(gltf->arena,
					      (size_t)out->vertex_count *
						      sizeof(*indices));
		for (uint32_t i = 0; i < out->vertex_count; i++)
			indices[i] = i;
		out->indices = indices;
		out->index_count = out->vertex_count;
		return true;
	}

	if (!voe_assets_json_uint(gltf->json, token, accessor_count - 1, &index))
		return voe_assets_gltf_malformed(
			gltf, "an indices accessor index that is out of range");
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
			gltf, "%" PRIu32 " indices, not a multiple of three",
			view.count);

	indices = voe_base_arena_push(gltf->arena,
				      (size_t)view.count * sizeof(*indices));
	for (uint32_t i = 0; i < view.count; i++) {
		indices[i] = voe_assets_gltf_read_index(&view, i);

		// EVERY INDEX IS CHECKED AGAINST THE VERTEX COUNT, AND THIS IS
		// THE CHECK THAT MATTERS MOST IN THE FILE. An index past the end
		// of the vertices is a read past the end of a GPU buffer, which
		// is a hostile file's shortest path to something interesting.
		if (indices[i] >= out->vertex_count)
			return voe_assets_gltf_malformed(
				gltf,
				"index %" PRIu32 " is %" PRIu32
				", past the part's %" PRIu32 " vertices",
				i, indices[i], out->vertex_count);
	}
	out->indices = indices;
	out->index_count = view.count;
	return true;
}

// One primitive wearing `material`, under that material's subject.
static bool read_primitive(struct gltf *gltf, uint32_t primitive,
			   uint32_t material, voe_assets_primitive *out)
{
	uint32_t accessor_count = gltf->accessors == VOE_ASSETS_JSON_NONE ?
					  0 :
					  voe_assets_json_count(gltf->json,
								gltf->accessors);
	uint32_t attributes;
	uint32_t mode;
	uint32_t position;
	uint32_t index;
	struct view positions;
	voe_math_float3 *points;
	voe_math_float3 *normals;
	voe_math_float2 *uvs;
	bool present;

	*out = (voe_assets_primitive){ .material = material };

	if (!voe_assets_gltf_uint_member(gltf, primitive, "mode", UINT32_MAX,
					 MODE_TRIANGLES, &mode))
		return false;
	if (mode != MODE_TRIANGLES) {
		char words[64];

		(void)snprintf(words, sizeof(words),
			       "primitive mode %" PRIu32
			       ", where only 4 (TRIANGLES) is read",
			       mode);
		return voe_assets_gltf_unsupported(
			gltf, words,
			"it has %s, and the engine draws triangles only; %s",
			mode < MODE_TRIANGLES ? "points or lines" :
						"triangle strips or fans",
			mode < MODE_TRIANGLES ?
				"in Blender delete its loose edges and points, or Triangulate it" :
				"Triangulate it in Blender");
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
	if (!voe_assets_json_uint(gltf->json, position, accessor_count - 1,
				  &index))
		return voe_assets_gltf_malformed(
			gltf, "a POSITION accessor index that is out of range");
	if (!voe_assets_gltf_resolve_accessor(gltf, index, &positions))
		return false;
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

	if (!read_attribute(gltf, attributes, "POSITION", "vertex positions",
			    3, 0, points, &present))
		return false;
	out->positions = points;

	if (!read_attribute(gltf, attributes, "NORMAL", "normals", 3,
			    positions.count, normals, &present))
		return false;
	out->normals = present ? normals : NULL;

	if (!read_attribute(gltf, attributes, "TEXCOORD_0", "UV coordinates",
			    2, positions.count, uvs, &present))
		return false;
	out->uvs = present ? uvs : NULL;

	if (!read_colours(gltf, attributes, accessor_count, positions.count,
			  &out->colours))
		return false;
	return read_indices(gltf, primitive, accessor_count, out);
}

// A primitive's material index, VOE_ASSETS_MODEL_NONE without one, checked
// against the materials before its subject can name it.
static bool primitive_material(struct gltf *gltf, uint32_t primitive,
			       uint32_t material_count, uint32_t *material)
{
	*material = VOE_ASSETS_MODEL_NONE;
	if (voe_assets_json_member(gltf->json, primitive, "material") ==
	    VOE_ASSETS_JSON_NONE)
		return true;
	if (material_count == 0)
		return voe_assets_gltf_malformed(
			gltf,
			"a primitive naming a material in a file that describes none");
	return voe_assets_gltf_uint_member(gltf, primitive, "material",
					   material_count - 1, 0, material);
}

// One mesh's primitives into `primitives`, under the part's subject the caller
// pushed, each under the material it wears.
static bool read_primitives(struct gltf *gltf, uint32_t mesh,
			    const voe_assets_model *model,
			    voe_assets_primitive *primitives,
			    voe_assets_mesh *out)
{
	uint32_t array;

	if (!mesh_primitives(gltf, mesh, &array, &out->primitive_count))
		return false;

	for (uint32_t p = 0; p < out->primitive_count; p++) {
		uint32_t primitive = voe_assets_gltf_object_at(gltf, array, p);
		uint32_t material;
		bool read;

		if (primitive == VOE_ASSETS_JSON_NONE)
			return voe_assets_gltf_malformed(
				gltf, "a primitive that is not an object");
		if (!primitive_material(gltf, primitive, model->material_count,
					&material))
			return false;
		if (material == VOE_ASSETS_MODEL_NONE)
			voe_assets_gltf_subject_push(gltf, "no material", NULL,
						     VOE_ASSETS_MODEL_NONE);
		else
			voe_assets_gltf_subject_push(
				gltf, "material",
				&model->materials[material].name, material);
		read = read_primitive(gltf, primitive, material, &primitives[p]);
		voe_assets_gltf_subject_pop(gltf);
		if (!read)
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
		bool read;

		if (!push_part(gltf, mesh, i))
			return false;
		meshes[i].first_primitive = written;
		read = read_primitives(gltf, mesh, model, primitives + written,
				       &meshes[i]);
		voe_assets_gltf_subject_pop(gltf);
		if (!read)
			return false;
		written += meshes[i].primitive_count;
	}

	model->meshes = meshes;
	model->mesh_count = count;
	model->primitives = primitives;
	model->primitive_count = total;
	return true;
}
