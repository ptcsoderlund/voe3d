// What an entity draws when the thing it draws is a surface of elements: a
// range of this frame's element buffer, how big that surface is in millimetres,
// and which layer it is drawn in. Read by anyone, const.
//
// THE RANGE IS ONE FRAME'S AND IS WRONG THE MOMENT THAT FRAME HAS ENDED. There
// is one element buffer per frame slot, it is empty at the top of every frame,
// and a range is two numbers about the buffer that is open now — so a row left
// alone from one frame to the next points at records that no longer exist. That
// is not a flaw to be worked around: it is what immediate mode is. The program
// rewrites the range every frame, in the phase the loop reserves for exactly
// this — after voe_render_frame_begin and before the draw system walks (see
// 3d/draw_system.h) — and a panel it forgot is a panel that is not drawn.
//
// A PANEL IS A COMPONENT AND NOT AN ENTITY. An entity is an id and nothing else
// (ecs/entity.h); what a panel is, is a row in this table keyed by one, sitting
// beside the voe_scene_transform row that says where it is. That is the same
// shape a drawn mesh has and it is deliberately not a different one.
//
// IT HOLDS NO MATRIX. Where a panel stands is its transform's, exactly as for a
// mesh, and the draw system already reads that table. What the draw system
// composes is projection × view × the transform's matrix ×
// voe_render_element_surface_matrix(size) — the last of which is `render`'s and
// is where element space's Y-down convention meets the world's Y-up one, once.
//
// THE SIZE IS HERE AND IS NOT IMPLIED BY THE ELEMENTS. Forty rectangles do not
// say how big the paper is: they say where forty rectangles are on it. The
// surface's own rectangle is what the millimetre-to-metre step needs in order to
// centre the surface on the entity's position, and it is what hit-testing will
// need when something asks which panel a pointer is over.
//
// AND A MILLIMETRE HERE IS A REAL MILLIMETRE (ADR-0089). A panel is an object in
// the world: 240 mm across is 0.24 m across, before the entity's own transform
// scales it. That is not the meaning a millimetre has on a surface mapped
// straight onto the window, where it is a proportion of that surface's authored
// height — see voe_render_element_transform, which is the other one. Nothing
// about a panel answers to the window's size or to a calibration.
//
// THERE IS NO PANEL SYSTEM AND NO PANEL INTENT, for the reason
// 3d/mesh_component.h gives about meshes: every row has exactly one writer —
// whoever builds the surface's elements each frame — so there is nothing to
// coordinate and the table is written directly.
//
// THE RANGE IS THE ONLY THING THAT CHANGES AFTER CREATION. The size and the
// layer are not writable, the same way a mesh's layer is not: the layer is what
// the draw system's depth clear is ordered by, and a size that could change
// between the layout and the draw would put the elements somewhere other than
// where they were laid out.
#pragma once

// For voe_3d_layer, which lives beside the mesh because a mesh was the first
// drawable to need one. A panel using the same enum rather than a second one is
// the point: a layer is a property of the drawing and says nothing about what
// kind of thing is being drawn.
#include <3d/mesh_component.h>

#include <ecs/component.h>
#include <ecs/world.h>
#include <math/float2.h>

#include <stdint.h>

typedef struct {
	// Where this surface's records start in the open frame's element
	// buffer, and how many of them there are. Read
	// voe_render_frame_elements_submitted before and after submitting the
	// surface; the first number is `first` and the difference is `count`.
	uint32_t first;
	uint32_t count;
	// How big the surface is in millimetres — the paper, not the ink.
	voe_math_float2 size;
	// Which side of the overlay's depth clear this is drawn on. A panel in
	// the world is hidden by what stands in front of it; a panel in the
	// overlay is not, while still keeping a real position in metres.
	voe_3d_layer layer;
} voe_3d_panel;

extern const struct voe_ecs_key voe_3d_panel_key;

// Registers the table. Once per world, before anything adds a panel.
void voe_3d_panel_register(voe_ecs_world *world, uint32_t capacity);

// Gives the entity its surface. False when the table is full or the entity is
// not alive. A direct call and not an intent because this table has one writer
// per row — see the header.
[[nodiscard]] bool voe_3d_panel_add(voe_ecs_world *world, voe_ecs_entity entity,
				    voe_3d_panel panel);

// Points the panel at this frame's range and leaves its size and layer alone.
// False when the entity is not alive or has no panel.
//
// THIS IS CALLED EVERY FRAME AND THAT IS NOT A COST WORTH AVOIDING. It is two
// integers into a row that is already in cache; what would be expensive is a
// panel drawing last frame's rectangles.
[[nodiscard]] bool voe_3d_panel_set_range(voe_ecs_world *world,
					  voe_ecs_entity entity, uint32_t first,
					  uint32_t count);

// NULL when the entity has no panel or is not alive.
const voe_3d_panel *voe_3d_panel_get(const voe_ecs_world *world,
				     voe_ecs_entity entity);

// The table, for the draw system. rows[i] belongs to entities[i].
uint32_t voe_3d_panel_count(const voe_ecs_world *world);
const voe_3d_panel *voe_3d_panel_rows(const voe_ecs_world *world);
const voe_ecs_entity *voe_3d_panel_entities(const voe_ecs_world *world);
