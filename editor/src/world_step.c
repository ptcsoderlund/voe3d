// The world's step once a frame, game's step and nothing else (world_step.h).
//
// Constraints: every call runs every system, unconditionally, and no move.
#include "world_step.h"

#include <game/frame.h>

void voe_editor_world_step(voe_ecs_world *world, const voe_3d_shapes *shapes)
{
	voe_game_world_step(world, shapes);
}
