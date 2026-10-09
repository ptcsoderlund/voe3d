// A landscape row drawn by its chosen nodes (0396 points 3 and 4). Internal to
// 3d: draw_system.c hands it each landscape row's entry and world place, and
// draw_shadows.c hands the cast the same for every shadow and capture pass.
//
//     struct voe_3d_terrain_nodes nodes = voe_3d_draw_terrain_nodes(
//             models, entry, world, voe_3d_normal_matrix(world), scratch);
//     if (fading)
//             voe_3d_draw_terrain_hold(&blended, &nodes, fade, view.view);
//     else
//             (void)voe_3d_draw_terrain_solid(device, &nodes);
//
// THE SAME EYE CHOOSES THE SAME NODES. Selection reads only the eye, which is
// the origin of the row's world matrix about the frame's eye, and the row's
// pyramid; so every pass of one frame draws the same ground.
//
// A node's records are on the caller's scratch, given back by its rewind.
#pragma once

#include "draw_group.h"

#include <3d/models.h>
#include <base/arena.h>
#include <math/float4x4.h>
#include <render/device.h>

#include <stdbool.h>
#include <stdint.h>

// One landscape row's nodes, each record ready to draw on `grid`; `count`
// nought draws nothing.
struct voe_3d_terrain_nodes {
	voe_render_geometry grid;
	// The landscape's one part: its material and its faded twin.
	const voe_3d_model_part *part;
	voe_render_object *objects;
	uint32_t count;
};

// `entry`'s nodes as the eye at `world`'s origin chooses them, at most
// VOE_3D_LANDSCAPE_NODES: each `world` × its box, `normal` the row's own,
// solid in the part's record and white. None for an entry that is no loaded
// landscape of `models`, or a `world` scaled to nothing.
struct voe_3d_terrain_nodes
voe_3d_draw_terrain_nodes(const voe_3d_models *models,
			  const voe_3d_model_entry *entry,
			  voe_math_float4x4 world, voe_math_float4x4 normal,
			  voe_base_arena *scratch);

// Every node drawn solid; false when render refuses a draw, which stops here.
bool voe_3d_draw_terrain_solid(voe_render_device *device,
			       const struct voe_3d_terrain_nodes *nodes);

// `entry`'s nodes as voe_3d_draw_terrain_nodes chooses and records them, drawn
// solid into the pass that is open with no scratch: a shadow, capture or sun
// map's caster. True, nothing drawn, where _nodes chooses none; false when
// render refuses a draw, which stops here.
bool voe_3d_draw_terrain_cast(voe_render_device *device,
			      const voe_3d_models *models,
			      const voe_3d_model_entry *entry,
			      voe_math_float4x4 world, voe_math_float4x4 normal);

// Every node held in `group` in the part's faded record at alpha 1 − `fade`,
// as a fading model's part is (0336 point 3).
void voe_3d_draw_terrain_hold(struct voe_3d_draw_group *group,
			      const struct voe_3d_terrain_nodes *nodes,
			      float fade, voe_math_float4x4 view);
