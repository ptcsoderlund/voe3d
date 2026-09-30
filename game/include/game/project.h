// The seam a project's own code is written against (0239, 0242): its component
// types registered through game, and the four entry points the project defines.
//
//     const struct voe_ecs_key player_key = { "player" };
//     void voe_game_project_register(voe_ecs_world *world)
//     {
//             voe_game_project_component(world, &(voe_game_project_type){
//                     &player_key, sizeof(player), 1,
//                     VOE_GAME_PROJECT_DESCRIPTION(player), NULL, "Game / Player",
//                     &voe_scene_transform_key });
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
// (game/run.h registers; game/steps.h runs the systems each fixed step); the
// editor resolves register with one lookup in the loaded library and never
// runs the systems (ADR-0008).
//
// TWO SLOTS A STEP (0256): systems_run is before the bodies' move;
// systems_after_move is after it, in the same step, so what it moves is
// drawn at the same lag (a camera following a body belongs there). Within a
// slot the project calls its systems in list order, and that is the only
// ordering. A project with no after-the-move systems defines it empty.
//
// THE INTERFACE RUNS ONCE A FRAME, AFTER THE STEPS (0259), because a click is
// per frame and a step runs zero to four times. Game begins the ui frame and
// sets the pointer; the project lays out, calls voe_ui_frame_end itself and
// reads its buttons. False ends the run. A project with no interface ends the
// frame and returns true.
//
// SPAWN AND REMOVE LAND AT THE STEP'S STRUCTURAL APPLY: the entities exist at
// once, their rows and a removal only then. They live in project.c because
// the editor links it, so a loaded project library binds them (0283 point
// 10); the table comes in the step, NULL in the editor and in tests.
//
// Constraints: at most VOE_GAME_PROJECT_TYPES types, each row at most
// VOE_GAME_PROJECT_ROW bytes so its intent fits the Inspector's 256; each
// replace queue holds VOE_GAME_WORLD_AUTHORED values a frame. The world is one
// voe_game_world_new made, with nothing registered past the engine's types
// but through here. A removed tree is at most a world's 1024 parents; a full
// queue may leave part of it queued.
#pragma once

#include <audio/mixer.h>

#include <base/describe.h>

#include <ecs/component.h>
#include <ecs/world.h>

#include <game/prefabs.h>

#include <math/float2.h>

#include <platform/window.h>

#include <ui/layout.h>

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

// What one fixed step hands the project's systems. `window` is NULL headless;
// `audio` is the game's mixer, where a system plays a sound (0266 point 4),
// NULL only in a test whose systems play nothing; `prefabs` is the table
// spawns come from, NULL when there is nothing to spawn.
typedef struct {
	voe_ecs_world *world;
	voe_platform_window *window;
	voe_audio_mixer *audio;
	const voe_game_prefabs *prefabs;
	double seconds;
} voe_game_project_step;

// What one frame hands the project's interface. `window` is NULL headless;
// `ui`'s frame is begun with the pointer set; `size` is the surface in
// millimetres, the root's size.
typedef struct {
	voe_ecs_world *world;
	voe_platform_window *window;
	voe_ui_context *ui;
	voe_math_float2 size;
} voe_game_project_frame;

// One project type. `default_row` NULL is zeros; `menu` NULL is no menu path.
// `needs` is the key of a type a row of this one does nothing without, such as
// voe_scene_transform_key, told to ecs so Add component brings it and the
// Inspector keeps it (0302); NULL for none. It must be an engine type or a
// project type registered before this one.
typedef struct {
	const struct voe_ecs_key *key;
	size_t size;
	uint32_t capacity;
	const voe_base_struct_description *description;
	const void *default_row;
	const char *menu;
	const struct voe_ecs_key *needs;
} voe_game_project_type;

// Registers the table, its replace intent, default and menu. False, reported,
// when refused as the header says.
[[nodiscard]] bool voe_game_project_component(voe_ecs_world *world,
					      const voe_game_project_type *type);

// Drains every project replace queue: a live entity with the row gets it
// whole, any other value is dropped silently, and each queue ends empty.
void voe_game_project_replaces_apply(voe_ecs_world *world);

// Makes the prefab `name`'s entities and queues its rows, the root at
// `position` and `rotation`, into `out_root`. False with nothing made when
// the world or queue is full; an unknown name or no table is also a line on
// stderr.
[[nodiscard]] bool voe_game_project_spawn(const voe_game_project_step *step,
					  const char *name,
					  voe_math_double3 position,
					  voe_math_quat rotation,
					  voe_ecs_entity *out_root);

// Queues destroying `entity` and everything under it. False when the queue
// is full.
[[nodiscard]] bool voe_game_project_remove(const voe_game_project_step *step,
					   voe_ecs_entity entity);

// Defined by the project, never by the engine.
void voe_game_project_register(voe_ecs_world *world);
void voe_game_project_systems_run(const voe_game_project_step *step);
void voe_game_project_systems_after_move(const voe_game_project_step *step);
bool voe_game_project_interface(const voe_game_project_frame *frame);
