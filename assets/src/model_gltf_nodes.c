// glTF's node tree, read into the arrays in assets/model.h: each node's name,
// mesh and local matrix, its children, and the roots — the default scene's nodes, or
// every node nothing claims. Called by model_gltf.c through model_gltf_read.h
// after the meshes, whose count a node's mesh index is checked against.
//
// THE TREE IS PROVEN A TREE HERE, WITHOUT RECURSION (rule 14). The nodes are
// read as a flat array with a claimed flag beside each: a node claimed as a
// child twice is refused, so the walk the importer does cannot meet a cycle.
#include "model_gltf_read.h"

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

bool voe_assets_gltf_read_nodes(struct gltf *gltf, voe_assets_model *model)
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
		uint32_t node = voe_assets_gltf_object_at(gltf, gltf->nodes, i);
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
			return voe_assets_gltf_malformed(
				gltf, "a node that is not an object");
		if (!voe_assets_gltf_read_name(gltf, node, &nodes[i].name))
			return false;

		nodes[i].mesh = VOE_ASSETS_MODEL_NONE;
		if (voe_assets_json_member(gltf->json, node, "mesh") !=
		    VOE_ASSETS_JSON_NONE) {
			if (model->mesh_count == 0)
				return voe_assets_gltf_malformed(
					gltf,
					"a node naming a mesh in a file that describes none");
			if (!voe_assets_gltf_uint_member(gltf, node, "mesh",
							 model->mesh_count - 1,
							 0, &nodes[i].mesh))
				return false;
		}

		// A node gives either a matrix or the three parts, and glTF says
		// a matrix wins. Composing the parts here means the importer
		// sees one shape.
		if (!voe_assets_gltf_floats_member(gltf, node, "matrix", 16,
						   matrix, &has_matrix))
			return false;
		if (has_matrix) {
			nodes[i].local = transposed(matrix);
		} else {
			if (!voe_assets_gltf_floats_member(gltf, node,
							   "translation", 3,
							   translation,
							   &found) ||
			    !voe_assets_gltf_floats_member(gltf, node,
							   "rotation", 4,
							   rotation, &found) ||
			    !voe_assets_gltf_floats_member(gltf, node, "scale",
							   3, scale, &found))
				return false;
			nodes[i].local = composed(translation, rotation, scale);
		}

		if (!voe_assets_gltf_array_member(gltf, node, "children",
						  &children, &child_count))
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
				return voe_assets_gltf_malformed(
					gltf,
					"a child index that is not a node this file describes");

			// A NODE CLAIMED TWICE IS NOT A TREE, AND REFUSING IT
			// HERE IS WHAT MAKES A WALK OF IT SAFE. Two parents is
			// how a file describes a cycle — a is b's child and b
			// is a's — and a walk of that never ends. glTF forbids
			// it; a hostile or broken file does not care, and the
			// importer's depth limit should not be the only thing
			// standing in the way.
			if (child == i)
				return voe_assets_gltf_malformed(
					gltf, "a node that is its own child");
			if (claimed[child])
				return voe_assets_gltf_malformed(
					gltf,
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

		if (!voe_assets_gltf_uint_member(gltf, 0, "scene",
						 scene_count - 1, 0, &which))
			return false;
		scene = voe_assets_gltf_object_at(gltf, gltf->scenes, which);
		if (scene == VOE_ASSETS_JSON_NONE)
			return voe_assets_gltf_malformed(
				gltf, "a scene that is not an object");
		if (!voe_assets_gltf_array_member(gltf, scene, "nodes", &roots,
						  &root_count))
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
				return voe_assets_gltf_malformed(
					gltf,
					"a scene naming a node this file does not describe");
			if (claimed[root_list[i]])
				return voe_assets_gltf_malformed(
					gltf,
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
