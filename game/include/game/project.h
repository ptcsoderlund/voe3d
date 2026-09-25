// The seam a project's own code is written against (0239, 0242): its component
// types registered through game, and the two entry points the project defines.
//
//     const struct voe_ecs_key player_key = { "player" };
//     void voe_game_project_register(voe_ecs_world *world)
//     {
//             voe_game_project_component(world, &(voe_game_project_type){
//                     &player_key, sizeof(player), 1,
//                     VOE_GAME_PROJECT_DESCRIPTION(player), NULL, "Game / Player" });
//     }
//
// KEY AND STRUCT SHARE ONE NAME: the struct `player`, its key `player_key`
// named "player", because the cook names both from the key.
//
// A REFUSAL IS REPORTED, NOT ASSERTED: a row over VOE_GAME_PROJECT_ROW, a
// type past VOE_GAME_PROJECT_TYPES or a zero size is a line on stderr and
// false, because a project's code is not the engine's to assert on.
//
// GAME IS THE ONE WRITER OF A PROJECT ROW FROM OUTSIDE. Each type gets a
// replace intent of game's own (the entity at 0, the row at the next
// max_align_t boundary), applied whole by voe_game_project_replaces_apply
// after the structural queue. A project system writes its own rows directly
// and other folders' through their intents (rule 4).
//
// THE ENTRY POINTS ARE THE PROJECT'S, never the engine's. The game links them
// (game/run.h registers, then runs the systems before each frame); the editor
// resolves register with one lookup in the loaded library and never runs the
// systems (ADR-0008).
//
// Constraints: at most VOE_GAME_PROJECT_TYPES types, each row at most
// VOE_GAME_PROJECT_ROW bytes so its intent fits the Inspector's 256; each
// replace queue holds VOE_GAME_WORLD_AUTHORED values a frame. The world is one
// voe_game_world_new made, with nothing registered past the engine's types
// but through here.
#pragma once

#include <base/describe.h>

#include <ecs/component.h>
#include <ecs/world.h>

#include <platform/window.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define VOE_GAME_PROJECT_TYPES 32
#define VOE_GAME_PROJECT_ROW 240

// The description of the struct `name`, or the compiled-out marker in a
// build without descriptions (ecs/component.h).
#if defined(VOE_BASE_DESCRIPTIONS) && VOE_BASE_DESCRIPTIONS
#define VOE_GAME_PROJECT_DESCRIPTION(name) name##_description()
#else
#define VOE_GAME_PROJECT_DESCRIPTION(name) (&voe_ecs_description_compiled_out)
#endif

// What one frame hands the project's systems. `window` is NULL headless.
typedef struct {
	voe_ecs_world *world;
	voe_platform_window *window;
	double seconds;
} voe_game_project_step;

// One project type. `default_row` NULL is zeros; `menu` NULL is no menu path.
typedef struct {
	const struct voe_ecs_key *key;
	size_t size;
	uint32_t capacity;
	const voe_base_struct_description *description;
	const void *default_row;
	const char *menu;
} voe_game_project_type;

// Registers the table, its replace intent, default and menu. False, reported,
// when refused as the header says.
[[nodiscard]] bool voe_game_project_component(voe_ecs_world *world,
					      const voe_game_project_type *type);

// Drains every project replace queue: a live entity with the row gets it
// whole, any other value is dropped silently, and each queue ends empty.
void voe_game_project_replaces_apply(voe_ecs_world *world);

// Defined by the project, never by the engine.
void voe_game_project_register(voe_ecs_world *world);
void voe_game_project_systems_run(const voe_game_project_step *step);
