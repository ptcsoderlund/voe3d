// A landscape row's nodes (draw_terrain.h; 0396 points 3 and 4): the eye taken
// into the grid's space, the nodes selected on scratch, and each node's record
// over the heights texture, drawn solid or held blended.
//
// Used by draw_system.c for each landscape row of a pass, and by the shadow,
// capture and sun-map passes through the cast, which takes no scratch.
//
// Constraints: one selection per row per pass, from the root of the pyramid,
// and every chosen node drawn: no frustum test, which is 093's (0395). A
// box under a millimetre tall is drawn a millimetre tall, so the shader's
// division by its height stays finite. The cast chooses on the stack, 32 KB,
// because the shadows call takes no arena; an arena on the frame would lift it.
#include "draw_terrain.h"

#include "models_store.h"

#include <3d/landscape.h>
#include <base/assert.h>

// The least box height, metres (0396 point 3).
#define LEAST_BOX 0.001f

// One node's record: `world` × the node's box, the row's `normal`, and the
// heights, `terrain` and `morph` render's header asks for.
static voe_render_object node_object(voe_math_float4x4 world,
				     voe_math_float4x4 normal,
				     const voe_3d_landscape_node *node,
				     voe_render_texture heights, float side,
				     voe_render_shading shading)
{
	const float tall = node->high - node->low > LEAST_BOX ?
				   node->high - node->low :
				   LEAST_BOX;
	const voe_math_float4x4 box = { .m = { { node->side, 0, 0, node->x },
					       { 0, tall, 0, node->low },
					       { 0, 0, node->side, node->z },
					       { 0, 0, 0, 1 } } };
	voe_render_object object = {
		.world = voe_math_float4x4_mul(world, box),
		.normal = normal,
		.shading = shading.index,
		.heights = heights.index + 1,
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
		.terrain = { node->x, node->z, node->side, side },
		.morph = { node->morph_start, node->morph_end, node->low, tall },
	};

	VOE_BASE_ASSERT(node->side > 0.0f, "a node with no side");
	VOE_BASE_ASSERT(object.morph.w > 0.0f, "a node box with no height");
	return object;
}

// `entry`'s terrain into `terrain` and the nodes the eye at `world`'s origin
// chooses into `chosen`, room VOE_3D_LANDSCAPE_NODES; how many. Nought for an
// entry that is no loaded landscape of `models` or a `world` scaled to nothing.
static uint32_t choose(const voe_3d_models *models,
		       const voe_3d_model_entry *entry, voe_math_float4x4 world,
		       voe_3d_models_terrain *terrain,
		       voe_3d_landscape_node *chosen)
{
	voe_math_float3 eye;
	uint32_t count;

	VOE_BASE_ASSERT(models != NULL && entry != NULL,
			"terrain nodes need a store and an entry");
	VOE_BASE_ASSERT(terrain != NULL && chosen != NULL,
			"terrain nodes need somewhere to go");
	if (!voe_3d_models_terrain_of(models, entry, terrain) ||
	    voe_math_float4x4_determinant(world) == 0.0f)
		return 0;
	// The eye is the origin of the space `world` maps into.
	eye = voe_math_float4x4_transform_point(voe_math_float4x4_inverse(world),
						(voe_math_float3){ 0.0f, 0.0f, 0.0f });
	count = voe_3d_landscape_select(terrain->lod, entry->landscape, eye,
					chosen, VOE_3D_LANDSCAPE_NODES);
	VOE_BASE_ASSERT(count <= VOE_3D_LANDSCAPE_NODES,
			"more nodes than were room for");
	return count;
}

struct voe_3d_terrain_nodes
voe_3d_draw_terrain_nodes(const voe_3d_models *models,
			  const voe_3d_model_entry *entry,
			  voe_math_float4x4 world, voe_math_float4x4 normal,
			  voe_base_arena *scratch)
{
	struct voe_3d_terrain_nodes out = { 0 };
	voe_3d_models_terrain terrain;
	voe_3d_landscape_node *chosen;

	VOE_BASE_ASSERT(scratch != NULL, "terrain nodes need scratch");
	chosen = voe_base_arena_push(scratch,
				     sizeof(*chosen) * VOE_3D_LANDSCAPE_NODES);
	out.count = choose(models, entry, world, &terrain, chosen);
	if (out.count == 0)
		return out;
	out.grid = terrain.grid;
	out.part = &entry->parts[0];
	out.objects = voe_base_arena_push(scratch,
					  sizeof(*out.objects) * out.count);
	for (uint32_t i = 0; i < out.count; i++)
		out.objects[i] = node_object(world, normal, &chosen[i],
					     terrain.heights, entry->landscape->size,
					     out.part->material.shading);
	return out;
}

bool voe_3d_draw_terrain_solid(voe_render_device *device,
			       const struct voe_3d_terrain_nodes *nodes)
{
	VOE_BASE_ASSERT(device != NULL && nodes != NULL,
			"drawing terrain needs a device and nodes");
	VOE_BASE_ASSERT(nodes->count == 0 || nodes->objects != NULL,
			"terrain nodes counted with no records");
	for (uint32_t i = 0; i < nodes->count; i++)
		if (!voe_render_frame_draw(device, nodes->grid, nodes->objects[i]))
			return false;
	return true;
}

bool voe_3d_draw_terrain_cast(voe_render_device *device,
			      const voe_3d_models *models,
			      const voe_3d_model_entry *entry,
			      voe_math_float4x4 world, voe_math_float4x4 normal)
{
	voe_3d_landscape_node chosen[VOE_3D_LANDSCAPE_NODES];
	voe_3d_models_terrain terrain;
	uint32_t count;

	VOE_BASE_ASSERT(device != NULL, "casting terrain needs a device");
	count = choose(models, entry, world, &terrain, chosen);
	VOE_BASE_ASSERT(count == 0 || entry->part_count >= 1,
			"a landscape with no part");
	for (uint32_t i = 0; i < count; i++)
		if (!voe_render_frame_draw(
			    device, terrain.grid,
			    node_object(world, normal, &chosen[i], terrain.heights,
					entry->landscape->size,
					entry->parts[0].material.shading)))
			return false;
	return true;
}

void voe_3d_draw_terrain_hold(struct voe_3d_draw_group *group,
			      const struct voe_3d_terrain_nodes *nodes,
			      float fade, voe_math_float4x4 view)
{
	VOE_BASE_ASSERT(group != NULL && nodes != NULL,
			"holding terrain needs a group and nodes");
	VOE_BASE_ASSERT(fade > 0.0f && fade < 1.0f,
			"a fading terrain outside 0 and 1");
	for (uint32_t i = 0; i < nodes->count; i++) {
		struct voe_3d_deferred entry = { .panel = false };

		entry.mesh.geometry = nodes->grid;
		entry.mesh.object = nodes->objects[i];
		entry.mesh.object.shading = nodes->part->faded.index;
		entry.mesh.object.colour.w = 1.0f - fade;
		voe_3d_draw_group_hold(group, entry, entry.mesh.object.world,
				       view);
	}
}
