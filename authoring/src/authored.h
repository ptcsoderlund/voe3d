// Authored entities by id: the table both directions of a scene sort and search.
//
// The writer sorts the world's identities to write them in order; the reader
// sorts the file's `[N]` sections so that `[N.<key>]` and an entity reference can
// find their entity without a walk per lookup, which on a scene of thousands is
// the difference between a load and a wait.
//
// A MERGE SORT AND A BINARY SEARCH, AND NEITHER IS qsort OR bsearch, whose
// comparators are function pointers — and function pointers in this engine are
// render's loader table alone.
#pragma once

#include <ecs/world.h>

#include <stdint.h>

typedef struct {
	uint64_t id;
	voe_ecs_entity entity;
} voe_authoring_authored;

// Ascending by id, and stable. `scratch` is as long as `items` and holds nothing
// on the way in or out.
void voe_authoring_authored_sort(voe_authoring_authored *items,
				 voe_authoring_authored *scratch,
				 uint32_t count);

// The item with this id in `items`, which must already be sorted, or NULL. With
// the id more than once, any one of them.
voe_authoring_authored *voe_authoring_authored_find(voe_authoring_authored *items,
						    uint32_t count, uint64_t id);
