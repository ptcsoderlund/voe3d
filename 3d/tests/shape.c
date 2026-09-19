// The shape component and the system that turns it into a mesh and a
// material: that kind is described and read-only and colour is a described
// colour, that the default row is a grey cube and a shape needs a transform,
// that the intent lands with kind put back and colour clamped, that a run gives
// a shaped entity exactly one mesh and material and a second run adds nothing,
// that an entity without a shape is untouched, that an unknown kind gets
// nothing, that each kind gets its own geometry, that a removed shape's mesh
// and material are dropped and an imported mesh is not, that the capsule and
// the cylinder are built the right size and the right way out, and that the GPU
// half uploads the three shapes into a device sized from the constants, with
// their one white material.
//
// THE CAPSULE AND CYLINDER BUILDERS ARE INTERNAL, reached through ../src/ the
// way ui/tests/theme.c reaches its own, because what they build is checked
// with no graphics card and a public surface only a test calls is not written.
//
// THE DESCRIPTION IS SWITCHED ON HERE, WHATEVER THE BUILD SAID, for the reason
// scene/tests/identity.c gives at length: check.cmake builds without
// descriptions, and a check that followed the build would never run on the one
// run that gates a card. WHAT THE BUILD SAID IS KEPT FIRST, because
// 3d/src/shape_component.c registers following the build and not this file.
#if defined(VOE_BASE_DESCRIPTIONS) && VOE_BASE_DESCRIPTIONS
#define BUILD_DESCRIBES true
#else
#define BUILD_DESCRIBES false
#endif
#undef VOE_BASE_DESCRIPTIONS
#define VOE_BASE_DESCRIPTIONS 1

#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/shape_component.h>
#include <3d/shape_system.h>

#include "../src/capsule.h"
#include "../src/cylinder.h"

#include <base/arena.h>
#include <base/error.h>

#include <ecs/component.h>
#include <ecs/structure.h>
#include <ecs/world.h>

#include <render/device.h>

#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define SCRATCH (256 * 1024)
#define SIDE 16
#define ENTITIES 8

// Not a real geometry or shading id: the table half never opens a device, so
// what a hand-made voe_3d_shapes carries only has to be copied faithfully into
// the mesh and material tables, and these say so if it were not.
static const voe_render_geometry FAKE_CUBE = { .index = 11, .generation = 22 };
static const voe_render_geometry FAKE_CAPSULE = { .index = 12, .generation = 23 };
static const voe_render_geometry FAKE_CYLINDER = { .index = 13,
						   .generation = 24 };
// A geometry no shape is on, which is what an imported model's mesh names.
static const voe_render_geometry FAKE_IMPORTED = { .index = 14,
						   .generation = 25 };

static const uint32_t UNKNOWN_KIND = 99;

static voe_ecs_world *a_world(voe_base_arena *arena)
{
	voe_ecs_limits limits = {
		.entities = ENTITIES,
		.component_types = 5,
		.intent_types = 2,
		.structure_requests = 4 * ENTITIES,
		.structure_bytes = 1024,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);

	// Transform first: a shape's registration names what it needs.
	voe_scene_transform_register(world, ENTITIES);
	voe_3d_shape_register(world, ENTITIES);
	voe_3d_mesh_register(world, ENTITIES);
	voe_3d_material_register(world, ENTITIES);
	return world;
}

static voe_ecs_entity shaped(voe_ecs_world *world, uint32_t kind)
{
	voe_ecs_entity entity = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_3d_shape_add(world, entity, (voe_3d_shape){
							       .kind = kind,
							       .colour = VOE_3D_SHAPE_GREY }));
	return entity;
}

// A hand-made voe_3d_shapes: what the table half of the run needs, and nothing
// a device could refuse — see FAKE_CUBE above.
static voe_3d_shapes a_shapes_value(void)
{
	return (voe_3d_shapes){
		.cube = FAKE_CUBE,
		.capsule = FAKE_CAPSULE,
		.cylinder = FAKE_CYLINDER,
		.material = { .base_colour = { 1.0f, 1.0f, 1.0f, 1.0f },
			     .roughness = 0.6f,
			     .alpha_mode = VOE_RENDER_ALPHA_OPAQUE },
	};
}

static void check_field(const voe_base_field_description *actual,
			const char *name, voe_base_field_kind kind,
			size_t offset, uint32_t count, bool read_only)
{
	VOE_TEST_CHECK(strcmp(actual->name, name) == 0);
	VOE_TEST_CHECK_INT(actual->kind, kind);
	VOE_TEST_CHECK_INT((long long)actual->offset, (long long)offset);
	VOE_TEST_CHECK_INT(actual->count, count);
	VOE_TEST_CHECK_INT(actual->read_only, read_only);
}

// Kind, read-only — nothing edits a shape's kind after creation — then colour,
// an editable colour: see 3d/shape_component.h.
static void the_description_is_kind_then_colour(void)
{
	const voe_base_struct_description *description =
		voe_3d_shape_description();

	VOE_TEST_CHECK(strcmp(description->name, "voe_3d_shape") == 0);
	VOE_TEST_CHECK_INT(description->field_count, 2);
	if (description->field_count != 2)
		return;

	check_field(&description->fields[0], "kind", VOE_BASE_FIELD_UINT32,
		    offsetof(voe_3d_shape, kind), 1, true);
	check_field(&description->fields[1], "colour", VOE_BASE_FIELD_COLOUR,
		    offsetof(voe_3d_shape, colour), 1, false);
}

// The default row is a grey cube, a shape needs a transform, and the intent is
// the shape's replace with the row after the entity.
static void the_default_row_and_the_needs(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_type type = voe_ecs_component_type(world, &voe_3d_shape_key);
	const voe_3d_shape *row = voe_ecs_component_default(world, type);
	voe_ecs_type needed = { 0 };
	voe_ecs_replace replace = voe_ecs_component_replace(world, type);

	VOE_TEST_CHECK(row != NULL);
	if (row != NULL) {
		VOE_TEST_CHECK_INT(row->kind, VOE_3D_SHAPE_CUBE);
		VOE_TEST_CHECK_FLOAT(row->colour.x, 0.7f, 1e-6f);
		VOE_TEST_CHECK_FLOAT(row->colour.y, 0.7f, 1e-6f);
		VOE_TEST_CHECK_FLOAT(row->colour.z, 0.7f, 1e-6f);
	}

	VOE_TEST_CHECK(voe_ecs_component_needs(world, type, &needed));
	VOE_TEST_CHECK_INT(
		needed.value,
		voe_ecs_component_type(world, &voe_scene_transform_key).value);

	VOE_TEST_CHECK(replace.set);
	VOE_TEST_CHECK_INT((long long)replace.row_offset,
			   (long long)offsetof(voe_3d_shape_intent, shape));
	VOE_TEST_CHECK_INT((long long)replace.value_size,
			   (long long)sizeof(voe_3d_shape_intent));
}

// An intent lands with kind put back to the entity's own and its colour
// applied; a colour out of range lands clamped; an intent for an entity with no
// shape changes nothing.
static void an_intent_lands_with_kind_kept_and_colour_clamped(
	voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_3d_shapes shapes = a_shapes_value();
	voe_ecs_entity cube = shaped(world, VOE_3D_SHAPE_CUBE);
	voe_ecs_entity bare = { 0 };
	const voe_3d_shape *row;

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &bare));

	VOE_TEST_CHECK(voe_3d_shape_submit(
		world, (voe_3d_shape_intent){
			       .entity = cube,
			       .shape = { .kind = VOE_3D_SHAPE_CYLINDER,
					  .colour = { 0.1f, 0.2f, 0.3f } } }));
	VOE_TEST_CHECK(voe_3d_shape_submit(
		world, (voe_3d_shape_intent){
			       .entity = bare,
			       .shape = { .kind = VOE_3D_SHAPE_CUBE,
					  .colour = { 0.1f, 0.2f, 0.3f } } }));
	voe_3d_shape_system_run(world, &shapes);

	row = voe_3d_shape_get(world, cube);
	VOE_TEST_CHECK(row != NULL);
	if (row != NULL) {
		VOE_TEST_CHECK_INT(row->kind, VOE_3D_SHAPE_CUBE);
		VOE_TEST_CHECK_FLOAT(row->colour.x, 0.1f, 1e-6f);
		VOE_TEST_CHECK_FLOAT(row->colour.y, 0.2f, 1e-6f);
		VOE_TEST_CHECK_FLOAT(row->colour.z, 0.3f, 1e-6f);
	}
	VOE_TEST_CHECK(voe_3d_shape_get(world, bare) == NULL);

	VOE_TEST_CHECK(voe_3d_shape_submit(
		world, (voe_3d_shape_intent){
			       .entity = cube,
			       .shape = { .kind = VOE_3D_SHAPE_CUBE,
					  .colour = { 2.0f, -1.0f, 0.5f } } }));
	voe_3d_shape_system_run(world, &shapes);

	row = voe_3d_shape_get(world, cube);
	VOE_TEST_CHECK(row != NULL);
	if (row != NULL) {
		VOE_TEST_CHECK_FLOAT(row->colour.x, 1.0f, 1e-6f);
		VOE_TEST_CHECK_FLOAT(row->colour.y, 0.0f, 1e-6f);
		VOE_TEST_CHECK_FLOAT(row->colour.z, 0.5f, 1e-6f);
	}

	// A run with nothing corrected closes the report's run.
	voe_3d_shape_system_run(world, &shapes);
}

// The world's own introspection: described and not runtime-only, whatever the
// build compiled — see 3d/shape_component.h and its scene/identity_component.h
// precedent.
static void the_type_is_described_and_not_runtime_only(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_type type = voe_ecs_component_type(world, &voe_3d_shape_key);

	VOE_TEST_CHECK(!voe_ecs_component_runtime_only(world, type));

	if (!BUILD_DESCRIBES) {
		VOE_TEST_CHECK(voe_ecs_component_description(world, type) ==
			      NULL);
		return;
	}

	VOE_TEST_CHECK(voe_ecs_component_description(world, type) != NULL);
}

// A shaped cube entity gets exactly one mesh and one material, both the
// shapes' own values, and a second run leaves it exactly as it was — the run
// is idempotent because it skips any entity that already has a mesh.
static void a_run_gives_a_shaped_entity_one_mesh_and_material(
	voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_3d_shapes shapes = a_shapes_value();
	voe_ecs_entity cube = shaped(world, VOE_3D_SHAPE_CUBE);
	const voe_3d_mesh *mesh;
	const voe_3d_material *material;

	voe_3d_shape_system_run(world, &shapes);

	mesh = voe_3d_mesh_get(world, cube);
	VOE_TEST_CHECK(mesh != NULL);
	if (mesh != NULL) {
		VOE_TEST_CHECK_INT(mesh->geometry.index, FAKE_CUBE.index);
		VOE_TEST_CHECK_INT(mesh->geometry.generation,
				   FAKE_CUBE.generation);
		VOE_TEST_CHECK_INT(mesh->layer, VOE_3D_LAYER_WORLD);
	}
	material = voe_3d_material_get(world, cube);
	VOE_TEST_CHECK(material != NULL);
	if (material != NULL)
		VOE_TEST_CHECK_FLOAT(material->base_colour.x, 1.0f, 1e-6f);

	VOE_TEST_CHECK_INT(voe_3d_mesh_count(world), 1);
	VOE_TEST_CHECK_INT(voe_3d_material_count(world), 1);

	// A second run finds the entity already meshed and adds nothing more.
	voe_3d_shape_system_run(world, &shapes);
	VOE_TEST_CHECK_INT(voe_3d_mesh_count(world), 1);
	VOE_TEST_CHECK_INT(voe_3d_material_count(world), 1);
}

// An entity with a transform and no shape is not what the run walks: it stays
// without a mesh, whatever the run does to entities that do have one.
static void an_entity_without_a_shape_is_untouched(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_3d_shapes shapes = a_shapes_value();
	voe_ecs_entity bare = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &bare));

	voe_3d_shape_system_run(world, &shapes);

	VOE_TEST_CHECK(voe_3d_mesh_get(world, bare) == NULL);
	VOE_TEST_CHECK(voe_3d_material_get(world, bare) == NULL);
	VOE_TEST_CHECK_INT(voe_3d_mesh_count(world), 0);
}

// A kind this build does not know draws nothing: no mesh, no material, and the
// run does not stop over it.
static void an_unknown_kind_gets_nothing(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_3d_shapes shapes = a_shapes_value();
	voe_ecs_entity mystery = shaped(world, UNKNOWN_KIND);
	voe_ecs_entity cube = shaped(world, VOE_3D_SHAPE_CUBE);

	voe_3d_shape_system_run(world, &shapes);

	VOE_TEST_CHECK(voe_3d_mesh_get(world, mystery) == NULL);
	VOE_TEST_CHECK(voe_3d_material_get(world, mystery) == NULL);
	VOE_TEST_CHECK(voe_3d_mesh_get(world, cube) != NULL);
	VOE_TEST_CHECK_INT(voe_3d_mesh_count(world), 1);
}

// The geometry an entity of a kind was given, or false when it has no mesh.
static bool has_geometry(const voe_ecs_world *world, voe_ecs_entity entity,
			 voe_render_geometry geometry)
{
	const voe_3d_mesh *mesh = voe_3d_mesh_get(world, entity);

	return mesh != NULL && mesh->geometry.index == geometry.index &&
	       mesh->geometry.generation == geometry.generation;
}

// Each kind is drawn with its own geometry, and all three wear the one
// material.
static void each_kind_gets_its_own_geometry(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_3d_shapes shapes = a_shapes_value();
	voe_ecs_entity cube = shaped(world, VOE_3D_SHAPE_CUBE);
	voe_ecs_entity capsule = shaped(world, VOE_3D_SHAPE_CAPSULE);
	voe_ecs_entity cylinder = shaped(world, VOE_3D_SHAPE_CYLINDER);

	voe_3d_shape_system_run(world, &shapes);

	VOE_TEST_CHECK(has_geometry(world, cube, FAKE_CUBE));
	VOE_TEST_CHECK(has_geometry(world, capsule, FAKE_CAPSULE));
	VOE_TEST_CHECK(has_geometry(world, cylinder, FAKE_CYLINDER));
	VOE_TEST_CHECK_INT(voe_3d_material_count(world), 3);
}

// Removing a shape, applying, running and applying again leaves the entity
// with no mesh and no material; an imported-style entity — a mesh on other
// geometry and no shape — keeps its mesh through all of it.
static void a_removed_shape_drops_its_mesh_and_material(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_3d_shapes shapes = a_shapes_value();
	voe_ecs_entity cube = shaped(world, VOE_3D_SHAPE_CUBE);
	voe_ecs_entity imported = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &imported));
	VOE_TEST_CHECK(voe_3d_mesh_add(
		world, imported,
		(voe_3d_mesh){ .geometry = FAKE_IMPORTED,
			       .layer = VOE_3D_LAYER_WORLD }));
	VOE_TEST_CHECK(voe_3d_material_add(world, imported, shapes.material));

	voe_3d_shape_system_run(world, &shapes);
	VOE_TEST_CHECK(voe_3d_mesh_get(world, cube) != NULL);
	VOE_TEST_CHECK_INT(voe_ecs_structure_count(world), 0);

	VOE_TEST_CHECK(voe_ecs_structure_remove(
		world, voe_ecs_component_type(world, &voe_3d_shape_key), cube));
	voe_ecs_structure_apply(world);
	voe_3d_shape_system_run(world, &shapes);
	voe_ecs_structure_apply(world);

	VOE_TEST_CHECK(voe_3d_shape_get(world, cube) == NULL);
	VOE_TEST_CHECK(voe_3d_mesh_get(world, cube) == NULL);
	VOE_TEST_CHECK(voe_3d_material_get(world, cube) == NULL);
	VOE_TEST_CHECK(voe_3d_mesh_get(world, imported) != NULL);
	VOE_TEST_CHECK(voe_3d_material_get(world, imported) != NULL);
}

static voe_math_float3 minus(voe_math_float3 a, voe_math_float3 b)
{
	return (voe_math_float3){ a.x - b.x, a.y - b.y, a.z - b.z };
}

static float dot(voe_math_float3 a, voe_math_float3 b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

// What every built shape has to be, whatever it is: unit normals, every vertex
// inside its box and each face of the box touched, every index a vertex, and
// every triangle wound counter-clockwise from outside — its face normal points
// away from the Y axis, or on a flat cap or a rounded end away from the origin.
static void check_built(const voe_render_vertex *vertices,
			uint32_t vertex_count, const uint32_t *indices,
			uint32_t index_count, float half_height)
{
	float most_x = 0.0f;
	float most_y = 0.0f;
	float most_z = 0.0f;
	uint32_t inside = 0;
	uint32_t unit = 0;
	uint32_t in_range = 0;
	uint32_t outward = 0;

	for (uint32_t i = 0; i < vertex_count; i++) {
		voe_math_float3 p = vertices[i].position;
		voe_math_float3 n = vertices[i].normal;

		if (fabsf(sqrtf(dot(n, n)) - 1.0f) <= 1e-4f)
			unit++;
		if (fabsf(p.x) <= 0.5f + 1e-4f && fabsf(p.z) <= 0.5f + 1e-4f &&
		    fabsf(p.y) <= half_height + 1e-4f)
			inside++;
		most_x = fmaxf(most_x, fabsf(p.x));
		most_y = fmaxf(most_y, fabsf(p.y));
		most_z = fmaxf(most_z, fabsf(p.z));
	}
	VOE_TEST_CHECK_INT(unit, vertex_count);
	VOE_TEST_CHECK_INT(inside, vertex_count);
	VOE_TEST_CHECK_FLOAT(most_x, 0.5f, 1e-4f);
	VOE_TEST_CHECK_FLOAT(most_y, half_height, 1e-4f);
	VOE_TEST_CHECK_FLOAT(most_z, 0.5f, 1e-4f);

	for (uint32_t i = 0; i < index_count; i++)
		if (indices[i] < vertex_count)
			in_range++;
	VOE_TEST_CHECK_INT(in_range, index_count);
	if (in_range != index_count)
		return;

	for (uint32_t i = 0; i + 2 < index_count; i += 3) {
		voe_math_float3 a = vertices[indices[i]].position;
		voe_math_float3 b = vertices[indices[i + 1]].position;
		voe_math_float3 c = vertices[indices[i + 2]].position;
		voe_math_float3 ab = minus(b, a);
		voe_math_float3 ac = minus(c, a);
		voe_math_float3 face = { ab.y * ac.z - ab.z * ac.y,
					 ab.z * ac.x - ab.x * ac.z,
					 ab.x * ac.y - ab.y * ac.x };
		voe_math_float3 centre = { (a.x + b.x + c.x) / 3.0f,
					   (a.y + b.y + c.y) / 3.0f,
					   (a.z + b.z + c.z) / 3.0f };
		voe_math_float3 from_axis = { centre.x, 0.0f, centre.z };

		if (dot(face, from_axis) > 1e-9f || dot(face, centre) > 1e-9f)
			outward++;
	}
	VOE_TEST_CHECK_INT(outward, index_count / 3);
}

// A capsule is two metres from end to end and a cylinder one metre tall, both
// half a metre in radius — ADR-0191.
static void the_capsule_and_cylinder_are_built_right(void)
{
	static voe_render_vertex vertices[VOE_3D_CAPSULE_VERTICES];
	static uint32_t indices[VOE_3D_CAPSULE_INDICES];

	voe_3d_capsule_build(vertices, indices);
	check_built(vertices, VOE_3D_CAPSULE_VERTICES, indices,
		    VOE_3D_CAPSULE_INDICES, 1.0f);

	voe_3d_cylinder_build(vertices, indices);
	check_built(vertices, VOE_3D_CYLINDER_VERTICES, indices,
		    VOE_3D_CYLINDER_INDICES, 0.5f);
}

// The GPU half: a device sized exactly from the constants a program is told to
// size it from, the material voe_3d_shapes_upload writes, and a run that puts
// a capsule and a cylinder on the geometries it made for them.
//
// IT NEEDS A GRAPHICS CARD, and skips with a reason without one, for the
// reason 3d/tests/material.c gives at length: a build box with no Vulkan is
// the box and not this engine.
static void the_upload_makes_three_shapes_and_a_white_material(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	voe_render_capacities capacities = {
		.vertices = VOE_3D_SHAPES_VERTICES,
		.indices = VOE_3D_SHAPES_INDICES,
		.geometries = VOE_3D_SHAPES_GEOMETRIES,
		.objects = 1,
		.shadings = VOE_3D_SHAPES_SHADINGS,
		.passes = 1,
	};
	voe_render_device *device =
		voe_render_device_new_headless(arena, size, capacities, &error);
	voe_3d_shapes shapes;

	if (device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED) {
			printf("skip: %s\n", voe_base_error_string(error));
			voe_base_arena_destroy(arena);
			return;
		}
		VOE_TEST_CHECK(device != NULL);
		voe_base_arena_destroy(arena);
		return;
	}

	VOE_TEST_CHECK(voe_3d_shapes_upload(device, &shapes, &error));
	{
		voe_ecs_world *world = a_world(arena);
		voe_ecs_entity capsule = shaped(world, VOE_3D_SHAPE_CAPSULE);
		voe_ecs_entity cylinder = shaped(world, VOE_3D_SHAPE_CYLINDER);

		voe_3d_shape_system_run(world, &shapes);
		VOE_TEST_CHECK(has_geometry(world, capsule, shapes.capsule));
		VOE_TEST_CHECK(has_geometry(world, cylinder, shapes.cylinder));
	}
	VOE_TEST_CHECK_FLOAT(shapes.material.base_colour.x, 1.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(shapes.material.base_colour.y, 1.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(shapes.material.base_colour.z, 1.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(shapes.material.metallic, 0.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(shapes.material.roughness, 0.6f, 1e-6f);
	VOE_TEST_CHECK_INT(shapes.material.alpha_mode, VOE_RENDER_ALPHA_OPAQUE);
	VOE_TEST_CHECK(!shapes.material.unlit);

	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);

	the_description_is_kind_then_colour();
	the_type_is_described_and_not_runtime_only(arena);
	the_default_row_and_the_needs(arena);
	an_intent_lands_with_kind_kept_and_colour_clamped(arena);
	a_run_gives_a_shaped_entity_one_mesh_and_material(arena);
	an_entity_without_a_shape_is_untouched(arena);
	an_unknown_kind_gets_nothing(arena);
	each_kind_gets_its_own_geometry(arena);
	a_removed_shape_drops_its_mesh_and_material(arena);
	the_capsule_and_cylinder_are_built_right();

	voe_base_arena_destroy(arena);

	the_upload_makes_three_shapes_and_a_white_material();

	return voe_test_result();
}
