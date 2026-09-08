// What an entity draws: a range in `render`'s shared geometry pools, and nothing
// else. Read by anyone, const.
//
// IT HOLDS AN ID AND NOT A BUFFER, AND THAT IS THE POINT RATHER THAN A DETAIL. A
// component holding a Vulkan handle could not be written out as text, would leak
// Vulkan into every folder that names a mesh, and could not be read by a thread
// that is not also allowed to touch Vulkan. Two integers serialise, copy and
// sort, and any thread may read them.
//
// IT ALSO SAYS WHICH LAYER THE DRAWABLE IS IN, AND THAT IS HERE RATHER THAN ON
// THE MATERIAL OR THE TRANSFORM. A layer is about when a thing is drawn relative
// to everything else, which is a property of the drawing and not of the surface
// or of where the thing is: two entities sharing one material may sit in
// different layers, and moving something does not move it between them. See
// voe_3d_layer below and 3d/draw_system.h for what the draw does with it.
//
// THERE IS NO MESH SYSTEM AND NO MESH INTENT, AND THE REASON IS WHO WRITES THE
// TABLE. A system and its intent queue exist to let several parties ask for a
// change to data one of them owns, and to have the owner apply those asks in one
// place and one order. This table has nothing to coordinate: every row has
// exactly one writer — whoever made the entity keeps pointing it at geometry, and
// nothing else in the engine writes a mesh — so it is written directly, at
// creation by voe_3d_mesh_add and afterwards by voe_3d_mesh_set_geometry. That
// is the actual boundary, and it is not "creation is special". The day two
// systems both want to write a mesh row — a level-of-detail system and a
// destruction system, say — that is the day this module gets a system and an
// intent, and it is an ADR and not a patch.
//
// GEOMETRY IS THE ONLY FIELD THAT CHANGES AFTER CREATION. The layer is not
// writable and a row is not replaceable wholesale, because the layer is what the
// draw system's depth clear is ordered by, and a layer that could move at run
// time is a way to break that ordering from any call site. Geometry that
// changes — a readout built every frame through
// voe_render_geometry_create_transient — is the case this exists for.
#pragma once

#include <ecs/component.h>
#include <ecs/world.h>
#include <render/device.h>

#include <stdint.h>

// The two layers a drawable can be in. There are two and a third is a decision
// rather than a parameter — a layer costs a depth clear and an ordering rule
// that has to be written down, and neither of those is something to grow by
// counting upwards.
//
// NEITHER OF THESE IS THE NORMAL CASE. An object in the world that gets walked
// behind and an object above the world are equally ordinary: a character and a
// crosshair are both sprites and neither is the odd one out. WORLD is nought
// because a C struct has to have some value when nobody wrote one, and that is
// the whole of why it is first — it is not a statement that the world is the
// default and the overlay an exception to it.
//
// THE LAYER DECIDES ORDER AND NOTHING ELSE. It does not decide lighting: whether
// a surface is lit is `unlit` on its material (3d/material_component.h), and an
// overlay object with a lit material is lit exactly as it would be in the world.
// The two have to drift apart on purpose, because they look alike from a
// distance — text happens to be both unlit and in the overlay, and that is two
// decisions that agreed rather than one.
typedef enum {
	// Drawn with everything else, and covered by anything in front of it.
	VOE_3D_LAYER_WORLD = 0,
	// Drawn after the world's depth has been cleared, so nothing in the
	// world covers it — while it still tests depth against, and is covered
	// by, the rest of the overlay. Real positions in metres, seen through
	// the same camera: this is "always on top", not "screen space".
	VOE_3D_LAYER_OVERLAY,
} voe_3d_layer;

typedef struct {
	voe_render_geometry geometry;
	voe_3d_layer layer;
} voe_3d_mesh;

extern const struct voe_ecs_key voe_3d_mesh_key;

// Registers the table. Once per world, before anything adds a mesh.
void voe_3d_mesh_register(voe_ecs_world *world, uint32_t capacity);

// Gives the entity its geometry and its layer. False when the table is full or
// the entity is not alive. A direct call and not an intent because this table
// has one writer per row and nothing to coordinate — see the header.
[[nodiscard]] bool voe_3d_mesh_add(voe_ecs_world *world, voe_ecs_entity entity,
				   voe_3d_mesh mesh);

// Points the entity's mesh at different geometry and leaves its layer alone.
// False when the entity is not alive or has no mesh. The one field that changes
// after creation, and the whole of what a geometry built this frame needs in
// order to be drawn: make the range, set it here, and the draw system finds it
// when it walks the table.
[[nodiscard]] bool voe_3d_mesh_set_geometry(voe_ecs_world *world,
					    voe_ecs_entity entity,
					    voe_render_geometry geometry);

// NULL when the entity has no mesh or is not alive.
const voe_3d_mesh *voe_3d_mesh_get(const voe_ecs_world *world,
				   voe_ecs_entity entity);

// The table, for the draw system. rows[i] belongs to entities[i].
uint32_t voe_3d_mesh_count(const voe_ecs_world *world);
const voe_3d_mesh *voe_3d_mesh_rows(const voe_ecs_world *world);
const voe_ecs_entity *voe_3d_mesh_entities(const voe_ecs_world *world);
