// The prefab cook: one prefab's world into the C function a game spawns it
// with (authoring/prefab.h, ADR-0283 point 9).
//
// Everything but the order and the root's transform is scene_cook.c's, through
// cook_text.h: the authored entities are collected and sorted by id there, and
// `order` maps each to its place in `entities`, the root first and the rest
// ascending, so an ENTITY field inside the prefab is spelled `entities[j]`.
//
// THE ROOT IS FOUND BY A LINEAR SCAN for the one authored entity with no parent
// row. A prefab is at most 32 things (0283 point 9), so nothing lifts it.
#include <authoring/prefab.h>

#include "cook_text.h"

#include <base/assert.h>
#include <base/report.h>
#include <scene/identity_component.h>
#include <scene/parent_component.h>
#include <scene/transform_component.h>

#include <inttypes.h>
#include <stdint.h>

#define MODULE "authoring"

// The index of the one authored entity with no parent; false, reported, when
// there is none or more than one.
static bool find_root(const voe_authoring_cook *c, uint32_t *out_root)
{
	uint32_t roots = 0;

	VOE_BASE_ASSERT(c != NULL && out_root != NULL, "a cook and an out");
	for (uint32_t i = 0; i < c->authored_count; i++) {
		if (voe_scene_parent_get(c->world, c->authored[i].entity) != NULL)
			continue;
		*out_root = i;
		roots++;
	}
	if (roots == 1)
		return true;
	VOE_BASE_ERROR(MODULE,
		       "a prefab to cook has %" PRIu32 " entities with no parent, "
		       "and it must have exactly one root",
		       roots);
	return false;
}

// The root's transform is where the spawn puts it, not its row.
static void put_root_transform(voe_authoring_cook *c)
{
	VOE_BASE_ASSERT(c != NULL && c->array != NULL, "a cook with an array");
	voe_authoring_cook_put(
		c, "\tif (!voe_ecs_structure_add(world, voe_ecs_component_type("
		   "world, &voe_scene_transform_key), entities[0], "
		   "&(voe_scene_transform){ .position = position, .rotation = "
		   "rotation, .scale = { 0x1p+0f, 0x1p+0f, 0x1p+0f } }))\n"
		   "\t\treturn false;\n");
}

// Every row of authored[index] but its identity, the root's transform replaced.
static bool put_entity_rows(voe_authoring_cook *c, uint32_t index, bool root)
{
	VOE_BASE_ASSERT(c != NULL && index < c->authored_count, "an entity");
	c->id = c->authored[index].id;
	for (uint32_t t = 0; t < c->type_count; t++) {
		const voe_authoring_cook_type *type = &c->types[t];

		if (type->key == voe_scene_identity_key.name)
			continue;
		if (root && type->key == voe_scene_transform_key.name &&
		    voe_scene_transform_get(c->world,
					    c->authored[index].entity) != NULL) {
			put_root_transform(c);
			continue;
		}
		if (!voe_authoring_cook_row(c, "voe_ecs_structure_add", index,
					    type))
			return false;
	}
	return true;
}

bool voe_authoring_prefab_cook(const voe_ecs_world *world,
			       const char *function, voe_base_arena *arena,
			       voe_authoring_text *out, uint32_t *out_entities)
{
	VOE_BASE_ASSERT(world != NULL && function != NULL, "a world and a name");
	VOE_BASE_ASSERT(arena != NULL && out != NULL && out_entities != NULL,
			"an arena and outs");
	voe_authoring_cook c = { .world = world, .arena = arena,
				 .array = "entities" };
	uint32_t root = 0;

	if (!voe_authoring_cook_collect(&c) || !find_root(&c, &root))
		return false;
	uint32_t *order = voe_base_arena_push(arena, c.authored_count *
							     sizeof(uint32_t));
	for (uint32_t i = 0; i < c.authored_count; i++)
		order[i] = i < root ? i + 1 : i == root ? 0 : i;
	c.order = order;

	voe_authoring_cook_put(&c, "static bool ");
	voe_authoring_cook_put(&c, function);
	voe_authoring_cook_put(
		&c, "(voe_ecs_world *world, const voe_ecs_entity *entities,\n"
		    "\tvoe_math_double3 position, voe_math_quat rotation)\n{\n");
	if (!put_entity_rows(&c, root, true))
		return false;
	for (uint32_t i = 0; i < c.authored_count; i++)
		if (i != root && !put_entity_rows(&c, i, false))
			return false;
	// A root with no transform leaves the parameters unused.
	if (voe_scene_transform_get(world, c.authored[root].entity) == NULL)
		voe_authoring_cook_put(&c, "\t(void)world;\n\t(void)entities;\n"
					   "\t(void)position;\n"
					   "\t(void)rotation;\n");
	voe_authoring_cook_put(&c, "\treturn true;\n}\n");
	*out = (voe_authoring_text){ .text = c.bytes, .size = c.size };
	*out_entities = c.authored_count;
	return true;
}
