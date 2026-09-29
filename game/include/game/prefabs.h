// The cooked prefabs a game spawns from (0283 point 9): each one's name, how
// many entities it makes, and the function that queues its rows onto them.
//
//     static bool shell(voe_ecs_world *world, const voe_ecs_entity *entities,
//                       voe_math_double3 position, voe_math_quat rotation)
//     { ... voe_ecs_structure_add(world, ..., entities[0], ...) ... }
//     const voe_game_prefabs voe_game_prefabs_cooked = {
//             (const voe_game_prefab[]){ { "Tank/Shell", 2, shell } }, 1 };
//
// DEFINED BY THE COOKED prefabs.c IN A PROJECT'S GAME TREE, NOT BY THIS
// FOLDER, as game/scene.h's function is by the cooked scene.c. This is the
// one include that file needs: it brings game/scene.h's types and the
// structural queue. Only game/src/run.c names the table; everything else
// takes it as data in the step (game/project.h).
//
// A NAME IS THE PREFAB'S PATH UNDER Assets/ LESS .prefab, with `/`.
//
// Constraints: a prefab makes at most VOE_GAME_PREFAB_ENTITIES entities.
// `entities[0]` is the root; a build queues through the structural queue
// only, and false is a full queue.
#pragma once

#include <game/scene.h>

#include <ecs/structure.h>

#include <math/double3.h>
#include <math/quat.h>

#include <stdbool.h>
#include <stdint.h>

// The most entities one prefab makes.
#define VOE_GAME_PREFAB_ENTITIES 32

// Queues one prefab's rows onto `entities`, the root at `position` and
// `rotation`. False when the queue had no room.
typedef bool voe_game_prefab_build(voe_ecs_world *world,
				   const voe_ecs_entity *entities,
				   voe_math_double3 position,
				   voe_math_quat rotation);

// One cooked prefab.
typedef struct {
	const char *name;
	uint32_t entities;
	voe_game_prefab_build *build;
} voe_game_prefab;

// The table a game spawns from.
typedef struct {
	const voe_game_prefab *prefabs;
	uint32_t count;
} voe_game_prefabs;

// Defined by the game tree's cooked prefabs.c.
extern const voe_game_prefabs voe_game_prefabs_cooked;
