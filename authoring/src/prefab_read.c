// The prefab reader: scene_read.c's first pass (scene_scratch.h), a prefab's
// shape checked on the scratch, then its own second pass onto a placed root.
//
// THE SHAPE IS CHECKED BEFORE ANYTHING IS CREATED: exactly one `[N]` with no
// `[N.voe_scene_parent]`, and no camera, light, prefab or part section, whether
// or not this world registered the type. So a refused text creates nothing.
//
// THE ROOT IS ONLY ADDED TO. Its file rows go on as rows it lacks; its identity,
// transform and parent are the placed copy's own and are never taken.
//
// A KEPT SECTION IS DROPPED with one warning per key name, found by a naive scan
// of the ones before it: a prefab holds tens of sections, not thousands. A
// sorted list of names would lift that.
#include <authoring/prefab.h>

#include "authored.h"
#include "scene_scratch.h"

#include <base/assert.h>
#include <base/report.h>
#include <ecs/component.h>
#include <scene/camera_component.h>
#include <scene/identity_component.h>
#include <scene/light_component.h>
#include <scene/parent_component.h>
#include <scene/prefab_component.h>
#include <scene/transform_component.h>

#include <inttypes.h>
#include <string.h>

#define MODULE "authoring"

// The `<key>` of `[N.<key>]`, or NULL for `[N]`.
static const char *section_key(const voe_authoring_scratch *scratch, uint32_t s)
{
	VOE_BASE_ASSERT(s < scratch->doc.section_count, "a section past the end");

	const char *dot = strchr(scratch->doc.sections[s].name, '.');

	return dot != NULL ? dot + 1 : NULL;
}

static uint32_t authored_index(voe_authoring_scratch *scratch, uint64_t id)
{
	const voe_authoring_authored *found = voe_authoring_authored_find(
		scratch->refs.authored, scratch->refs.authored_count, id);

	VOE_BASE_ASSERT(found != NULL, "a section's [N] missing from the map");
	return (uint32_t)(found - scratch->refs.authored);
}

// No forbidden section, and exactly one `[N]` with no parent section, whose
// index in the authored list goes to `*out_root`.
static bool check_shape(voe_authoring_scratch *scratch, uint32_t *out_root)
{
	static const struct voe_ecs_key *const forbidden[] = {
		&voe_scene_camera_key, &voe_scene_light_key,
		&voe_scene_prefab_key, &voe_scene_prefab_part_key,
	};
	uint32_t count = scratch->refs.authored_count;
	bool *parented = voe_base_arena_push(scratch->arena,
					     (count + 1) * sizeof(*parented));
	uint32_t roots = 0;

	memset(parented, 0, (count + 1) * sizeof(*parented));
	for (uint32_t s = 0; s < scratch->doc.section_count; s++) {
		const char *key = section_key(scratch, s);

		if (key == NULL)
			continue;
		if (strcmp(key, voe_scene_parent_key.name) == 0)
			parented[authored_index(scratch, scratch->sections[s].id)] =
				true;
		for (uint32_t f = 0; f < sizeof(forbidden) / sizeof(*forbidden);
		     f++) {
			if (strcmp(key, forbidden[f]->name) != 0)
				continue;
			VOE_BASE_ERROR(MODULE,
				       "line %u: [%s] is a %s, which a prefab "
				       "cannot hold; nothing was loaded",
				       scratch->doc.sections[s].line,
				       scratch->doc.sections[s].name, key);
			return false;
		}
	}
	for (uint32_t i = 0; i < count; i++) {
		if (!parented[i]) {
			roots++;
			*out_root = i;
		}
	}
	if (roots == 1)
		return true;
	VOE_BASE_ERROR(MODULE,
		       "the prefab has %u entities with no parent, and a prefab "
		       "is one tree with exactly one root; nothing was loaded",
		       roots);
	return false;
}

// Every entity but the root's, made in ascending file id; then every held
// reference written through.
static bool create_entities(voe_authoring_scratch *scratch, uint32_t root_index,
			    voe_ecs_entity root)
{
	for (uint32_t i = 0; i < scratch->refs.authored_count; i++) {
		voe_authoring_authored *authored = &scratch->refs.authored[i];

		if (i == root_index) {
			authored->entity = root;
			continue;
		}
		if (!voe_ecs_entity_create(scratch->world, &authored->entity)) {
			VOE_BASE_ERROR(MODULE,
				       "the world is full at the prefab's [%" PRIu64
				       "]; it holds part of the prefab",
				       authored->id);
			return false;
		}
	}
	for (uint32_t p = 0; p < scratch->refs.patch_count; p++) {
		const voe_authoring_patch *patch = &scratch->refs.patches[p];
		const voe_authoring_authored *authored =
			&scratch->refs.authored[authored_index(scratch, patch->id)];

		memcpy(patch->bytes, &authored->entity, sizeof(authored->entity));
	}
	return true;
}

// Whether section `s` goes on the root: not its identity, transform or parent,
// and not a row it already has.
static bool root_takes(const voe_authoring_scratch *scratch, uint32_t s,
		       voe_ecs_entity root)
{
	const voe_authoring_section *section = &scratch->sections[s];
	const char *key = section_key(scratch, s);

	if (key == NULL || strcmp(key, voe_scene_transform_key.name) == 0 ||
	    strcmp(key, voe_scene_parent_key.name) == 0)
		return false;
	return voe_ecs_component_get(scratch->world, section->type, root) == NULL;
}

// The identities first, then every other row, each in file order; an identity
// takes `first_id` plus its rank among the made entities.
static bool add_rows(voe_authoring_scratch *scratch, uint32_t root_index,
		     uint64_t first_id)
{
	voe_ecs_entity root = scratch->refs.authored[root_index].entity;

	for (uint32_t pass = 0; pass < 2; pass++) {
		for (uint32_t s = 0; s < scratch->doc.section_count; s++) {
			const voe_authoring_section *section = &scratch->sections[s];
			bool entity =
				section->role == VOE_AUTHORING_SECTION_ENTITY;
			uint32_t i = authored_index(scratch, section->id);
			const void *row = section->row;
			voe_scene_identity identity;

			if (section->role == VOE_AUTHORING_SECTION_KEPT ||
			    entity != (pass == 0))
				continue;
			if (i == root_index) {
				if (!root_takes(scratch, s, root))
					continue;
			} else if (entity) {
				memcpy(&identity, section->row, sizeof(identity));
				identity.id = first_id + i - (i > root_index);
				row = &identity;
			}
			if (voe_ecs_component_add(scratch->world, section->type,
						  scratch->refs.authored[i].entity,
						  row))
				continue;
			VOE_BASE_ERROR(MODULE,
				       "line %u: the world has no room for [%s]; "
				       "it holds part of the prefab",
				       scratch->doc.sections[s].line,
				       scratch->doc.sections[s].name);
			return false;
		}
	}
	return true;
}

// A part row naming the root on every entity made and on the root, unless the
// root already has one.
static bool add_parts(voe_authoring_scratch *scratch, voe_ecs_entity root)
{
	voe_ecs_type part =
		voe_ecs_component_type(scratch->world, &voe_scene_prefab_part_key);
	const voe_scene_prefab_part row = { .instance = root };

	for (uint32_t i = 0; i < scratch->refs.authored_count; i++) {
		voe_ecs_entity entity = scratch->refs.authored[i].entity;

		if (voe_ecs_component_get(scratch->world, part, entity) != NULL)
			continue;
		if (voe_ecs_component_add(scratch->world, part, entity, &row))
			continue;
		VOE_BASE_ERROR(MODULE,
			       "the world has no room for a prefab part row; "
			       "it holds part of the prefab");
		return false;
	}
	return true;
}

static void warn_kept(const voe_authoring_scratch *scratch)
{
	for (uint32_t k = 0; k < scratch->kept_count; k++) {
		const char *key = scratch->kept[k].key;
		uint32_t before = 0;

		while (before < k && strcmp(scratch->kept[before].key, key) != 0)
			before++;
		if (before == k)
			VOE_BASE_WARNING(MODULE,
					 "the prefab holds %s, which nothing "
					 "registered; its sections are skipped",
					 key);
	}
}

bool voe_authoring_prefab_read(const char *text, size_t size,
			       voe_ecs_world *world, voe_ecs_entity root,
			       uint64_t first_id, voe_base_arena *arena,
			       uint64_t *out_next_id)
{
	voe_authoring_scratch scratch = { .world = world, .arena = arena };
	uint32_t root_index = 0;

	VOE_BASE_ASSERT(world != NULL && arena != NULL,
			"reading a prefab with no world or arena");
	VOE_BASE_ASSERT(out_next_id != NULL, "nowhere to put the next id");
	VOE_BASE_ASSERT(voe_ecs_entity_alive(world, root) &&
				voe_scene_transform_get(world, root) != NULL,
			"reading a prefab onto a root that is dead or has no "
			"transform");

	if (!voe_authoring_scratch_read(&scratch, text, size) ||
	    !check_shape(&scratch, &root_index))
		return false;
	warn_kept(&scratch);
	if (!create_entities(&scratch, root_index, root) ||
	    !add_rows(&scratch, root_index, first_id) ||
	    !add_parts(&scratch, root))
		return false;

	*out_next_id = first_id + scratch.refs.authored_count - 1;
	return true;
}
