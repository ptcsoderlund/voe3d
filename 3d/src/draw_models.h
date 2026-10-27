// The model rows drawn from the frame's store. Internal to 3d: draw_system.c's
// run calls the walk once a frame, after the mesh walk and before the
// landscapes, and asks the part count when it sizes the world's blended group;
// its landscape walk asks the entry and landscape test of each row.
//
//     world_blended = voe_3d_draw_group_new(arena,
//             ... + voe_3d_draw_models_part_count(world, frame.models), true);
//     grounds = voe_3d_draw_grounds(world, &frame, arena);
//     (void)voe_3d_draw_models(world, device, &frame, &world_blended, &cull,
//                              &grounds, &counts);
//
// A landscape row is skipped here and drawn by draw_terrain.c's nodes. A row
// the cull cannot see is skipped and counted (0464 points 2 and 5). A row
// blending with a ground under it (0481) has its parts filled with it and its
// solid ones drawn last, in the `ground blend` span.
#pragma once

#include "cull.h"
#include "draw_ground.h"
#include "draw_group.h"

#include <3d/draw_system.h>
#include <3d/models.h>
#include <ecs/world.h>
#include <render/device.h>

#include <stdbool.h>
#include <stdint.h>

// The loaded entry model row `row` draws from, NULL when it draws nothing: no
// store, no transform, or a path the store lacks or failed on.
const voe_3d_model_entry *voe_3d_draw_models_entry(const voe_ecs_world *world,
						   const voe_3d_models *models,
						   uint32_t row);

// Whether a loaded entry is a landscape, drawn by draw_terrain.c's nodes.
bool voe_3d_draw_models_is_landscape(const voe_3d_models *models,
				     const voe_3d_model_entry *entry);

// Every loaded part of every model row, VOE_3D_LANDSCAPE_NODES for a landscape
// row: the room the world's blended group keeps for them. Nought with no store.
uint32_t voe_3d_draw_models_part_count(const voe_ecs_world *world,
				       const voe_3d_models *models);

// Each model row's solid parts drawn now and blended or fading ones held in
// `world_blended`; false when a draw is refused, which stops this walk only.
// A row, not a landscape, whose entry's `centre` and `radius` under its world
// matrix about the eye `cull` does not see draws and holds none of its parts;
// NULL tests nothing. Each such row is one thing in `counts`, drawn or skipped.
// A row with `blend_with_ground` over one of `grounds` has every part's record
// filled with it at its `blend_height`, and its solid parts drawn after every
// other row's inside one `ground blend` span, opened only when there is one.
bool voe_3d_draw_models(voe_ecs_world *world, voe_render_device *device,
			const voe_3d_frame *frame,
			struct voe_3d_draw_group *world_blended,
			const struct voe_3d_cull *cull,
			const struct voe_3d_grounds *grounds,
			voe_3d_view_counts *counts);
