// Adding, duplicating and deleting entities and their rows, each through the
// world's structural queue. See the header for the id and name rules and for
// what a failure leaves behind.
#include "entities.h"

#include <3d/shape_component.h>

#include <base/assert.h>

#include <ecs/structure.h>

#include <scene/identity_component.h>
#include <scene/transform_component.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

// One more than the largest identity id in the world, or 1 in a world with none.
static uint64_t next_id(const voe_ecs_world *world)
{
	const voe_scene_identity *rows = voe_scene_identity_rows(world);
	uint64_t largest = 0;

	for (uint32_t i = 0; i < voe_scene_identity_count(world); i++)
		if (rows[i].id > largest)
			largest = rows[i].id;

	return largest + 1;
}

static bool name_taken(const voe_ecs_world *world, const char *name)
{
	const voe_scene_identity *rows = voe_scene_identity_rows(world);

	for (uint32_t i = 0; i < voe_scene_identity_count(world); i++)
		if (strcmp(rows[i].name, name) == 0)
			return true;

	return false;
}

// `base` when no identity has it, else "<base> N" for the lowest free N from
// 2. The identities are fewer than N ever needs to count past, so the loop
// always ends; the base is cut short when it and the suffix would not fit.
static void free_name(const voe_ecs_world *world, const char *base,
		      size_t base_length, char name[VOE_SCENE_IDENTITY_NAME])
{
	if (base_length > VOE_SCENE_IDENTITY_NAME - 1)
		base_length = VOE_SCENE_IDENTITY_NAME - 1;

	snprintf(name, VOE_SCENE_IDENTITY_NAME, "%.*s", (int)base_length, base);
	for (uint32_t n = 2; name_taken(world, name); n++) {
		char suffix[16];
		int suffix_length = snprintf(suffix, sizeof suffix, " %u", n);
		size_t room = VOE_SCENE_IDENTITY_NAME - 1 - (size_t)suffix_length;

		snprintf(name, VOE_SCENE_IDENTITY_NAME, "%.*s%s",
			 (int)(base_length < room ? base_length : room), base,
			 suffix);
	}
}

// The duplicate's base: its source's name less a trailing " <number>".
static size_t base_length_of(const char *name)
{
	size_t length = strlen(name);
	size_t digits = length;

	while (digits > 0 && name[digits - 1] >= '0' && name[digits - 1] <= '9')
		digits--;

	if (digits < length && digits > 0 && name[digits - 1] == ' ')
		return digits - 1;

	return length;
}

// Leaves nothing of an entity whose rows could not all be queued. See the
// header on why a direct destroy is safe when the queue is full.
static void undo_create(voe_ecs_world *world, voe_ecs_entity entity)
{
	if (!voe_ecs_structure_destroy(world, entity))
		voe_ecs_entity_destroy(world, entity);
}

static bool queue_default(voe_ecs_world *world, voe_ecs_entity entity,
			  voe_ecs_type type)
{
	const void *row = voe_ecs_component_default(world, type);

	VOE_BASE_ASSERT(row != NULL,
			"adding a component whose type has no default row");

	return voe_ecs_structure_add(world, type, entity, row);
}

bool voe_editor_entities_add(voe_ecs_world *world, voe_editor_add what,
			     voe_ecs_entity *out)
{
	static const char *const bases[] = {
		[VOE_EDITOR_ADD_ENTITY] = "Entity",
		[VOE_EDITOR_ADD_CUBE] = "Cube",
		[VOE_EDITOR_ADD_CAPSULE] = "Capsule",
		[VOE_EDITOR_ADD_CYLINDER] = "Cylinder",
	};
	static const uint32_t kinds[] = {
		[VOE_EDITOR_ADD_CUBE] = VOE_3D_SHAPE_CUBE,
		[VOE_EDITOR_ADD_CAPSULE] = VOE_3D_SHAPE_CAPSULE,
		[VOE_EDITOR_ADD_CYLINDER] = VOE_3D_SHAPE_CYLINDER,
	};
	voe_scene_identity identity = { 0 };
	voe_ecs_entity entity;
	bool ok;

	VOE_BASE_ASSERT(world != NULL, "adding an entity to no world");
	VOE_BASE_ASSERT(out != NULL, "adding an entity with nowhere to put it");
	VOE_BASE_ASSERT(what <= VOE_EDITOR_ADD_CYLINDER,
			"adding something the Add menu does not offer");

	identity.id = next_id(world);
	free_name(world, bases[what], strlen(bases[what]), identity.name);

	if (!voe_ecs_entity_create(world, &entity))
		return false;

	ok = voe_ecs_structure_add(
		world, voe_ecs_component_type(world, &voe_scene_identity_key),
		entity, &identity);

	if (ok && what != VOE_EDITOR_ADD_ENTITY) {
		voe_ecs_type shape_type =
			voe_ecs_component_type(world, &voe_3d_shape_key);
		const voe_3d_shape *shape_default =
			voe_ecs_component_default(world, shape_type);
		voe_3d_shape shape;

		VOE_BASE_ASSERT(shape_default != NULL,
				"a world whose shape type has no default row");
		shape = *shape_default;
		shape.kind = kinds[what];

		ok = queue_default(world, entity,
				   voe_ecs_component_type(
					   world, &voe_scene_transform_key)) &&
		     voe_ecs_structure_add(world, shape_type, entity, &shape);
	}

	if (!ok) {
		undo_create(world, entity);
		return false;
	}

	*out = entity;
	return true;
}

bool voe_editor_entities_component_add(voe_ecs_world *world,
				       voe_ecs_entity entity, voe_ecs_type type)
{
	VOE_BASE_ASSERT(world != NULL, "adding a component in no world");

	return queue_default(world, entity, type);
}

bool voe_editor_entities_component_remove(voe_ecs_world *world,
					  voe_ecs_entity entity,
					  voe_ecs_type type)
{
	VOE_BASE_ASSERT(world != NULL, "removing a component in no world");

	return voe_ecs_structure_remove(world, type, entity);
}

bool voe_editor_entities_delete(voe_ecs_world *world, voe_ecs_entity entity)
{
	VOE_BASE_ASSERT(world != NULL, "deleting an entity in no world");

	return voe_ecs_structure_destroy(world, entity);
}

bool voe_editor_entities_duplicate(voe_ecs_world *world, voe_ecs_entity source,
				   voe_ecs_entity *out)
{
	voe_ecs_type identity_type;
	voe_ecs_entity entity;
	bool ok = true;

	VOE_BASE_ASSERT(world != NULL, "duplicating an entity in no world");
	VOE_BASE_ASSERT(out != NULL,
			"duplicating an entity with nowhere to put the copy");

	identity_type = voe_ecs_component_type(world, &voe_scene_identity_key);

	if (!voe_ecs_entity_create(world, &entity))
		return false;

	for (uint32_t i = 0; ok && i < voe_ecs_component_type_count(world);
	     i++) {
		voe_ecs_type type = voe_ecs_component_type_at(world, i);
		const void *row = voe_ecs_component_get(world, type, source);

		if (row == NULL || voe_ecs_component_runtime_only(world, type))
			continue;

		if (type.value == identity_type.value) {
			const voe_scene_identity *from = row;
			voe_scene_identity copy = { .id = next_id(world) };

			free_name(world, from->name, base_length_of(from->name),
				  copy.name);
			ok = voe_ecs_structure_add(world, type, entity, &copy);
		} else {
			ok = voe_ecs_structure_add(world, type, entity, row);
		}
	}

	if (!ok) {
		undo_create(world, entity);
		return false;
	}

	*out = entity;
	return true;
}
