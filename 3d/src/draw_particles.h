// Every live particle as one blended draw in the world layer (ADR-0298 point 6).
// Internal to 3d: voe_3d_draw_system_run sizes the world's blended group with
// _count and fills it with _hold, and the group's sort puts each particle among
// the other see-through drawables.
//
//     uint32_t room = ... + voe_3d_draw_particles_count(world, frame.models);
//     struct voe_3d_draw_group blended = voe_3d_draw_group_new(arena, room, true);
//     voe_3d_draw_particles_hold(world, &frame, &blended);
//
// A PARTICLE IS HELD, NEVER DRAWN AS FOUND: its picture is blended, so it waits
// for the sort. It goes to no shadow pass (3d/src/draw_shadows.c reads none).
//
// Constraints: a world that never registered the emitter draws no particles
// rather than asserting, as a world without shapes draws white; with a NULL
// store neither table is read.
#pragma once

#include "draw_group.h"

#include <3d/draw_system.h>
#include <3d/models.h>
#include <ecs/world.h>

#include <stdint.h>

// Every live particle in the world, which the blended group is sized for. Nought
// with no store or no particles table.
uint32_t voe_3d_draw_particles_count(const voe_ecs_world *world,
				     const voe_3d_models *models);

// Holds one draw per live particle whose emitter's picture `frame->models` has
// loaded in `world_blended`, the frame's hidden entity's particles left out.
void voe_3d_draw_particles_hold(const voe_ecs_world *world,
				const voe_3d_frame *frame,
				struct voe_3d_draw_group *world_blended);
