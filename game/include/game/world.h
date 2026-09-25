// The one list of component types a project's world holds, and the room it
// has for each: transform, identity, light, camera, mesh, material, panel and
// shape, registered once on a fresh world.
//
//     voe_ecs_world *world = voe_game_world_new(arena);
//
// THE EDITOR AND THE GAME BOTH BUILD THEIR WORLD HERE (0237), so the cook,
// which names what the editor's world holds, can never name a type the game
// did not register, and the two can never have different room for the same
// thing.
//
// THE ROOM FOR A PROJECT'S TYPES: past the engine's VOE_GAME_WORLD_TYPES the
// world takes VOE_GAME_PROJECT_TYPES more types and as many more intents,
// registered through game/project.h.
//
// Constraints: the world lives in `arena` and has no destroy, as every ecs
// world (ecs/world.h). The capacities are fixed numbers; a project that
// authors more than VOE_GAME_WORLD_AUTHORED entities or draws more than
// VOE_GAME_WORLD_MAX_DRAWN is refused at the add, and raising either number
// here is what lifts it, for the editor and the game at once.
#pragma once

#include <base/arena.h>

#include <ecs/world.h>

// How many entities may wear a mesh and a material, which is also the room
// for shapes: every shape the shape system finds becomes one of each. A
// device that draws the world is sized from the same number.
#define VOE_GAME_WORLD_MAX_DRAWN 64

// How many authored entities a world holds: its identities, and its lights,
// which are only ever on an authored entity. The editor's Scene panel lists
// exactly this many rows.
#define VOE_GAME_WORLD_AUTHORED 32

// How many types the engine registers here; a project's come after them.
#define VOE_GAME_WORLD_TYPES 8

// A fresh world with the eight types registered and nothing in it. Never NULL:
// the arena aborts rather than failing.
voe_ecs_world *voe_game_world_new(voe_base_arena *arena);
