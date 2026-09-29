// The scene writer's walk: which entities and sections are written, in what
// order, with the kept sections merged in. One field's value is value_write.c's.
//
// THE SPLIT FALLS AT ONE FIELD. What is here refuses what is wrong with an
// entity or an id — two entities under one authored id, a kept section whose
// name a type has since taken; what is wrong with one value is refused where the
// value is spelled.
//
// TWO PASSES OVER THE SAME WALK, AND THAT IS HOW THE TEXT IS ONE PUSH OF EXACTLY
// ITS SIZE. Two pushes are not guaranteed to be adjacent (base/arena.h), so text
// that grew a push at a time could not be handed back as one span. The first pass
// measures — every put adds to a count and writes nothing — and it is also the
// pass that refuses and reports; the second pass writes into a buffer pushed at
// that size. Both passes go through put_scene(), so they cannot disagree about a
// byte, and the second cannot refuse anything the first accepted.
//
// ORDER IS SORTED OUT BEFORE ANYTHING IS WRITTEN. The authored entities are
// copied out of the identity table and merge-sorted by id (authored.h), the
// component types are sorted by key name, and a copy of the kept sections by id
// and then key name. None uses qsort, whose comparator is a function pointer, and
// function pointers in this engine are render's loader table alone. The ids are
// merge-sorted because a scene may hold thousands; the types are insertion-sorted
// because a world holds a handful; the kept sections are insertion-sorted because
// they arrive from a file this writer wrote, already in that order, and insertion
// sort is one comparison per item on sorted input.
//
// A KEPT SECTION IS MERGED INTO ITS ENTITY'S COMPONENT SECTIONS AS THEY ARE
// WRITTEN, two sorted runs walked side by side, so it lands where its key name
// sorts without the described types and the kept ones ever sharing an array.
//
// A PLACED COPY IS WRITTEN AS ITS ROOT ALONE (0283 point 3). An entity whose
// part row names another is skipped, kept sections and all; one whose part row
// names itself gets its identity, transform, parent and prefab sections and its
// kept ones. A world with no part table writes every entity whole.
//
// A PREFAB IS THIS WALK NARROWED TO ONE TREE (scene_tree.h): the authored table
// holds only the tree's entities, and the root's rows pass through row_written().
//
// THE IDENTITY TYPE IS FOUND BY WALKING THE TYPES, not with
// voe_ecs_component_type, which asserts on a world that registered none — and a
// world with no identities is an ordinary world with nothing authored in it.
#include <authoring/scene_write.h>

#include "authored.h"
#include "scene_tree.h"
#include "value_write.h"

#include <base/assert.h>
#include <base/describe.h>
#include <base/report.h>
#include <ecs/component.h>
#include <scene/identity_component.h>
#include <scene/parent_component.h>
#include <scene/prefab_component.h>
#include <scene/transform_component.h>

#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#define MODULE "authoring"

struct described {
	voe_ecs_type type;
	const struct voe_ecs_key *ecs_key;
	const char *key;
	// NULL when this build compiled the description out.
	const voe_base_struct_description *description;
	// Transform, parent or prefab: written for a placed copy's root.
	bool on_copy_root;
};

struct scene {
	const voe_ecs_world *world;
	// NULL for the whole world.
	const voe_authoring_tree *tree;

	bool has_identity;
	voe_ecs_type identity;

	// Ascending by id.
	voe_authoring_authored *authored;
	uint32_t authored_count;

	// Every described type but the identity, ascending by key name.
	struct described *described;
	uint32_t described_count;

	// A copy of the kept sections, ascending by id and then by key name.
	voe_authoring_kept_section *kept;
	uint32_t kept_count;
};

static bool registered(const voe_ecs_world *world, const char *name)
{
	uint32_t count = voe_ecs_component_type_count(world);

	for (uint32_t i = 0; i < count; i++) {
		voe_ecs_type type = voe_ecs_component_type_at(world, i);

		if (strcmp(voe_ecs_component_key(world, type)->name, name) == 0)
			return true;
	}
	return false;
}

// DEVIATION: card 071 scope 3, a kept section whose key name some registered type
// now has is refused, which the card does not list. It was kept because nothing
// had registered that name; written back beside that type's own section it is the
// same section twice, and beside a runtime-only one it is a section the reader
// refuses — either way a file that cannot be read back as what was saved, which is
// what this writer refuses rather than writes.
static bool put_kept(voe_authoring_output *text, const struct scene *scene,
		     const voe_authoring_kept_section *kept)
{
	if (registered(scene->world, kept->key)) {
		if (voe_authoring_measuring(text))
			VOE_BASE_ERROR(MODULE,
				       "entity %" PRIu64 " has a kept section %s, "
				       "and a type has been registered under that "
				       "name since; writing it would give a file "
				       "that cannot be read back; nothing was "
				       "written",
				       kept->id, kept->key);
		return false;
	}

	voe_authoring_put_string(text, "[");
	voe_authoring_put_unsigned(text, kept->id);
	voe_authoring_put_string(text, ".");
	voe_authoring_put_string(text, kept->key);
	voe_authoring_put_string(text, "]\n");
	voe_authoring_put(text, kept->lines, kept->size);
	return true;
}

static bool same_entity(voe_ecs_entity a, voe_ecs_entity b)
{
	return a.index == b.index && a.generation == b.generation;
}

// The row this entity writes for `described`, or NULL for none. A prefab's
// root writes no parent and its transform at the origin, with no turn and
// scale one (0283 point 1), copied into `origin`; every other row is as it is.
static const uint8_t *row_written(const struct scene *scene,
				  voe_ecs_entity entity,
				  const struct described *described,
				  voe_scene_transform *origin)
{
	const uint8_t *row = voe_ecs_component_get(scene->world,
						   described->type, entity);

	VOE_BASE_ASSERT(origin != NULL, "nowhere to put the root's origin");
	if (row == NULL || scene->tree == NULL ||
	    !same_entity(entity, scene->tree->root))
		return row;
	if (described->ecs_key == &voe_scene_parent_key)
		return NULL;
	if (described->ecs_key != &voe_scene_transform_key)
		return row;

	memcpy(origin, row, sizeof(*origin));
	origin->position = (voe_math_double3){ 0.0, 0.0, 0.0 };
	origin->rotation = (voe_math_quat){ 0.0f, 0.0f, 0.0f, 1.0f };
	origin->scale = (voe_math_float3){ 1.0f, 1.0f, 1.0f };
	return (const uint8_t *)origin;
}

// `kept` is this entity's kept sections, `kept_count` of them, ascending by key
// name.
static bool put_entity_block(voe_authoring_output *text,
			     const struct scene *scene,
			     const voe_authoring_authored *authored,
			     const voe_authoring_kept_section *kept,
			     uint32_t kept_count)
{
	const voe_ecs_world *world = scene->world;
	const voe_base_struct_description *identity_description =
		voe_ecs_component_description(world, scene->identity);
	voe_authoring_value_site site = {
		.world = world,
		.identity = scene->identity,
		.tree = scene->tree,
		.id = authored->id,
		.component = voe_ecs_component_key(world, scene->identity)->name,
	};
	const voe_scene_prefab_part *part =
		voe_scene_prefab_part_get(world, authored->entity);
	// A part row naming itself: a placed copy's root, saved as its root alone.
	bool copy_root = part != NULL &&
			 part->instance.index == authored->entity.index &&
			 part->instance.generation == authored->entity.generation;
	const uint8_t *row;
	voe_scene_transform origin;

	if (!voe_authoring_value_described_or_refuse(text, &site,
						     identity_description))
		return false;

	voe_authoring_put_string(text, "[");
	voe_authoring_put_unsigned(text, authored->id);
	voe_authoring_put_string(text, "]\n");

	row = voe_ecs_component_get(world, scene->identity, authored->entity);
	for (uint32_t f = 0; f < identity_description->field_count; f++) {
		const voe_base_field_description *field =
			&identity_description->fields[f];

		if (field->offset == offsetof(voe_scene_identity, id))
			continue;
		if (!voe_authoring_value_put_field(text, &site, field, row))
			return false;
	}

	uint32_t k = 0;

	for (uint32_t t = 0; t < scene->described_count; t++) {
		const struct described *described = &scene->described[t];

		if (copy_root && !described->on_copy_root)
			continue;
		row = row_written(scene, authored->entity, described, &origin);
		if (row == NULL)
			continue;

		for (; k < kept_count && strcmp(kept[k].key, described->key) < 0;
		     k++)
			if (!put_kept(text, scene, &kept[k]))
				return false;

		site.component = described->key;
		site.field[0] = '\0';
		if (!voe_authoring_value_described_or_refuse(
			    text, &site, described->description))
			return false;

		voe_authoring_put_string(text, "[");
		voe_authoring_put_unsigned(text, authored->id);
		voe_authoring_put_string(text, ".");
		voe_authoring_put_string(text, described->key);
		voe_authoring_put_string(text, "]\n");
		for (uint32_t f = 0; f < described->description->field_count;
		     f++)
			if (!voe_authoring_value_put_field(
				    text, &site,
				    &described->description->fields[f], row))
				return false;
	}
	for (; k < kept_count; k++)
		if (!put_kept(text, scene, &kept[k]))
			return false;
	return true;
}

static void drop_kept(const voe_authoring_output *text,
		      const voe_authoring_kept_section *kept)
{
	if (voe_authoring_measuring(text))
		VOE_BASE_WARNING(MODULE,
				 "the kept section [%" PRIu64 ".%s] belongs to "
				 "authored id %" PRIu64 ", which no entity in the "
				 "world has now; it is dropped",
				 kept->id, kept->key, kept->id);
}

static bool put_scene(voe_authoring_output *text, const struct scene *scene)
{
	uint32_t k = 0;
	bool written_one = false;

	for (uint32_t i = 0; i < scene->authored_count; i++) {
		const voe_authoring_authored *authored = &scene->authored[i];
		uint32_t end;

		for (; k < scene->kept_count && scene->kept[k].id < authored->id;
		     k++)
			drop_kept(text, &scene->kept[k]);
		end = k;
		while (end < scene->kept_count &&
		       scene->kept[end].id == authored->id)
			end++;

		if (voe_authoring_prefab_part_skipped(scene->world,
						      authored->entity)) {
			k = end;
			continue;
		}
		if (written_one)
			voe_authoring_put_string(text, "\n");
		written_one = true;
		if (!put_entity_block(text, scene, authored, scene->kept + k,
				      end - k))
			return false;
		k = end;
	}
	for (; k < scene->kept_count; k++)
		drop_kept(text, &scene->kept[k]);
	return true;
}

static void sort_by_key(struct described *items, uint32_t count)
{
	for (uint32_t i = 1; i < count; i++) {
		struct described item = items[i];
		uint32_t j = i;

		while (j > 0 && strcmp(items[j - 1].key, item.key) > 0) {
			items[j] = items[j - 1];
			j--;
		}
		items[j] = item;
	}
}

static bool kept_before(const voe_authoring_kept_section *a,
			const voe_authoring_kept_section *b)
{
	return a->id < b->id || (a->id == b->id && strcmp(a->key, b->key) < 0);
}

static void sort_kept(voe_authoring_kept_section *items, uint32_t count)
{
	for (uint32_t i = 1; i < count; i++) {
		voe_authoring_kept_section item = items[i];
		uint32_t j = i;

		while (j > 0 && kept_before(&item, &items[j - 1])) {
			items[j] = items[j - 1];
			j--;
		}
		items[j] = item;
	}
}

static void gather_kept(struct scene *scene, const voe_authoring_kept *kept,
			voe_base_arena *arena)
{
	if (kept == NULL || kept->count == 0)
		return;

	scene->kept = voe_base_arena_push(arena,
					  kept->count * sizeof(*scene->kept));
	memcpy(scene->kept, kept->sections, kept->count * sizeof(*scene->kept));
	scene->kept_count = kept->count;
	sort_kept(scene->kept, scene->kept_count);
}

static void gather(struct scene *scene, const voe_ecs_world *world,
		   const voe_authoring_kept *kept,
		   const voe_authoring_tree *tree, voe_base_arena *arena)
{
	uint32_t type_count = voe_ecs_component_type_count(world);

	*scene = (struct scene){ .world = world, .tree = tree };
	gather_kept(scene, kept, arena);
	if (type_count > 0)
		scene->described = voe_base_arena_push(
			arena, type_count * sizeof(*scene->described));

	for (uint32_t i = 0; i < type_count; i++) {
		voe_ecs_type type = voe_ecs_component_type_at(world, i);
		const struct voe_ecs_key *key = voe_ecs_component_key(world, type);

		if (key == &voe_scene_identity_key) {
			scene->has_identity = true;
			scene->identity = type;
			continue;
		}
		if (voe_ecs_component_runtime_only(world, type))
			continue;

		scene->described[scene->described_count++] = (struct described){
			.type = type,
			.ecs_key = key,
			.key = key->name,
			.description = voe_ecs_component_description(world, type),
			.on_copy_root = key == &voe_scene_transform_key ||
					key == &voe_scene_parent_key ||
					key == &voe_scene_prefab_key,
		};
	}
	sort_by_key(scene->described, scene->described_count);
	for (uint32_t i = 1; i < scene->described_count; i++)
		VOE_BASE_ASSERT(strcmp(scene->described[i - 1].key,
				       scene->described[i].key) != 0,
				"two component types registered under one key name");

	if (!scene->has_identity)
		return;

	uint32_t count = voe_ecs_component_count(world, scene->identity);

	if (count == 0)
		return;

	const voe_scene_identity *rows =
		voe_ecs_component_rows(world, scene->identity);
	const voe_ecs_entity *entities =
		voe_ecs_component_entities(world, scene->identity);
	voe_authoring_authored *scratch =
		voe_base_arena_push(arena, count * sizeof(*scratch));

	scene->authored = voe_base_arena_push(arena,
					      count * sizeof(*scene->authored));
	for (uint32_t i = 0; i < count; i++)
		if (tree == NULL || voe_authoring_tree_holds(tree, entities[i]))
			scene->authored[scene->authored_count++] =
				(voe_authoring_authored){
					.id = rows[i].id,
					.entity = entities[i],
				};
	voe_authoring_authored_sort(scene->authored, scratch,
				    scene->authored_count);
}

static bool ids_unique_or_refuse(const struct scene *scene)
{
	for (uint32_t i = 1; i < scene->authored_count; i++) {
		const voe_authoring_authored *a = &scene->authored[i - 1];
		const voe_authoring_authored *b = &scene->authored[i];

		if (a->id != b->id)
			continue;
		VOE_BASE_ERROR(MODULE,
			       "entities %uv%u and %uv%u both have authored id "
			       "%" PRIu64 ", and a scene file cannot tell them "
			       "apart; nothing was written",
			       a->entity.index, a->entity.generation,
			       b->entity.index, b->entity.generation, a->id);
		return false;
	}
	return true;
}

bool voe_authoring_scene_write(const voe_ecs_world *world,
			       const voe_authoring_kept *kept,
			       voe_base_arena *arena, voe_authoring_text *out)
{
	return voe_authoring_scene_write_tree(world, kept, NULL, arena, out);
}

bool voe_authoring_scene_write_tree(const voe_ecs_world *world,
				    const voe_authoring_kept *kept,
				    const voe_authoring_tree *tree,
				    voe_base_arena *arena,
				    voe_authoring_text *out)
{
	struct scene scene;
	voe_authoring_output measured = { 0 };
	voe_authoring_output written;

	VOE_BASE_ASSERT(world != NULL, "writing a NULL world");
	VOE_BASE_ASSERT(arena != NULL, "writing into a NULL arena");
	VOE_BASE_ASSERT(out != NULL, "nowhere to put the text");

	gather(&scene, world, kept, tree, arena);
	if (!ids_unique_or_refuse(&scene))
		return false;
	if (!put_scene(&measured, &scene))
		return false;

	written = (voe_authoring_output){
		.bytes = voe_base_arena_push(arena, measured.size + 1),
	};
	bool accepted = put_scene(&written, &scene);

	VOE_BASE_ASSERT(accepted && written.size == measured.size,
			"the writing pass disagreed with the measuring pass");
	written.bytes[written.size] = '\0';

	*out = (voe_authoring_text){
		.text = written.bytes,
		.size = written.size,
	};
	return true;
}
