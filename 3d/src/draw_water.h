// Every water with a transform and a waves row as one blended draw in the world
// layer (ADR-0305 point 7). Internal to 3d: voe_3d_draw_system_run sizes the
// world's blended group with _count, fills it with _hold, and copies the depth
// once before drawing that group when _hold held any.
//
//     uint32_t room = ... + voe_3d_draw_water_count(world, frame.models);
//     struct voe_3d_draw_group blended = voe_3d_draw_group_new(arena, room, true);
//     if (voe_3d_draw_water_hold(world, &frame, &blended) > 0)
//             ... voe_render_frame_copy_depth before the group is drawn
//
// WATER CASTS NO SHADOW: it is held here only, and 3d/src/draw_shadows.c reads
// no water table, so the shore under it stays lit as the plane is see-through.
//
// THE STORE'S QUAD IS TURNED. It lies in model XY facing +Z; water is a floor,
// so a quarter turn about X takes it to XZ facing +Y, after it is scaled by
// (width, length, 1): width along the transform's X, length along its Z.
//
// EYE-RELATIVE, as every drawable (ADR-0250): the entity's matrix is taken
// about the frame's eye in double before the turn is multiplied on.
//
// Constraints: a world that never registered the water draws none rather than
// asserting, and with a NULL store, or a store with no water record
// (voe_3d_models_load_water), neither table is read.
#pragma once

#include "draw_group.h"

#include <3d/draw_system.h>
#include <3d/models.h>
#include <ecs/world.h>

#include <stdint.h>

// Every water row, which the blended group is sized for. Nought with no store,
// no water record or no water table.
uint32_t voe_3d_draw_water_count(const voe_ecs_world *world,
				 const voe_3d_models *models);

// Holds one draw per water with a transform and a waves row in
// `world_blended`, the frame's hidden entity left out; how many it held.
uint32_t voe_3d_draw_water_hold(const voe_ecs_world *world,
				const voe_3d_frame *frame,
				struct voe_3d_draw_group *world_blended);
