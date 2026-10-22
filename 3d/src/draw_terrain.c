// A landscape row's nodes (draw_terrain.h; 0396 points 3 and 4): the eye taken
// into the grid's space, the nodes selected on scratch, and each node's record
// over the heights texture, drawn solid or held blended.
//
// Used by draw_system.c for each landscape row of a pass, and by the shadow,
// capture and sun-map passes through the cast, which takes no scratch and a
// capacity: the bounce's passes choose at most VOE_3D_TERRAIN_BOUNCE_NODES.
// A painted landscape's layers are looked up once per row per call (0402).
//
// Constraints: one selection per row per pass, from the root of the pyramid,
// then each chosen node's box sphere tested against the pass's cull (0464). A
// box under a millimetre tall is drawn a millimetre tall, so the shader's
// division by its height stays finite. The cast chooses on the stack, 32 KB,
// because the shadows call takes no arena; an arena on the frame would lift it.
#include "draw_terrain.h"

#include "draw_material.h"
#include "models_store.h"

#include <3d/landscape.h>
#include <base/assert.h>

#include <math.h>

// The least box height, metres (0396 point 3).
#define LEAST_BOX 0.001f

// The record every node of `entry` shares (0402 point 4): the part's shading
// and, with layers, `paint` and each layer's record, asked of the store now.
// A layer whose material is not loaded takes the part's own record; layers
// past the count repeat layer 0's. No layers: `paint` and `layers` nought.
static voe_render_object look_of(const voe_3d_models *models,
				 const voe_3d_model_entry *entry,
				 const voe_3d_models_terrain *terrain)
{
	const voe_assets_landscape *land = entry->landscape;
	const uint32_t own = entry->parts[0].material.shading.index;
	voe_render_object look = { .shading = own };

	VOE_BASE_ASSERT(land->layer_count <= VOE_ASSETS_LANDSCAPE_LAYERS,
			"more layers than a landscape holds");
	if (land->layer_count == 0)
		return look;
	look.paint[0] = terrain->paint[0].index + 1;
	look.paint[1] = terrain->paint[1].index + 1;
	for (uint32_t i = 0; i < land->layer_count; i++) {
		const voe_3d_model_part *worn =
			voe_3d_draw_material_named(models, land->layers[i]);

		look.layers[i] = worn != NULL ? worn->material.shading.index :
						own;
	}
	for (uint32_t i = land->layer_count; i < VOE_ASSETS_LANDSCAPE_LAYERS;
	     i++)
		look.layers[i] = look.layers[0];
	VOE_BASE_ASSERT(look.paint[0] != 0, "a painted landscape with no paint");
	return look;
}

// One node's record: `look` with `world` × the node's box, the row's
// `normal`, and the heights, `terrain` and `morph` render's header asks for.
static voe_render_object node_object(voe_math_float4x4 world,
				     voe_math_float4x4 normal,
				     const voe_3d_landscape_node *node,
				     voe_render_texture heights, float side,
				     const voe_render_object *look)
{
	const float tall = node->high - node->low > LEAST_BOX ?
				   node->high - node->low :
				   LEAST_BOX;
	const voe_math_float4x4 box = { .m = { { node->side, 0, 0, node->x },
					       { 0, tall, 0, node->low },
					       { 0, 0, node->side, node->z },
					       { 0, 0, 0, 1 } } };
	voe_render_object object = *look;

	object.world = voe_math_float4x4_mul(world, box);
	object.normal = normal;
	object.heights = heights.index + 1;
	object.colour = (voe_math_float4){ 1.0f, 1.0f, 1.0f, 1.0f };
	object.terrain = (voe_math_float4){ node->x, node->z, node->side, side };
	object.morph = (voe_math_float4){ node->morph_start, node->morph_end,
					  node->low, tall };

	VOE_BASE_ASSERT(node->side > 0.0f, "a node with no side");
	VOE_BASE_ASSERT(object.morph.w > 0.0f, "a node box with no height");
	return object;
}

// Whether `cull` sees the box `object` is drawn over: the grid's unit cube
// under its world, the row's world × the node's box (node_object), so about
// (0.5, 0.5, 0.5). The row's rotation and scale times the box's diagonal keep
// the columns square to each other, so every corner is half the root of their
// summed squares from the centre. NULL sees everything.
static bool node_seen(const struct voe_3d_cull *cull,
		      const voe_render_object *object)
{
	float squares = 0.0f;

	VOE_BASE_ASSERT(object != NULL, "culling no node");
	if (cull == NULL)
		return true;
	for (uint32_t r = 0; r < 3; r++)
		for (uint32_t c = 0; c < 3; c++)
			squares += object->world.m[r][c] * object->world.m[r][c];
	VOE_BASE_ASSERT(squares > 0.0f, "a node box of no size");
	return voe_3d_cull_sees(
		cull,
		voe_math_float4x4_transform_point(
			object->world, (voe_math_float3){ 0.5f, 0.5f, 0.5f }),
		0.5f * sqrtf(squares));
}

// Whether `object` is drawn, one thing counted into `counts` unless NULL: a
// cascade's `cast` or `cast_skipped` when `cast`, else `drawn` or `skipped`.
static bool node_counted(const struct voe_3d_cull *cull,
			 const voe_render_object *object,
			 voe_3d_view_counts *counts, bool cast)
{
	const bool drawn = node_seen(cull, object);
	uint32_t *count;

	if (counts == NULL)
		return drawn;
	if (cast)
		count = drawn ? &counts->cast : &counts->cast_skipped;
	else
		count = drawn ? &counts->drawn : &counts->skipped;
	(*count)++;
	VOE_BASE_ASSERT(*count > 0, "a node count wrapped");
	return drawn;
}

// `entry`'s terrain into `terrain` and the nodes the eye at `world`'s origin
// chooses into `chosen`, room `capacity`; how many. Nought for an entry that
// is no loaded landscape of `models` or a `world` scaled to nothing.
static uint32_t choose(const voe_3d_models *models,
		       const voe_3d_model_entry *entry, voe_math_float4x4 world,
		       voe_3d_models_terrain *terrain,
		       voe_3d_landscape_node *chosen, uint32_t capacity)
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
					chosen, capacity);
	VOE_BASE_ASSERT(count <= capacity, "more nodes than were room for");
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
	voe_render_object look;

	VOE_BASE_ASSERT(scratch != NULL, "terrain nodes need scratch");
	chosen = voe_base_arena_push(scratch,
				     sizeof(*chosen) * VOE_3D_LANDSCAPE_NODES);
	out.count = choose(models, entry, world, &terrain, chosen,
			   VOE_3D_LANDSCAPE_NODES);
	if (out.count == 0)
		return out;
	out.grid = terrain.grid;
	out.part = &entry->parts[0];
	look = look_of(models, entry, &terrain);
	out.objects = voe_base_arena_push(scratch,
					  sizeof(*out.objects) * out.count);
	for (uint32_t i = 0; i < out.count; i++)
		out.objects[i] = node_object(world, normal, &chosen[i],
					     terrain.heights, entry->landscape->size,
					     &look);
	return out;
}

bool voe_3d_draw_terrain_solid(voe_render_device *device,
			       const struct voe_3d_terrain_nodes *nodes,
			       const struct voe_3d_cull *cull,
			       voe_3d_view_counts *counts)
{
	VOE_BASE_ASSERT(device != NULL && nodes != NULL,
			"drawing terrain needs a device and nodes");
	VOE_BASE_ASSERT(nodes->count == 0 || nodes->objects != NULL,
			"terrain nodes counted with no records");
	for (uint32_t i = 0; i < nodes->count; i++) {
		if (!node_counted(cull, &nodes->objects[i], counts, false))
			continue;
		if (!voe_render_frame_draw(device, nodes->grid, nodes->objects[i]))
			return false;
	}
	return true;
}

bool voe_3d_draw_terrain_cast(voe_render_device *device,
			      const voe_3d_models *models,
			      const voe_3d_model_entry *entry,
			      voe_math_float4x4 world, voe_math_float4x4 normal,
			      uint32_t capacity, const struct voe_3d_cull *cull,
			      voe_3d_view_counts *counts)
{
	voe_3d_landscape_node chosen[VOE_3D_LANDSCAPE_NODES];
	voe_3d_models_terrain terrain;
	voe_render_object look = { 0 };
	uint32_t count;

	VOE_BASE_ASSERT(device != NULL, "casting terrain needs a device");
	VOE_BASE_ASSERT(capacity >= 1 && capacity <= VOE_3D_LANDSCAPE_NODES,
			"a cast's capacity outside 1 and the view's nodes");
	count = choose(models, entry, world, &terrain, chosen, capacity);
	VOE_BASE_ASSERT(count == 0 || entry->part_count >= 1,
			"a landscape with no part");
	if (count > 0)
		look = look_of(models, entry, &terrain);
	for (uint32_t i = 0; i < count; i++) {
		const voe_render_object object =
			node_object(world, normal, &chosen[i], terrain.heights,
				    entry->landscape->size, &look);

		if (!node_counted(cull, &object, counts, true))
			continue;
		if (!voe_render_frame_draw(device, terrain.grid, object))
			return false;
	}
	return true;
}

void voe_3d_draw_terrain_hold(struct voe_3d_draw_group *group,
			      const struct voe_3d_terrain_nodes *nodes,
			      float fade, voe_math_float4x4 view,
			      const struct voe_3d_cull *cull,
			      voe_3d_view_counts *counts)
{
	VOE_BASE_ASSERT(group != NULL && nodes != NULL,
			"holding terrain needs a group and nodes");
	VOE_BASE_ASSERT(fade > 0.0f && fade < 1.0f,
			"a fading terrain outside 0 and 1");
	for (uint32_t i = 0; i < nodes->count; i++) {
		struct voe_3d_deferred entry = { .panel = false };

		if (!node_counted(cull, &nodes->objects[i], counts, false))
			continue;
		entry.mesh.geometry = nodes->grid;
		entry.mesh.object = nodes->objects[i];
		entry.mesh.object.shading = nodes->part->faded.index;
		entry.mesh.object.colour.w = 1.0f - fade;
		voe_3d_draw_group_hold(group, entry, entry.mesh.object.world,
				       view);
	}
}
