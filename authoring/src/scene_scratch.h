// Scene text read into scratch and validated whole, touching no world: the
// first pass that scene_read.c and prefab_read.c share, defined in scene_read.c.
//
//     voe_authoring_scratch scratch = { .world = world, .arena = arena };
//
//     if (!voe_authoring_scratch_read(&scratch, text, size))
//             return false;           // reported; the world is untouched
//     // create the entities, fill scratch.refs.authored[].entity, patch, add
//
// EVERY REFUSAL IS HERE, and reported with its line: a section name, a value,
// an `[N]` missing for a component, an identity or runtime-only section. What
// the caller refuses on top (a prefab's shape) it checks on the scratch before
// it creates anything.
//
// A ROW IS FILLED BUT NOTHING IS CREATED. Every described section's row is in
// the arena, a missing field taken from the type's default row; an entity
// reference is held as a patch naming an authored id until the caller's second
// pass writes the entity it made through it. A section naming an unregistered
// type is copied out into `kept` whatever the caller does with it.
//
// The world is only read, and the arena holds everything pushed.
#pragma once

#include "field_read.h"
#include "key_span.h"

#include <authoring/scene_read.h>

#include <assets/sectioned.h>
#include <base/arena.h>
#include <base/describe.h>
#include <ecs/component.h>
#include <ecs/world.h>

#include <stddef.h>
#include <stdint.h>

// What one section is, decided before any value is read.
enum voe_authoring_section_role {
	VOE_AUTHORING_SECTION_ENTITY,	 // `[N]`
	VOE_AUTHORING_SECTION_COMPONENT, // `[N.<key>]`, a described type
	VOE_AUTHORING_SECTION_KEPT,	 // `[N.<key>]`, a name nothing registered
};

typedef struct {
	enum voe_authoring_section_role role;
	uint64_t id;
	voe_ecs_type type;
	const voe_base_struct_description *description;
	// The scratch row; NULL for a kept section.
	uint8_t *row;
} voe_authoring_section;

typedef struct {
	voe_ecs_world *world;
	voe_base_arena *arena;
	voe_assets_sectioned doc;

	// One per key, as the sectioned reader numbers them.
	voe_authoring_span *key_span;

	bool has_identity;
	voe_ecs_type identity;

	// One per section of `doc`, in file order.
	voe_authoring_section *sections;

	// One `[N]` each, ascending by id, their entities the caller's to fill;
	// and the references held.
	voe_authoring_field_refs refs;

	voe_authoring_kept_section *kept;
	uint32_t kept_count;
} voe_authoring_scratch;

// Reads `size` bytes of scene text into `scratch`, whose world and arena are
// set and the rest zero. False, reported, when the text is refused.
[[nodiscard]] bool voe_authoring_scratch_read(voe_authoring_scratch *scratch,
					      const char *text, size_t size);
