// The built-in shapes' GPU side, the shape's intent, and the system that drains
// it and turns a shape into a mesh and a material (ADR-0163, ADR-0191).
//
//     voe_3d_shapes shapes;
//
//     if (!voe_3d_shapes_upload(device, &shapes, &error))
//             ...
//
//     // from anywhere, any number of times:
//     if (!voe_3d_shape_submit(world, (voe_3d_shape_intent){
//                 .entity = entity, .shape = recoloured }))
//             ...                                  // the queue is full
//
//     // once a frame, after voe_ecs_structure_apply, before the draw system
//     voe_3d_shape_system_run(world, &shapes);
//
// voe_3d_shapes_upload IS A STARTUP OPERATION, exactly as voe_render_geometry_-
// create and voe_3d_material_upload — which it calls — already are: it waits
// for the card to go idle and may not be called while a frame is being drawn.
// Called once, into a value the program keeps for as long as it runs shapes.
//
// voe_3d_shape_system_run GIVES A MESH AND A MATERIAL TO EVERY SHAPED ENTITY
// THAT HAS NEITHER YET, through 3d's own creation calls — voe_3d_mesh_add and
// voe_3d_material_add — never by writing those tables directly. An entity that
// already has a mesh is given no second one: this is what makes the run
// idempotent, and it is also what leaves an entity that a call site has pointed
// at geometry of its own alone (there is no such call site yet, but the rule is
// the shape's and not an accident of a game having none).
//
// THE WORLD MUST HAVE MESH AND MATERIAL REGISTERED BEFORE THIS RUNS — the same
// requirement voe_3d_mesh_add and voe_3d_material_add already have — and a table
// too small to hold what the run adds is the caller's bug: the run asserts
// rather than dropping the shape silently, because a program that draws shapes
// chose its own capacities.
//
// IT DROPS WHAT IT DERIVED WHEN THE SHAPE IS GONE (0190, ADR-0191). After giving
// meshes, an entity whose mesh is on one of the shapes' geometries and whose
// material is the shapes' own, and which has no shape, gets a structural remove
// for its mesh and its material. So THE WORLD NEEDS A STRUCTURAL QUEUE
// (ecs/structure.h) as soon as anything removes a shape, and a remove the queue
// refuses asserts: the program chose the world's capacities. A removed shape
// draws one more frame, white: the removes are applied at the next
// voe_ecs_structure_apply, and until then the mesh is drawn with no shape to
// give it a colour (3d/draw_system.h). An imported mesh is on other geometry
// and is never touched.
#pragma once

#include <3d/material_component.h>
#include <3d/shape_component.h>

#include <base/error.h>

#include <ecs/world.h>

#include <render/device.h>

// THE CAPACITY CONSTANTS BELOW ARE WHAT A DEVICE MUST HAVE ROOM FOR before
// voe_3d_shapes_upload is called on it — the vertices, indices, geometries and
// shadings the cube, the capsule and the cylinder cost together, one geometry
// each, one shading between them and a second for the selection outline's unlit
// record (0203). A program that draws shapes sizes its voe_render_capacities
// from these. They are written out as numbers so a reader sees the cost;
// 3d/src/shape_system.c asserts at compile time that the vertices, the indices
// and the geometries are the sum of the three shapes' own counts.
#define VOE_3D_SHAPES_VERTICES 750
#define VOE_3D_SHAPES_INDICES 3492
#define VOE_3D_SHAPES_GEOMETRIES 3
#define VOE_3D_SHAPES_SHADINGS 2

// The GPU side of every built-in shape: one geometry per kind, the one white
// material every shape uses and the outline's unlit record. A program keeps one
// of these for as long as it runs the shape system.
typedef struct {
	voe_render_geometry cube;
	voe_render_geometry capsule;
	voe_render_geometry cylinder;
	voe_3d_material material;

	// The record the selection outline's quads wear (0203): white, opaque
	// and UNLIT, so the colour in the drawn object's record is the whole of
	// what they are — ADR-0191's rule for a shape's colour, applied to a
	// line, and the reason the sun cannot change an outline's colour as the
	// camera moves. Nothing in this folder puts it on an entity: it is
	// handed to a pass through voe_3d_frame.
	voe_3d_material outline;
} voe_3d_shapes;

// Creates the cube's, the capsule's and the cylinder's geometry, in that order,
// and uploads two records, in the order it makes them: the shapes' own material
// — opaque, white (1, 1, 1), metallic 0, roughness 0.6, lit — and then the
// outline's, white and opaque too, metallic 0, roughness 1 and unlit. White
// because a shape's own colour is multiplied in through its drawn object's
// record (ADR-0191). False when the device has no room, which is the one way
// this fails (see voe_render_geometry_create and voe_3d_material_upload, both
// of which it calls).
[[nodiscard]] bool voe_3d_shapes_upload(voe_render_device *device,
					voe_3d_shapes *out,
					voe_base_error *error);

// Change this entity's shape to the submitter's row.
//
// THE INTENT CARRIES THE WHOLE ROW, as the transform's does
// (scene/transform_system.h), and is the shape's replace (ecs/component.h). The
// run drains it first. An intent naming a dead entity or one with no shape is
// dropped, silently. A new kind lands as the colour does when it is one of the
// three built-in ones (3d/shape_component.h), because a kind is chosen from
// their names (0195, 0198); a kind that is none of the three is put back to the
// entity's own and reported as a corrected intent. Each colour channel is
// clamped to 0..1, a NaN to 0. Both corrections are reported the way an unknown
// kind is, below.
typedef struct {
	voe_ecs_entity entity;
	voe_3d_shape shape;
} voe_3d_shape_intent;

// False when the queue is full — the system has not run for long enough, and
// the caller is the one that can do something about that.
[[nodiscard]] bool voe_3d_shape_submit(voe_ecs_world *world,
				       voe_3d_shape_intent intent);

// Drains the shape's intents, gives a mesh and a material, both in the world
// layer, to every shape row whose entity has no mesh yet, re-points the mesh of
// every shape whose kind names another of the three geometries than the one it
// is on, and queues the removal of the mesh and material of every entity whose
// shape is gone. See the header for why the world must already have mesh and
// material registered, and why it needs a structural queue.
//
// A KIND THAT CHANGED RE-POINTS THE ENTITY'S MESH (0195, 0198): after that, a
// shaped entity whose mesh is on one of the three shapes' geometries and not on
// the one its kind names is pointed at the kind's, through
// voe_3d_mesh_set_geometry. IT IS A RE-POINT AND NOT A MESH DROPPED AND REBUILT
// because there is nothing to free and nothing to queue: both geometries are
// ranges in the same pools the upload filled, and the material is the one white
// one every kind wears. So THE NEW KIND IS ON THE SCREEN THE SAME FRAME THE RUN
// HAPPENS, no frame late — unlike a removed shape, whose mesh waits for the next
// voe_ecs_structure_apply.
//
// THE PASS IS OVER THE SHAPE TABLE EVERY RUN AND REMEMBERS NOTHING — it compares
// the kind against the geometry the mesh is on rather than against a kind it
// kept from last time. That is what keeps the run idempotent, and it means a row
// a scene file wrote is corrected too, not only one an intent changed. A MESH ON
// GEOMETRY NONE OF THE THREE SHAPES OWNS IS AN IMPORTED MODEL'S AND IS NEVER
// TOUCHED, the same promise as leaving an entity that already has a mesh alone,
// and so is a row whose kind names no geometry at all.
//
// AN UNKNOWN KIND DRAWS NOTHING AND IS A WARNING, EDGE-TRIGGERED LIKE THE SCENE'S
// OTHER DRAINS (see scene/identity_system.c): the first entity found with an
// unknown kind in a run is named on stderr, further ones in the same run add to
// a count, and the count is reported once the run that had any ends. A build
// newer than the file it opened should not print one warning per unknown shape
// in a scene that has many. A corrected intent is reported the same way, with a
// run and a count of its own.
//
// IN A WORLD WITH THE CHANGES TABLE (below), THE RUN CLEARS IT FIRST, then marks
// each drained intent whose colour or kind differs from the row it replaces.
void voe_3d_shape_system_run(voe_ecs_world *world, const voe_3d_shapes *shapes);

// WHICH SHAPES THE LAST RUN RECOLOURED, FOR THE PROBE BOUNCE (0389 point 8). A
// recoloured or re-kinded shape bounces other light, so the bounce recaptures
// the probes about it; without this it could tell only that something moved.
//
// IT IS OPT-IN, as the previous transforms are (scene/transform_system.h). The
// game's world registers it; a world without it pays nothing, and the run then
// marks nothing. The table is runtime-only, so it is never saved, has no replace
// and is not in Add component. ONLY THE RUN WRITES IT. A row outlives its shape
// and is cleared to false by the next run.
//
//     voe_3d_shape_changes_register(world, 4096);    // once, after shapes
//     voe_3d_shape_system_run(world, &shapes);
//     if (voe_3d_shape_changed(world, entity))
//             ...                                    // recapture about it
//
// capacity is how many shapes may be marked: the shape table's. Registering
// twice, or before the shapes, asserts.
void voe_3d_shape_changes_register(voe_ecs_world *world, uint32_t capacity);

// True when the last voe_3d_shape_system_run changed that entity's colour or
// kind. False when the table is not registered or the entity has no row.
[[nodiscard]] bool voe_3d_shape_changed(const voe_ecs_world *world,
					voe_ecs_entity entity);
