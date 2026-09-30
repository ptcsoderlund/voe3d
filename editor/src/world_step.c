// The world's step once a frame: game's step, then the emitters run by the
// frame's seconds, then every placed copy queued since expanded (world_step.h).
//
// Constraints: every call runs every system, unconditionally, and no move.
#include "world_step.h"

#include "prefabs.h"

#include <3d/emitter_system.h>

#include <base/assert.h>

#include <game/frame.h>

void voe_editor_world_step(voe_ecs_world *world, const voe_3d_shapes *shapes,
			   float seconds, const char *folder,
			   voe_base_arena *scratch, voe_editor_notice *why)
{
	VOE_BASE_ASSERT(world != NULL && shapes != NULL,
			"stepping no world or with no shapes");
	VOE_BASE_ASSERT(scratch != NULL && why != NULL,
			"stepping with no scratch or nowhere to say why");
	voe_game_world_step(world, shapes);
	voe_3d_emitter_system_run(world, seconds);
	voe_editor_prefabs_expand(world, folder, scratch, why);
}
