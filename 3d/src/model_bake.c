// The bake: the node tree walked into a list of placed primitives, each part's
// room counted from that list, then every vertex carried into model space.
//
// TWO PASSES OVER A LIST AND NOT OVER THE TREE. The walk writes one placement per
// drawn primitive — which primitive, its node's world matrix, its part — so the
// counts that size the arrays and the fill that writes them read the same list,
// and the tree is walked once. The list is as long as the nodes' meshes have
// primitives, at most, since every node is placed at most once.
//
// A MIRRORED NODE HAS ITS TRIANGLES TURNED. A world matrix whose basis has a
// negative determinant turns counter-clockwise into clockwise, which is culled as
// a back face; glTF says to wind such a node's triangles the other way, so two
// indices of each are swapped as they are written.
//
// A NORMAL IS CARRIED BY THE NORMAL MATRIX AND MADE UNIT LENGTH AGAIN, since a
// vertex's normal is unit length in `render`; one of nothing stays nothing,
// which draws unlit (render/device.h).
#include "model_bake.h"

#include <3d/import.h>
#include <3d/normal_matrix.h>
#include <base/assert.h>
#include <base/report.h>
#include <math/float3.h>
#include <math/float4x4.h>

// One drawn primitive where the walk put it, and the part it joins.
struct placement {
	uint32_t primitive;
	uint32_t part;
	voe_math_float4x4 world;
};

// One frame of the walk: which node, where its parent put it, how deep it is.
struct frame {
	uint32_t node;
	voe_math_float4x4 world;
	uint32_t depth;
};

// What the walk carries: the list it writes and each part's counts so far,
// wide, so a sum past a 32-bit count is seen rather than wrapped.
struct baking {
	const voe_assets_model *model;
	voe_3d_model_bake *out;
	struct placement *placements;
	size_t placement_count;
	uint64_t vertices[VOE_3D_MODEL_PARTS];
	uint64_t indices[VOE_3D_MODEL_PARTS];
};

static bool refuse(voe_base_error *error, voe_base_error code)
{
	if (error != NULL)
		*error = code;
	return false;
}

// The part wearing `material`, added when it is new; VOE_3D_MODEL_PARTS when
// there is no room for another.
static uint32_t part_of(voe_3d_model_bake *out, uint32_t material)
{
	for (uint32_t p = 0; p < out->part_count; p++) {
		if (out->parts[p].material == material)
			return p;
	}
	if (out->part_count == VOE_3D_MODEL_PARTS)
		return VOE_3D_MODEL_PARTS;
	out->parts[out->part_count] = (voe_3d_model_bake_part){
		.material = material,
	};
	return out->part_count++;
}

// Every drawn primitive of the node's mesh, placed at the node's world matrix.
static bool place(struct baking *baking, const struct frame *frame,
		  voe_base_error *error)
{
	const voe_assets_model *model = baking->model;
	const voe_assets_node *node = &model->nodes[frame->node];
	const voe_assets_mesh *mesh;

	if (node->mesh == VOE_ASSETS_MODEL_NONE || node->mesh >= model->mesh_count)
		return true;
	mesh = &model->meshes[node->mesh];

	for (uint32_t p = 0; p < mesh->primitive_count; p++) {
		uint32_t index = mesh->first_primitive + p;
		const voe_assets_primitive *primitive;
		uint32_t material;
		uint32_t part;

		if (index >= model->primitive_count)
			return true;
		primitive = &model->primitives[index];
		if (primitive->vertex_count == 0 || primitive->index_count == 0)
			continue;

		material = primitive->material < model->material_count ?
				   primitive->material :
				   VOE_ASSETS_MODEL_NONE;
		part = part_of(baking->out, material);
		if (part == VOE_3D_MODEL_PARTS) {
			VOE_BASE_ERROR("3d",
				       "a model wearing more than %u materials",
				       VOE_3D_MODEL_PARTS);
			return refuse(error, VOE_BASE_ERROR_UNSUPPORTED);
		}

		baking->vertices[part] += primitive->vertex_count;
		baking->indices[part] += primitive->index_count;
		baking->placements[baking->placement_count++] =
			(struct placement){ index, part, frame->world };
	}
	return true;
}

static bool walk(struct baking *baking, voe_base_arena *arena,
		 voe_base_error *error)
{
	const voe_assets_model *model = baking->model;
	struct frame *stack;
	uint32_t open = 0;

	stack = voe_base_arena_push(arena,
				    (size_t)model->node_count * sizeof(*stack));

	for (uint32_t i = 0; i < model->root_count; i++) {
		stack[open++] = (struct frame){
			.node = model->roots[i],
			.world = model->nodes[model->roots[i]].local,
			.depth = 1,
		};
	}

	while (open > 0) {
		struct frame frame = stack[--open];
		const voe_assets_node *node = &model->nodes[frame.node];

		if (frame.depth > VOE_3D_IMPORT_MAX_DEPTH) {
			VOE_BASE_ERROR("3d",
				       "a model whose nodes nest more than %u deep",
				       VOE_3D_IMPORT_MAX_DEPTH);
			return refuse(error, VOE_BASE_ERROR_UNSUPPORTED);
		}
		if (!place(baking, &frame, error))
			return false;

		// The parent's matrix on the left: the child's own transform is
		// applied first and the parent's carries it into the model.
		for (uint32_t c = 0; c < node->child_count; c++) {
			uint32_t child = node->children[c];

			stack[open++] = (struct frame){
				.node = child,
				.world = voe_math_float4x4_mul(
					frame.world, model->nodes[child].local),
				.depth = frame.depth + 1,
			};
		}
	}
	return true;
}

// Each part's range in the arrays, parts one after another, and the arrays.
static bool lay_out(struct baking *baking, voe_base_arena *arena,
		    voe_base_error *error)
{
	voe_3d_model_bake *out = baking->out;
	uint64_t vertices = 0;
	uint64_t indices = 0;

	for (uint32_t p = 0; p < out->part_count; p++) {
		out->parts[p].first_vertex = (uint32_t)vertices;
		out->parts[p].first_index = (uint32_t)indices;
		out->parts[p].vertex_count = (uint32_t)baking->vertices[p];
		out->parts[p].index_count = (uint32_t)baking->indices[p];
		vertices += baking->vertices[p];
		indices += baking->indices[p];
		if (vertices > UINT32_MAX || indices > UINT32_MAX) {
			VOE_BASE_ERROR("3d",
				       "a model with more vertices or indices than a count holds");
			return refuse(error, VOE_BASE_ERROR_REFUSED);
		}
	}

	out->vertex_count = (uint32_t)vertices;
	out->index_count = (uint32_t)indices;
	if (out->part_count > 0) {
		out->vertices = voe_base_arena_push(
			arena, (size_t)vertices * sizeof(*out->vertices));
		out->indices = voe_base_arena_push(
			arena, (size_t)indices * sizeof(*out->indices));
	}
	return true;
}

static bool mirrored(const voe_math_float4x4 *m)
{
	float determinant =
		m->m[0][0] * (m->m[1][1] * m->m[2][2] - m->m[1][2] * m->m[2][1]) -
		m->m[0][1] * (m->m[1][0] * m->m[2][2] - m->m[1][2] * m->m[2][0]) +
		m->m[0][2] * (m->m[1][0] * m->m[2][1] - m->m[1][1] * m->m[2][0]);

	return determinant < 0.0f;
}

// One placement's vertices into model space at its part's next vertex, and its
// indices after them, counted from the part's first vertex.
static void fill(voe_3d_model_bake *out, const voe_assets_model *model,
		 const struct placement *placement, uint32_t *next_vertex,
		 uint32_t *next_index)
{
	const voe_assets_primitive *primitive =
		&model->primitives[placement->primitive];
	const voe_3d_model_bake_part *part = &out->parts[placement->part];
	voe_math_float4x4 normals = voe_3d_normal_matrix(placement->world);
	voe_render_vertex *vertices = &out->vertices[*next_vertex];
	uint32_t *indices = &out->indices[*next_index];
	uint32_t base = *next_vertex - part->first_vertex;
	bool turn = mirrored(&placement->world);

	for (uint32_t v = 0; v < primitive->vertex_count; v++) {
		voe_math_float3 normal = { 0.0f, 0.0f, 0.0f };

		if (primitive->normals != NULL)
			normal = voe_math_float4x4_transform_dir(
				normals, primitive->normals[v]);
		if (voe_math_float3_length(normal) > 0.0f)
			normal = voe_math_float3_normalize(normal);
		vertices[v] = (voe_render_vertex){
			.position = voe_math_float4x4_transform_point(
				placement->world, primitive->positions[v]),
			.normal = normal,
			.uv = primitive->uvs != NULL ?
				      primitive->uvs[v] :
				      (voe_math_float2){ 0.0f, 0.0f },
		};
	}

	for (uint32_t i = 0; i < primitive->index_count; i++) {
		uint32_t source = i;

		// The second and third corner of a triangle swapped.
		if (turn && i % 3 != 0)
			source = i % 3 == 1 ? i + 1 : i - 1;
		indices[i] = primitive->indices[source] + base;
	}

	*next_vertex += primitive->vertex_count;
	*next_index += primitive->index_count;
}

bool voe_3d_model_bake_create(voe_base_arena *arena, const voe_assets_model *model,
		       voe_3d_model_bake *out, voe_base_error *error)
{
	struct baking baking = { .model = model, .out = out };
	uint32_t next_vertex[VOE_3D_MODEL_PARTS];
	uint32_t next_index[VOE_3D_MODEL_PARTS];
	size_t most = 0;

	VOE_BASE_ASSERT(arena != NULL, "baking a model without an arena");
	VOE_BASE_ASSERT(model != NULL && out != NULL, "baking nothing");

	*out = (voe_3d_model_bake){ 0 };
	if (model->node_count == 0)
		return true;

	for (uint32_t n = 0; n < model->node_count; n++) {
		uint32_t mesh = model->nodes[n].mesh;

		if (mesh != VOE_ASSETS_MODEL_NONE && mesh < model->mesh_count)
			most += model->meshes[mesh].primitive_count;
	}
	baking.placements = voe_base_arena_push(
		arena, (most == 0 ? 1 : most) * sizeof(*baking.placements));

	if (!walk(&baking, arena, error) || !lay_out(&baking, arena, error))
		return false;

	for (uint32_t p = 0; p < out->part_count; p++) {
		next_vertex[p] = out->parts[p].first_vertex;
		next_index[p] = out->parts[p].first_index;
	}
	for (size_t i = 0; i < baking.placement_count; i++) {
		uint32_t part = baking.placements[i].part;

		fill(out, model, &baking.placements[i], &next_vertex[part],
		     &next_index[part]);
	}
	return true;
}
