// The built-in shapes: uploading the geometry and material every shape wears,
// the shape's intent and its drain, the run that gives a mesh and a material to
// a shape that has neither yet, re-points the mesh of a shape whose kind
// changed, and removes both once the shape is gone.
//
// THE RUNS AND THE COUNTS ARE FILE-SCOPE STATICS AND THEREFORE PER PROCESS, the
// same trade scene/identity_system.c makes and explains: two worlds in one
// process share them, and nothing here has needed more.
//
// FINDING WHAT TO DROP IS A WALK OF THE MESH TABLE, every run, with two lookups
// per mesh. Meshes are few enough that it has not mattered; a flag on the mesh
// row saying it was derived would lift it.
//
// THE CAPSULE AND THE CYLINDER ARE BUILT ON THE STACK AT UPLOAD, about forty
// kilobytes between them, and forgotten once render has copied them; the cube
// is data and needs no building.
#include "capsule.h"
#include "cube.h"
#include "cylinder.h"

#include <3d/mesh_component.h>
#include <3d/shape_system.h>

#include <base/assert.h>
#include <base/report.h>

#include <ecs/component.h>
#include <ecs/intent.h>
#include <ecs/structure.h>

#include <inttypes.h>
#include <math.h>

// The material every shape wears: opaque, lit and white, so that the shape's
// own colour, multiplied in through the drawn object's record, is the colour
// on screen (ADR-0191).
#define WHITE 1.0f
#define ROUGHNESS 0.6f

static_assert(VOE_3D_SHAPES_VERTICES == VOE_3D_CUBE_VERTICES +
					VOE_3D_CAPSULE_VERTICES +
					VOE_3D_CYLINDER_VERTICES);
static_assert(VOE_3D_SHAPES_INDICES == VOE_3D_CUBE_INDICES +
				       VOE_3D_CAPSULE_INDICES +
				       VOE_3D_CYLINDER_INDICES);
static_assert(VOE_3D_SHAPES_GEOMETRIES == 3);

// Whether the last run found an unknown kind, and how many it has found so far
// in the run it belongs to. Per process — see above.
static bool in_run;
static uint32_t run_count;

// The same two for intents the drain corrected.
static bool in_corrected_run;
static uint32_t corrected_run_count;

bool voe_3d_shapes_upload(voe_render_device *device, voe_3d_shapes *out,
			  voe_base_error *error)
{
	voe_render_geometry cube;
	voe_render_geometry capsule;
	voe_render_geometry cylinder;
	voe_render_vertex capsule_vertices[VOE_3D_CAPSULE_VERTICES];
	uint32_t capsule_indices[VOE_3D_CAPSULE_INDICES];
	voe_render_vertex cylinder_vertices[VOE_3D_CYLINDER_VERTICES];
	uint32_t cylinder_indices[VOE_3D_CYLINDER_INDICES];
	voe_3d_material material = {
		.base_colour = { WHITE, WHITE, WHITE, 1.0f },
		.metallic = 0.0f,
		.roughness = ROUGHNESS,
		.alpha_mode = VOE_RENDER_ALPHA_OPAQUE,
	};

	VOE_BASE_ASSERT(device != NULL, "uploading shapes to no device");
	VOE_BASE_ASSERT(out != NULL, "uploading shapes into nothing");

	voe_3d_capsule_build(capsule_vertices, capsule_indices);
	voe_3d_cylinder_build(cylinder_vertices, cylinder_indices);

	if (!voe_render_geometry_create(device, voe_3d_cube_vertices,
					VOE_3D_CUBE_VERTICES,
					voe_3d_cube_indices,
					VOE_3D_CUBE_INDICES, &cube, error))
		return false;
	if (!voe_render_geometry_create(device, capsule_vertices,
					VOE_3D_CAPSULE_VERTICES,
					capsule_indices, VOE_3D_CAPSULE_INDICES,
					&capsule, error))
		return false;
	if (!voe_render_geometry_create(device, cylinder_vertices,
					VOE_3D_CYLINDER_VERTICES,
					cylinder_indices,
					VOE_3D_CYLINDER_INDICES, &cylinder,
					error))
		return false;
	if (!voe_3d_material_upload(device, &material, error))
		return false;

	*out = (voe_3d_shapes){ .cube = cube,
				.capsule = capsule,
				.cylinder = cylinder,
				.material = material };
	return true;
}

bool voe_3d_shape_submit(voe_ecs_world *world, voe_3d_shape_intent intent)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "submitting a shape to no world");

	return voe_ecs_intent_submit(
		world,
		voe_ecs_component_replace(
			world, voe_ecs_component_type(world, &voe_3d_shape_key))
			.intent,
		&intent);
}

// One channel into 0..1; a NaN becomes 0, because fmaxf returns the number.
static float clamped(float channel)
{
	return fminf(fmaxf(channel, 0.0f), 1.0f);
}

// Whether a kind is one of the three the shapes hold.
static bool built_in(uint32_t kind)
{
	return kind == VOE_3D_SHAPE_CUBE || kind == VOE_3D_SHAPE_CAPSULE ||
	       kind == VOE_3D_SHAPE_CYLINDER;
}

// Applies every waiting intent in submission order and empties the queue, with
// a kind none of the three built-in ones put back and the colour clamped. A
// correction is reported like an unknown kind: the first of a run named, the
// rest counted, the count said once a drain corrects nothing.
static void drain(voe_ecs_world *world, voe_ecs_type type)
{
	voe_ecs_intent queue = voe_ecs_component_replace(world, type).intent;
	const voe_3d_shape_intent *intents = voe_ecs_intent_queue(world, queue);
	uint32_t count = voe_ecs_intent_count(world, queue);
	uint32_t corrected = 0;

	for (uint32_t i = 0; i < count; i++) {
		const voe_3d_shape *own =
			voe_ecs_component_get(world, type, intents[i].entity);
		voe_3d_shape row = intents[i].shape;

		if (own == NULL)
			continue;

		if (!built_in(row.kind))
			row.kind = own->kind;
		row.colour = (voe_math_float3){ clamped(row.colour.x),
						clamped(row.colour.y),
						clamped(row.colour.z) };

		// Compared as submitted, so a NaN — unequal to everything —
		// counts as corrected.
		if (row.kind != intents[i].shape.kind ||
		    !(row.colour.x == intents[i].shape.colour.x &&
		      row.colour.y == intents[i].shape.colour.y &&
		      row.colour.z == intents[i].shape.colour.z)) {
			if (!in_corrected_run && corrected == 0)
				VOE_BASE_WARNING(
					"3d",
					"voe_3d_shape: entity %" PRIu32 "v%" PRIu32
					": kind %" PRIu32 " colour (%g, %g, %g) submitted, kind %" PRIu32
					" colour (%g, %g, %g) applied",
					intents[i].entity.index,
					intents[i].entity.generation,
					intents[i].shape.kind,
					(double)intents[i].shape.colour.x,
					(double)intents[i].shape.colour.y,
					(double)intents[i].shape.colour.z,
					row.kind, (double)row.colour.x,
					(double)row.colour.y,
					(double)row.colour.z);
			corrected++;
		}

		(void)voe_ecs_component_set(world, type, intents[i].entity,
					    &row);
	}

	if (corrected > 0) {
		in_corrected_run = true;
		corrected_run_count += corrected;
	} else if (in_corrected_run) {
		VOE_BASE_WARNING("3d",
				 "voe_3d_shape: %" PRIu32
				 " intents corrected in that run",
				 corrected_run_count);
		in_corrected_run = false;
		corrected_run_count = 0;
	}

	voe_ecs_intent_clear(world, queue);
}

// Whether two geometry ids name the same range.
static bool same_geometry(voe_render_geometry a, voe_render_geometry b)
{
	return a.index == b.index && a.generation == b.generation;
}

// The geometry a kind is drawn with, or NULL when the kind names none — the one
// place the three-way choice is written.
static const voe_render_geometry *geometry_of(uint32_t kind,
					      const voe_3d_shapes *shapes)
{
	return kind == VOE_3D_SHAPE_CUBE	    ? &shapes->cube :
	       kind == VOE_3D_SHAPE_CAPSULE	    ? &shapes->capsule :
	       kind == VOE_3D_SHAPE_CYLINDER ? &shapes->cylinder :
					       NULL;
}

// Whether a mesh is on one of the shapes' geometries.
static bool on_a_shape(const voe_3d_mesh *mesh, const voe_3d_shapes *shapes)
{
	const voe_render_geometry all[] = { shapes->cube, shapes->capsule,
					    shapes->cylinder };

	for (uint32_t i = 0; i < sizeof all / sizeof all[0]; i++)
		if (same_geometry(mesh->geometry, all[i]))
			return true;
	return false;
}

// Points the mesh of every shaped entity whose mesh is on one of the shapes'
// geometries and not on the one its kind names at the kind's — see the header.
// A mesh on any other geometry is an imported model's and is left alone, as is
// a row whose kind names no geometry.
static void re_point(voe_ecs_world *world, const voe_3d_shape *rows,
		     const voe_ecs_entity *entities, uint32_t count,
		     const voe_3d_shapes *shapes)
{
	for (uint32_t i = 0; i < count; i++) {
		const voe_render_geometry *geometry =
			geometry_of(rows[i].kind, shapes);
		const voe_3d_mesh *mesh = voe_3d_mesh_get(world, entities[i]);

		if (geometry == NULL || mesh == NULL ||
		    !on_a_shape(mesh, shapes) ||
		    same_geometry(mesh->geometry, *geometry))
			continue;

		VOE_BASE_ASSERT(
			voe_3d_mesh_set_geometry(world, entities[i], *geometry),
			"a shape's mesh was read and then refused a geometry in the same run");
	}
}

// Queues the removal of the mesh and the material of every entity that is
// drawn as a shape and has none any more — see the header.
static void drop_orphans(voe_ecs_world *world, voe_ecs_type type,
			 const voe_3d_shapes *shapes)
{
	voe_ecs_type mesh_type = voe_ecs_component_type(world, &voe_3d_mesh_key);
	voe_ecs_type material_type =
		voe_ecs_component_type(world, &voe_3d_material_key);
	const voe_3d_mesh *meshes = voe_3d_mesh_rows(world);
	const voe_ecs_entity *owners = voe_3d_mesh_entities(world);
	uint32_t count = voe_3d_mesh_count(world);

	for (uint32_t i = 0; i < count; i++) {
		const voe_3d_material *material;

		if (!on_a_shape(&meshes[i], shapes) ||
		    voe_ecs_component_get(world, type, owners[i]) != NULL)
			continue;
		material = voe_3d_material_get(world, owners[i]);
		if (material == NULL ||
		    material->shading.index != shapes->material.shading.index ||
		    material->shading.generation !=
			    shapes->material.shading.generation)
			continue;

		VOE_BASE_ASSERT(
			voe_ecs_structure_remove(world, mesh_type, owners[i]),
			"the world's structural queue is too small to drop a removed shape's mesh");
		VOE_BASE_ASSERT(
			voe_ecs_structure_remove(world, material_type, owners[i]),
			"the world's structural queue is too small to drop a removed shape's material");
	}
}

void voe_3d_shape_system_run(voe_ecs_world *world, const voe_3d_shapes *shapes)
{
	voe_ecs_type type;
	const voe_3d_shape *rows;
	const voe_ecs_entity *entities;
	uint32_t count;
	uint32_t unknown = 0;

	VOE_BASE_DEBUG_ASSERT(world != NULL, "running the shape system on no world");
	VOE_BASE_DEBUG_ASSERT(shapes != NULL,
			      "running the shape system with no shapes uploaded");

	type = voe_ecs_component_type(world, &voe_3d_shape_key);
	drain(world, type);
	rows = voe_ecs_component_rows(world, type);
	entities = voe_ecs_component_entities(world, type);
	count = voe_ecs_component_count(world, type);

	for (uint32_t i = 0; i < count; i++) {
		const voe_render_geometry *geometry =
			geometry_of(rows[i].kind, shapes);

		if (voe_3d_mesh_get(world, entities[i]) != NULL)
			continue;

		if (geometry != NULL) {
			VOE_BASE_ASSERT(
				voe_3d_mesh_add(
					world, entities[i],
					(voe_3d_mesh){ .geometry = *geometry,
						       .layer = VOE_3D_LAYER_WORLD }),
				"the world's mesh table is too small for its own shapes");
			VOE_BASE_ASSERT(
				voe_3d_material_add(world, entities[i],
						    shapes->material),
				"the world's material table is too small for its own shapes");
		} else {
			// One line at the start of a run and no more: the first
			// unknown kind says what went wrong, and the count at
			// the end says how much of it there was — the same
			// shape scene/identity_system.c's drain reports in.
			if (!in_run && unknown == 0)
				VOE_BASE_WARNING(
					"3d",
					"voe_3d_shape_system_run: entity %" PRIu32
					"v%" PRIu32 ": unknown shape kind %" PRIu32
					", drawing nothing",
					entities[i].index, entities[i].generation,
					rows[i].kind);
			unknown++;
		}
	}

	re_point(world, rows, entities, count, shapes);
	drop_orphans(world, type, shapes);

	if (unknown > 0) {
		in_run = true;
		run_count += unknown;
	} else if (in_run) {
		VOE_BASE_WARNING("3d",
				 "voe_3d_shape_system_run: %" PRIu32
				 " unknown shape kinds in that run",
				 run_count);
		in_run = false;
		run_count = 0;
	}
}
