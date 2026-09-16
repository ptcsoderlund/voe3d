// The built-in shapes: uploading the geometry and material every shape wears,
// and the run that gives a mesh and a material to a shape that has neither yet.
//
// THE RUN AND THE COUNT ARE FILE-SCOPE STATICS AND THEREFORE PER PROCESS, the
// same trade scene/identity_system.c makes and explains: two worlds in one
// process share them, and nothing here has needed more.
#include "cube.h"

#include <3d/mesh_component.h>
#include <3d/shape_system.h>

#include <base/assert.h>
#include <base/report.h>

#include <inttypes.h>

// The material every shape wears: opaque, lit, and nothing about it named by a
// scene yet — see 3d/shape_system.h and ADR-0163 on why there is no colour
// field to read here instead.
#define GREY 0.7f
#define ROUGHNESS 0.6f

// Whether the last run found an unknown kind, and how many it has found so far
// in the run it belongs to. Per process — see above.
static bool in_run;
static uint32_t run_count;

bool voe_3d_shapes_upload(voe_render_device *device, voe_3d_shapes *out,
			  voe_base_error *error)
{
	voe_render_geometry cube;
	voe_3d_material material = {
		.base_colour = { GREY, GREY, GREY, 1.0f },
		.metallic = 0.0f,
		.roughness = ROUGHNESS,
		.alpha_mode = VOE_RENDER_ALPHA_OPAQUE,
	};

	VOE_BASE_ASSERT(device != NULL, "uploading shapes to no device");
	VOE_BASE_ASSERT(out != NULL, "uploading shapes into nothing");

	if (!voe_render_geometry_create(device, voe_3d_cube_vertices,
					VOE_3D_SHAPES_VERTICES,
					voe_3d_cube_indices,
					VOE_3D_SHAPES_INDICES, &cube, error))
		return false;
	if (!voe_3d_material_upload(device, &material, error))
		return false;

	*out = (voe_3d_shapes){ .cube = cube, .material = material };
	return true;
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
	rows = voe_ecs_component_rows(world, type);
	entities = voe_ecs_component_entities(world, type);
	count = voe_ecs_component_count(world, type);

	for (uint32_t i = 0; i < count; i++) {
		if (voe_3d_mesh_get(world, entities[i]) != NULL)
			continue;

		if (rows[i].kind == VOE_3D_SHAPE_CUBE) {
			VOE_BASE_ASSERT(
				voe_3d_mesh_add(
					world, entities[i],
					(voe_3d_mesh){ .geometry = shapes->cube,
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
