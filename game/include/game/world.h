// The one list of component types a project's world holds, and the room it
// has for each: transform, identity, parent, prefab, prefab part, light, camera,
// mesh, material, panel, shape, model, collider, body, emitter, its particles,
// the transforms' previous step (0254), sound and its voice row (0304), water
// and its waves (0305), point light (0320, 0321), registered once on a
// fresh world.
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

#include <render/device.h>

// How many entities may wear a mesh and a material, which is also the room
// for shapes and models: every shape the shape system finds becomes one of each. A
// device that draws the world is sized from the same number. Colliders have
// this room too, because spawned things collide (0293); bodies keep the
// authored room.
#define VOE_GAME_WORLD_MAX_DRAWN 256

// How many authored entities a world holds: its identities, and its lights,
// which are only ever on an authored entity, and its prefab rows, which are
// only on an authored root and its parts. The editor's Scene panel lists
// exactly this many rows. It stays small while the world grows because a thing
// game code spawns carries no identity (0283 point 11). It is 128 to hold a
// level several times the tank game's, which filled 32, while staying half
// the drawn room so a full level still leaves draws for spawned things (0337).
#define VOE_GAME_WORLD_AUTHORED 128

// How many emitters, and so particles rows, a world holds: the room of drawn
// things, because an effect sits on a drawn thing and a spawned wreck keeps
// its own (0298 point 8).
#define VOE_GAME_WORLD_EMITTERS VOE_GAME_WORLD_MAX_DRAWN

// How many sounds, and so voice rows, a world holds: the room of drawn things,
// because a sound sits on a thing and spawned ones carry theirs (0304).
#define VOE_GAME_WORLD_SOUNDS VOE_GAME_WORLD_MAX_DRAWN

// How many waters, and so waves rows, a world holds: a scene's few lakes and
// seas, and room for a spawned thing to carry its own, as an emitter's
// reasoning goes (0298 point 8), without the room of every drawn thing, since
// each is one large plane (0305).
#define VOE_GAME_WORLD_WATERS 16

// How many point lights a world holds (0320 point 9). No
// more than a pass carries: past VOE_RENDER_POINT_LIGHTS the rest light
// nothing, so room for them would be room for lamps that never shine.
#define VOE_GAME_WORLD_POINT_LIGHTS 256
static_assert(VOE_GAME_WORLD_POINT_LIGHTS <= VOE_RENDER_POINT_LIGHTS,
	      "a world holds more point lights than a pass carries");

// How many types the engine registers here; a project's come after them.
#define VOE_GAME_WORLD_TYPES 22

// A fresh world with the twenty-two types registered and nothing in it. Never NULL:
// the arena aborts rather than failing.
voe_ecs_world *voe_game_world_new(voe_base_arena *arena);
