// The keyboard system: raw keys into keyboard_input rows. Teaches the one
// thing a project system may do directly, write its own component's rows,
// and that input is raw (0239): the system asks whether a key is down and
// makes of it what the game needs.
//
// Constraints: W/A/S/D only, by place on the keyboard; opposite keys cancel.
#include "keyboard_input.h"

#include <base/assert.h>

#include <ecs/component.h>

#include <game/project.h>
#include <game/world.h>

#include <platform/input.h>

const struct voe_ecs_key keyboard_input_key = { "keyboard_input" };

bool keyboard_input_register(voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "registering keyboard input in no world");
	return voe_game_project_component(world, &(voe_game_project_type){
		&keyboard_input_key, sizeof(keyboard_input),
		VOE_GAME_WORLD_AUTHORED,
		VOE_GAME_PROJECT_DESCRIPTION(keyboard_input), NULL,
		"Keyboard Input" });
}

// -1, 0 or 1 from a pair of opposite keys.
static float key_axis(voe_platform_window *window, voe_platform_key minus,
		      voe_platform_key plus)
{
	return (voe_platform_input_key_down(window, plus) ? 1.0f : 0.0f) -
	       (voe_platform_input_key_down(window, minus) ? 1.0f : 0.0f);
}

void keyboard_system_run(voe_ecs_world *world, voe_platform_window *window)
{
	VOE_BASE_ASSERT(world != NULL, "reading the keyboard into no world");
	if (window == NULL)
		return;
	const voe_ecs_type type =
		voe_ecs_component_type(world, &keyboard_input_key);
	const keyboard_input row = { .move = {
		key_axis(window, VOE_PLATFORM_KEY_A, VOE_PLATFORM_KEY_D),
		key_axis(window, VOE_PLATFORM_KEY_S, VOE_PLATFORM_KEY_W) } };
	const voe_ecs_entity *entities = voe_ecs_component_entities(world, type);
	const uint32_t count = voe_ecs_component_count(world, type);

	VOE_BASE_ASSERT(count <= VOE_GAME_WORLD_AUTHORED,
			"more keyboard input rows than were registered");
	for (uint32_t i = 0; i < count; i++) {
		const bool set = voe_ecs_component_set(world, type, entities[i],
						       &row);

		VOE_BASE_ASSERT(set, "a listed keyboard input row is gone");
	}
}
