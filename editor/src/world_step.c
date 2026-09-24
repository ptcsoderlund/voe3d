// The world's step once a frame, in the order world_step.h gives and says why.
//
// Constraints: every call runs every system, unconditionally; nothing here
// decides whether one is needed.
#include "world_step.h"

#include <ecs/structure.h>

#include <game/project.h>

#include <scene/identity_system.h>
#include <scene/light_system.h>
#include <scene/transform_system.h>

void voe_editor_world_step(voe_ecs_world *world, const voe_3d_shapes *shapes)
{
	voe_ecs_structure_apply(world);
	voe_game_project_replaces_apply(world);

	voe_scene_transform_system_run(world);
	voe_scene_identity_system_run(world);
	voe_scene_light_system_run(world);

	voe_3d_shape_system_run(world, shapes);
}
