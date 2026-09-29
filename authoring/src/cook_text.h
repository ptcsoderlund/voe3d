// The cook's text machinery, shared: how prefab_cook.c reuses scene_cook.c's
// spelling of a value rather than copying it (authoring/scene_cook.h says how
// every field kind is spelled and what it refuses).
//
// A COOK IS ONE STRUCT THE CALLER FILLS AND THE FUNCTIONS HERE GROW. The caller
// sets world, arena and the entity array's name, calls _collect, then writes
// the text around each _row; what differs between a scene and a prefab is the
// array an entity is named in and the order it sits there, `order`.
//
// Everything is pushed into the caller's arena, which it rewinds.
#pragma once

#include "authored.h"

#include <base/arena.h>
#include <base/describe.h>
#include <ecs/component.h>
#include <ecs/world.h>

#include <stddef.h>
#include <stdint.h>

typedef struct {
	const char *key;
	voe_ecs_type type;
	// NULL for a described type this build compiled out.
	const voe_base_struct_description *description;
} voe_authoring_cook_type;

typedef struct {
	const voe_ecs_world *world;
	voe_base_arena *arena;
	char *bytes;
	size_t size;
	size_t capacity;
	voe_ecs_type identity;
	voe_authoring_authored *authored;
	uint32_t authored_count;
	voe_authoring_cook_type *types;
	uint32_t type_count;
	// The cooked entity array's name; authored[i] is at `order[i]` in it, or at
	// i when `order` is NULL.
	const char *array;
	const uint32_t *order;
	// Where a refusal is, for its report.
	uint64_t id;
	const char *component;
	const char *field;
} voe_authoring_cook;

void voe_authoring_cook_put(voe_authoring_cook *c, const char *s);

[[gnu::format(printf, 2, 3)]]
void voe_authoring_cook_putf(voe_authoring_cook *c, const char *format, ...);

// Every described type that is not runtime-only, ascending by key name, and
// the identities ascending by id. False, reported, on two with one id.
[[nodiscard]] bool voe_authoring_cook_collect(voe_authoring_cook *c);

// `\tif (!<call>(world, voe_ecs_component_type(world, &<key>), <array>[j],
// &(<key>){ ... }))\n\t\treturn false;\n` for authored[index]'s row of `type`,
// nothing when it has none. False, reported, on a value that cannot be cooked.
[[nodiscard]] bool voe_authoring_cook_row(voe_authoring_cook *c,
					  const char *call, uint32_t index,
					  const voe_authoring_cook_type *type);
