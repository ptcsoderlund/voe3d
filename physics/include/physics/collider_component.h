// What an entity collides as: a box, a sphere or a capsule, and whether it is a
// trigger. Read by anyone, const; written only through
// physics/collider_system.h.
//
// SIZE IS IN THE ENTITY'S OWN UNITS AND ITS TRANSFORM'S SCALE APPLIES (0253
// point 2). A box is `size` along its local axes. A sphere's diameter is
// `size.x`. A capsule stands on local Y: its diameter is `size.x` and its whole
// height, caps included, `size.y`. What that becomes in the world is
// physics/shape.h's to say, and the collider sits at its transform's position.
//
// IT IS PLAIN COMPONENT DATA (0249 rule 4), so any reader — a query, a debug
// draw, a later particle upload — reads the same numbers, and no hidden cache
// stands beside the row.
//
// A TRIGGER BLOCKS NOTHING. A query still finds it, marked as a trigger, and a
// body passes through it; a project notices a trigger by asking what overlaps
// its shape (0253 point 5).
//
// KIND IS CHOSEN FROM ITS THREE NAMES, indexed by the kind's own number, which
// is why entry nought is NULL: nought is no kind. A kind this build does not
// know is kept and warned about by the drain, not refused, because a file may be
// newer than the build; such a collider has no shape and collides as nothing.
//
// THE DEFAULT ROW IS A BOX OF 1, NOT A TRIGGER, and a collider needs a
// transform, so transforms are registered first.
#pragma once

#include <base/describe.h>
#include <base/imported.h>

#include <ecs/component.h>
#include <ecs/world.h>

#include <math/float3.h>

#include <stdint.h>

// The kinds defined so far. A future kind is a new constant here, never a
// reuse of one of these.
#define VOE_PHYSICS_COLLIDER_BOX 1u
#define VOE_PHYSICS_COLLIDER_SPHERE 2u
#define VOE_PHYSICS_COLLIDER_CAPSULE 3u

// The name of each kind, indexed by the kind's own number; NULL for nought.
extern VOE_BASE_IMPORTED const char *const voe_physics_collider_kind_names[4];

// kind is 4 bytes like the plain enum ENUM describes; size is never below 0.
#define VOE_PHYSICS_COLLIDER_FIELDS(F, F_READ_ONLY) \
	F(uint32_t, kind, ENUM)                     \
	F(voe_math_float3, size, FLOAT3)            \
	F(bool, trigger, BOOL)

#define VOE_PHYSICS_COLLIDER_NAMES(N) \
	N(kind, voe_physics_collider_kind_names)

VOE_BASE_DESCRIBE_STRUCT_NAMED(voe_physics_collider,
			       VOE_PHYSICS_COLLIDER_FIELDS,
			       VOE_PHYSICS_COLLIDER_NAMES)

// The key this component is registered against. Its address is its identity.
extern VOE_BASE_IMPORTED const struct voe_ecs_key voe_physics_collider_key;

// NULL when the entity has no collider or is not alive. The pointer is into the
// table and is good until the next add or remove.
const voe_physics_collider *
voe_physics_collider_get(const voe_ecs_world *world, voe_ecs_entity entity);

// The table, for a query. rows[i] belongs to entities[i], both `count` long.
uint32_t voe_physics_collider_count(const voe_ecs_world *world);
const voe_physics_collider *
voe_physics_collider_rows(const voe_ecs_world *world);
const voe_ecs_entity *voe_physics_collider_entities(const voe_ecs_world *world);
