// Keyboard Input: what the keys ask an entity to do this frame, as a
// direction. Teaches a component a project owns: the struct and its key share
// one name, the fields are listed once through VOE_BASE_DESCRIBE_STRUCT, and
// a register call hands both to game (game/project.h).
//
//     keyboard_input_register(world);           // in voe_game_project_register
//     keyboard_system_run(world, window);       // first, each frame
//
// `move` is x right and y forward, each -1 to 1. It is read-only: the
// Inspector shows it and does not edit it, because keyboard_system writes it
// every frame and anything typed there would be gone at once.
//
// Constraints: at most VOE_GAME_WORLD_AUTHORED rows.
#pragma once

#include <base/describe.h>

#include <ecs/world.h>

#include <math/float2.h>

#include <platform/window.h>

#include <stdbool.h>

#define KEYBOARD_INPUT_FIELDS(F, F_READ_ONLY) \
	F_READ_ONLY(voe_math_float2, move, FLOAT2)

VOE_BASE_DESCRIBE_STRUCT(keyboard_input, KEYBOARD_INPUT_FIELDS)

extern const struct voe_ecs_key keyboard_input_key;

// Registers the type under "Keyboard Input". False, reported, when game
// refuses it.
[[nodiscard]] bool keyboard_input_register(voe_ecs_world *world);

// Writes every keyboard_input row from W/A/S/D. A NULL window (headless)
// reads nothing and leaves the rows as they are.
void keyboard_system_run(voe_ecs_world *world, voe_platform_window *window);
