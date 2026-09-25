// Keyboard Input: what the keys ask an entity to do this frame, as a
// direction. Teaches a component a project owns: the struct and its key share
// one name, the fields are listed once through VOE_BASE_DESCRIBE_STRUCT, and
// a register call hands both to game (game/project.h).
//
//     keyboard_input_register(world);           // in voe_game_project_register
//     keyboard_system_run(world, window);       // first, each fixed step
//
// `move` is x right and y forward, each -1 to 1. `jump` is true in the one
// step Space went down in; `jump_down` is Space's level at the last step, the
// level that edge is found against. All three are read-only: the Inspector
// shows them and does not edit them, because keyboard_system writes them
// every step and anything typed there would be gone at once.
//
// Constraints: at most VOE_GAME_WORLD_AUTHORED rows. A press and release
// both inside one frame that runs no step is not seen.
#pragma once

#include <base/describe.h>

#include <ecs/world.h>

#include <math/float2.h>

#include <platform/window.h>

#include <stdbool.h>

#define KEYBOARD_INPUT_FIELDS(F, F_READ_ONLY) \
	F_READ_ONLY(voe_math_float2, move, FLOAT2)    \
	F_READ_ONLY(bool, jump, BOOL)                 \
	F_READ_ONLY(bool, jump_down, BOOL)

VOE_BASE_DESCRIBE_STRUCT(keyboard_input, KEYBOARD_INPUT_FIELDS)

extern const struct voe_ecs_key keyboard_input_key;

// Registers the type under "Keyboard Input". False, reported, when game
// refuses it.
[[nodiscard]] bool keyboard_input_register(voe_ecs_world *world);

// Writes every keyboard_input row from W/A/S/D and Space. A NULL window (headless)
// reads nothing and leaves the rows as they are.
void keyboard_system_run(voe_ecs_world *world, voe_platform_window *window);
