// The built-in shapes' GPU side, and the system that turns a shape into a mesh
// and a material (ADR-0163).
//
//     voe_3d_shapes shapes;
//
//     if (!voe_3d_shapes_upload(device, &shapes, &error))
//             ...
//
//     // once a frame, before the draw system walks
//     voe_3d_shape_system_run(world, &shapes);
//
// voe_3d_shapes_upload IS A STARTUP OPERATION, exactly as voe_render_geometry_-
// create and voe_3d_material_upload — which it calls — already are: it waits
// for the card to go idle and may not be called while a frame is being drawn.
// Called once, into a value the program keeps for as long as it runs shapes.
//
// THE CAPACITY CONSTANTS BELOW ARE WHAT A DEVICE MUST HAVE ROOM FOR before
// voe_3d_shapes_upload is called on it — the vertices, indices, geometries and
// shadings one cube costs. A program that draws shapes sizes its
// voe_render_capacities from these; today they are all the one cube spends.
//
// voe_3d_shape_system_run GIVES A MESH AND A MATERIAL TO EVERY SHAPED ENTITY
// THAT HAS NEITHER YET, through 3d's own creation calls — voe_3d_mesh_add and
// voe_3d_material_add — never by writing those tables directly. An entity that
// already has a mesh is left alone: this is what makes the run idempotent, and
// it is also what leaves an entity's mesh alone after a call site has pointed it
// at different geometry on purpose (there is no such call site yet, but the rule
// is the shape's and not an accident of a game having none).
//
// THE WORLD MUST HAVE MESH AND MATERIAL REGISTERED BEFORE THIS RUNS — the same
// requirement voe_3d_mesh_add and voe_3d_material_add already have — and a table
// too small to hold what the run adds is the caller's bug: the run asserts
// rather than dropping the shape silently, because a program that draws shapes
// chose its own capacities.
//
// AN UNKNOWN KIND DRAWS NOTHING AND IS A WARNING, EDGE-TRIGGERED LIKE THE SCENE'S
// OTHER DRAINS (see scene/identity_system.c): the first entity found with an
// unknown kind in a run is named on stderr, further ones in the same run add to
// a count, and the count is reported once the run that had any ends. A build
// newer than the file it opened should not print one warning per unknown shape
// in a scene that has many.
#pragma once

#include <3d/material_component.h>
#include <3d/shape_component.h>

#include <base/error.h>

#include <ecs/world.h>

#include <render/device.h>

// The vertices, indices, geometries and shadings one cube costs — see the
// header.
#define VOE_3D_SHAPES_VERTICES 24
#define VOE_3D_SHAPES_INDICES 36
#define VOE_3D_SHAPES_GEOMETRIES 1
#define VOE_3D_SHAPES_SHADINGS 1

// The GPU side of every built-in shape: today, one cube's geometry and the one
// grey material every shape uses. A program keeps one of these for as long as
// it runs the shape system.
typedef struct {
	voe_render_geometry cube;
	voe_3d_material material;
} voe_3d_shapes;

// Creates the cube's geometry and uploads the shapes' one material — opaque,
// grey (0.7, 0.7, 0.7), metallic 0, roughness 0.6, lit. False when the device
// has no room, which is the one way this fails (see voe_render_geometry_create
// and voe_3d_material_upload, both of which it calls).
[[nodiscard]] bool voe_3d_shapes_upload(voe_render_device *device,
					voe_3d_shapes *out,
					voe_base_error *error);

// Gives a mesh and a material, both in the world layer, to every shape row
// whose entity has no mesh yet. See the header for what an unknown kind does
// and why the world must already have mesh and material registered.
void voe_3d_shape_system_run(voe_ecs_world *world, const voe_3d_shapes *shapes);
