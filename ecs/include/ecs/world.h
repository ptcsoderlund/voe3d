// The world: entity ids, and the registry every component table and intent
// queue hangs off. Nothing in this folder knows what a transform is, what a
// mesh is or what a camera is — a component type is a size and a capacity, an
// intent type is a size and a capacity, and the folder that owns the meaning
// is the folder that registers it.
//
//     voe_ecs_world *world = voe_ecs_world_new(arena, (voe_ecs_limits){
//             .entities = 4096, .component_types = 8, .intent_types = 8 });
//     voe_scene_transform_register(world, 4096);   // scene says what one is
//
//     voe_ecs_entity thing;
//     if (!voe_ecs_entity_create(world, &thing))
//             ...                                  // the world is full
//
// THE WORLD LIVES IN THE ARENA IT IS HANDED AND THERE IS NO voe_ecs_world_destroy.
// Every table, queue and per-entity array is pushed once at creation out of that
// arena, so the whole world is freed by rewinding or destroying it and never one
// allocation at a time (rule 11). That is also why there is no _destroy to
// forget to call: the arena is the lifetime, and _new is spelled that way
// because it hands the caller something it owns.
//
// CAPACITIES ARE FIXED AT CREATION AND RUNNING OUT IS A RETURNED FAILURE. An
// arena does not reallocate, and a bounded world is the deliberate shape rather
// than a limitation to be worked around later: it makes the memory a program
// needs a number a person chose, and it makes "too many" something the caller
// hears about at the call that asked for one too many. Nothing here grows.
//
// AN ENTITY ID IS AN INDEX AND A GENERATION, AND THE GENERATION IS THE WHOLE
// POINT. Slots are reused, so an id kept across a destroy would name a live slot
// holding something else. Destroying an entity bumps its slot's generation, so
// the old id no longer matches and every function here refuses it instead of
// addressing whatever moved in. A zeroed voe_ecs_entity is never a live one,
// because generation 0 is never handed out.
//
// A KEY IS AN ADDRESS AND NOT A NAME, WHICH IS WHY TWO FOLDERS CANNOT COLLIDE.
// Registration takes a pointer to a voe_ecs_key that the registering module
// defines once; the world compares pointers, so uniqueness is the linker's job
// and there is no central list of numbers for anyone to have to keep in step.
// The name inside it is for a message when something goes wrong, and for
// nothing else.
//
// SINGLE-THREADED, like the arena under it. Components being plain data with no
// pointers in them is what makes a later card able to read them from another
// thread; nothing here does, and nothing here locks.
#pragma once

#include <base/arena.h>

#include <stdint.h>

typedef struct voe_ecs_world voe_ecs_world;

// An entity. Copy it, store it, compare it — it is two numbers and it names
// nothing but a row in whatever tables hold one for it.
typedef struct {
	uint32_t index;
	uint32_t generation;
} voe_ecs_entity;

// What a world can hold, decided by whoever makes it. component_types and
// intent_types are how many _register calls the world will accept, not how many
// components an entity may have.
typedef struct {
	uint32_t entities;
	uint32_t component_types;
	uint32_t intent_types;
} voe_ecs_limits;

// The identity of a component type or an intent type. One per module, defined
// in the module that owns the meaning:
//
//     const struct voe_ecs_key voe_scene_transform_key = { "voe_scene_transform" };
//
// Its address is the key. Never copy one and register the copy.
struct voe_ecs_key {
	const char *name;
};

// Never NULL — the arena aborts rather than failing (rule 11). Every limit must
// be greater than zero; a zero is the caller's bug and asserts.
voe_ecs_world *voe_ecs_world_new(voe_base_arena *arena, voe_ecs_limits limits);

// False when the world already holds `limits.entities` of them. That is the one
// way this fails.
[[nodiscard]] bool voe_ecs_entity_create(voe_ecs_world *world,
					 voe_ecs_entity *out);

// Removes whatever components it had, then bumps its slot's generation so the id
// just used names nothing. Destroying an id that is already stale does nothing
// and is not an error: a caller holding two copies of an id should not have to
// know which one got there first.
void voe_ecs_entity_destroy(voe_ecs_world *world, voe_ecs_entity entity);

bool voe_ecs_entity_alive(const voe_ecs_world *world, voe_ecs_entity entity);
uint32_t voe_ecs_entity_count(const voe_ecs_world *world);
