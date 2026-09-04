// What an entity draws: a range in `render`'s shared geometry pools, and nothing
// else. Read by anyone, const.
//
// IT HOLDS AN ID AND NOT A BUFFER, AND THAT IS THE POINT RATHER THAN A DETAIL. A
// component holding a Vulkan handle could not be written out as text, would leak
// Vulkan into every folder that names a mesh, and could not be read by a thread
// that is not also allowed to touch Vulkan. Two integers serialise, copy and
// sort, and any thread may read them.
//
// THERE IS NO MESH SYSTEM AND NO MESH INTENT YET, WHICH IS RULE 10 AND NOT AN
// OMISSION. Nothing in the engine changes which geometry an entity draws after
// it is built: an importer writes one and the draw system reads it. The card that
// swaps a mesh at run time — a level of detail, a destroyed variant — is the card
// that gives this module a system.
#pragma once

#include <ecs/component.h>
#include <ecs/world.h>
#include <render/device.h>

#include <stdint.h>

typedef struct {
	voe_render_geometry geometry;
} voe_3d_mesh;

extern const struct voe_ecs_key voe_3d_mesh_key;

// Registers the table. Once per world, before anything adds a mesh.
void voe_3d_mesh_register(voe_ecs_world *world, uint32_t capacity);

// Gives the entity its geometry. False when the table is full or the entity is
// not alive. It is a direct call and not an intent because it is creation — see
// the header on why there is no system here at all.
[[nodiscard]] bool voe_3d_mesh_add(voe_ecs_world *world, voe_ecs_entity entity,
				   voe_3d_mesh mesh);

// NULL when the entity has no mesh or is not alive.
const voe_3d_mesh *voe_3d_mesh_get(const voe_ecs_world *world,
				   voe_ecs_entity entity);

// The table, for the draw system. rows[i] belongs to entities[i].
uint32_t voe_3d_mesh_count(const voe_ecs_world *world);
const voe_3d_mesh *voe_3d_mesh_rows(const voe_ecs_world *world);
const voe_ecs_entity *voe_3d_mesh_entities(const voe_ecs_world *world);
