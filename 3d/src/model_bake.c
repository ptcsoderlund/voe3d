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
//
// A VERTEX COLOUR IS WRITTEN AS ITS COMPLEMENT, each channel clamped to 0–1 and
// stored as (1 − c) × 65535 rounded, so a primitive without colours leaves the
// field zeroed and draws white (ADR-0421 point 1).
//
// A NODE'S LEVEL RIDES THE WALK beside its matrix. A node of another level is
// walked, since its children may name their own, but its mesh is not placed; a
// node past HIGHEST_LEVEL is not walked further.
//
// A REFUSAL IS A LIMIT THE ENGINE KEEPS (0445 point 5): materials past
// VOE_3D_MODEL_PARTS, nodes nested past VOE_3D_IMPORT_MAX_DEPTH, vertices or
// triangles past a 32-bit count, each said with the file's own number, counted
// in full first, beside the most, and what to do in Blender.
#include "model_bake.h"

#include <3d/import.h>
#include <3d/normal_matrix.h>
#include <base/assert.h>
#include <base/report.h>
#include <math/float3.h>
#include <math/float4x4.h>

#include <ctype.h>
#include <inttypes.h>
#include <math.h>
#include <string.h>

// The last own level a file may name, the store's last (ADR-0425 point 1).
#define HIGHEST_LEVEL (VOE_3D_MODEL_LEVELS - 1)

// One drawn primitive where the walk put it, and the part it joins.
struct placement {
	uint32_t primitive;
	uint32_t part;
	voe_math_float4x4 world;
};

// One frame of the walk: which node, where its parent put it, how deep it is,
// and its level.
struct frame {
	uint32_t node;
	voe_math_float4x4 world;
	uint32_t depth;
	uint32_t level;
};

// What the walk carries: the level asked, the list it writes and each part's
// counts so far, wide, so a sum past a 32-bit count is seen rather than
// wrapped.
struct baking {
	const voe_assets_model *model;
	voe_base_arena *arena;
	uint32_t level;
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

// The level a node's name gives it: n for a name ending `_LOD<n>`, LOD in any
// case and n all digits, else `inherited`. The digits are read only while the
// level is within HIGHEST_LEVEL, so a long run of them cannot wrap and still
// reads as past it.
static uint32_t level_of(const voe_assets_node *node, uint32_t inherited)
{
	const char *suffix = strrchr(node->name.text, '_');
	uint32_t level = 0;
	size_t digits;

	if (suffix == NULL || tolower((unsigned char)suffix[1]) != 'l' ||
	    tolower((unsigned char)suffix[2]) != 'o' ||
	    tolower((unsigned char)suffix[3]) != 'd')
		return inherited;
	digits = strspn(suffix + 4, "0123456789");
	if (digits == 0 || suffix[4 + digits] != '\0')
		return inherited;
	for (size_t d = 0; d < digits && level <= HIGHEST_LEVEL; d++)
		level = level * 10 + (uint32_t)(suffix[4 + d] - '0');
	return level;
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

// How many materials the file's drawn primitives wear, glTF's default as one,
// for the refusal past VOE_3D_MODEL_PARTS: one flag a material, in `arena`.
static uint32_t materials_worn(const voe_assets_model *model,
			       voe_base_arena *arena)
{
	bool *worn = voe_base_arena_push(arena, (size_t)model->material_count + 1);
	uint32_t count = 0;

	memset(worn, 0, (size_t)model->material_count + 1);
	for (uint32_t p = 0; p < model->primitive_count; p++) {
		const voe_assets_primitive *primitive = &model->primitives[p];
		uint32_t material = primitive->material < model->material_count ?
					    primitive->material :
					    model->material_count;

		if (primitive->vertex_count == 0 || primitive->index_count == 0 ||
		    worn[material])
			continue;
		worn[material] = true;
		count++;
	}
	return count;
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
				       "the model uses %u materials, and the most the engine takes is %u; join materials in Blender, or split the model into files",
				       materials_worn(model, baking->arena),
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

// The walk goes on past a node too deep, placing nothing more, so the refusal
// names the file's deepest nesting and not the first past the limit.
static bool walk(struct baking *baking, voe_base_error *error)
{
	const voe_assets_model *model = baking->model;
	struct frame *stack;
	uint32_t deepest = 0;
	uint32_t open = 0;

	stack = voe_base_arena_push(baking->arena,
				    (size_t)model->node_count * sizeof(*stack));

	for (uint32_t i = 0; i < model->root_count; i++) {
		stack[open++] = (struct frame){
			.node = model->roots[i],
			.world = model->nodes[model->roots[i]].local,
			.depth = 1,
			.level = level_of(&model->nodes[model->roots[i]], 0),
		};
	}

	while (open > 0) {
		struct frame frame = stack[--open];
		const voe_assets_node *node = &model->nodes[frame.node];

		if (frame.depth > deepest)
			deepest = frame.depth;
		if (frame.level > HIGHEST_LEVEL)
			continue;
		if (deepest <= VOE_3D_IMPORT_MAX_DEPTH &&
		    frame.level == baking->level && !place(baking, &frame, error))
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
				.level = level_of(&model->nodes[child], frame.level),
			};
		}
	}
	if (deepest > VOE_3D_IMPORT_MAX_DEPTH) {
		VOE_BASE_ERROR("3d",
			       "the model's objects are parented %" PRIu32
			       " deep, and the most the engine takes is %u; in Blender, use Clear Parent and Keep Transform on the deepest",
			       deepest, VOE_3D_IMPORT_MAX_DEPTH);
		return refuse(error, VOE_BASE_ERROR_UNSUPPORTED);
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

	// At most VOE_3D_MODEL_PARTS sums of 32-bit counts, so 64 bits hold
	// the whole model's.
	for (uint32_t p = 0; p < out->part_count; p++) {
		vertices += baking->vertices[p];
		indices += baking->indices[p];
	}
	if (vertices > UINT32_MAX) {
		VOE_BASE_ERROR("3d",
			       "the model has %" PRIu64
			       " vertices, and the most the engine takes is %" PRIu32
			       "; reduce it with Decimate in Blender",
			       vertices, UINT32_MAX);
		return refuse(error, VOE_BASE_ERROR_REFUSED);
	}
	if (indices > UINT32_MAX) {
		VOE_BASE_ERROR("3d",
			       "the model has %" PRIu64
			       " triangles, and the most the engine takes is %" PRIu32
			       "; reduce it with Decimate in Blender",
			       indices / 3, UINT32_MAX / 3);
		return refuse(error, VOE_BASE_ERROR_REFUSED);
	}

	vertices = 0;
	indices = 0;
	for (uint32_t p = 0; p < out->part_count; p++) {
		out->parts[p].first_vertex = (uint32_t)vertices;
		out->parts[p].first_index = (uint32_t)indices;
		out->parts[p].vertex_count = (uint32_t)baking->vertices[p];
		out->parts[p].index_count = (uint32_t)baking->indices[p];
		vertices += baking->vertices[p];
		indices += baking->indices[p];
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

// One channel's complement; fmaxf first so a NaN reads as nought.
static uint16_t complement(float channel)
{
	float c = fminf(fmaxf(channel, 0.0f), 1.0f);

	return (uint16_t)lroundf((1.0f - c) * 65535.0f);
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
		if (primitive->colours != NULL) {
			const voe_math_float4 colour = primitive->colours[v];

			vertices[v].colour_complement[0] = complement(colour.x);
			vertices[v].colour_complement[1] = complement(colour.y);
			vertices[v].colour_complement[2] = complement(colour.z);
			vertices[v].colour_complement[3] = complement(colour.w);
		}
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
			      uint32_t level, voe_3d_model_bake *out,
			      voe_base_error *error)
{
	struct baking baking = {
		.model = model, .arena = arena, .level = level, .out = out
	};
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

	if (!walk(&baking, error) || !lay_out(&baking, arena, error))
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

// The bake's walk without an arena to push its stack into: one cursor per
// depth, each the next child to go down, so the room is the depth the bake
// allows and nothing below it is counted.
uint32_t voe_3d_model_bake_levels(const voe_assets_model *model)
{
	struct cursor {
		uint32_t node;
		uint32_t next_child;
		uint32_t level;
	} path[VOE_3D_IMPORT_MAX_DEPTH];
	uint32_t highest = 0;

	VOE_BASE_ASSERT(model != NULL, "counting the levels of nothing");

	for (uint32_t r = 0; r < model->root_count; r++) {
		uint32_t depth = 1;

		path[0] = (struct cursor){
			.node = model->roots[r],
			.level = level_of(&model->nodes[model->roots[r]], 0),
		};
		while (depth > 0) {
			struct cursor *at = &path[depth - 1];
			const voe_assets_node *node = &model->nodes[at->node];
			uint32_t child;

			if (at->level <= HIGHEST_LEVEL && at->level > highest)
				highest = at->level;
			if (at->level > HIGHEST_LEVEL ||
			    at->next_child == node->child_count ||
			    depth == VOE_3D_IMPORT_MAX_DEPTH) {
				depth--;
				continue;
			}
			child = node->children[at->next_child++];
			path[depth++] = (struct cursor){
				.node = child,
				.level = level_of(&model->nodes[child], at->level),
			};
		}
	}

	VOE_BASE_ASSERT(highest <= HIGHEST_LEVEL, "a level past the last");
	return highest + 1;
}
