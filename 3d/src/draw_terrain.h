// A landscape row drawn by its chosen nodes (0396 points 3 and 4). Internal to
// 3d: draw_system.c hands it each landscape row's entry and world place, and
// draw_shadows.c hands the cast the same for every shadow and capture pass.
//
//     struct voe_3d_terrain_nodes nodes = voe_3d_draw_terrain_nodes(
//             models, entry, world, voe_3d_normal_matrix(world), scratch);
//     if (fading)
//             voe_3d_draw_terrain_hold(&blended, &nodes, fade, view.view,
//                                      &cull, &counts);
//     else
//             (void)voe_3d_draw_terrain_solid(device, &nodes, &cull, &counts);
//
// THE SAME EYE CHOOSES THE SAME NODES. Selection reads only the eye, which is
// the origin of the row's world matrix about the frame's eye, and the row's
// pyramid; so every pass of one frame draws the same ground. The bounce's
// capture passes and sun maps choose fewer from that eye (0397), so coarser
// nodes of the same ground; a crack between nodes two levels apart is
// accepted there. The cull comes after the choice (0464 point 2): a pass skips
// the chosen nodes whose box sphere its volume cannot see, so a node never
// changes level by being culled.
//
// A PAINTED LANDSCAPE'S LAYERS ARE ASKED OF THE STORE EACH FRAME (0402 point
// 4): every record names its paint and each layer's material record, so an
// Inspector tint or a reloaded map shows at once, in every pass alike.
//
// A node's records are on the caller's scratch, given back by its rewind.
#pragma once

#include "cull.h"
#include "draw_group.h"

#include <3d/draw_system.h>
#include <3d/models.h>
#include <base/arena.h>
#include <math/float4x4.h>
#include <render/device.h>

#include <stdbool.h>
#include <stdint.h>

// The most nodes a bounce capture pass or sun map draws of one landscape
// (0397 point 1): at the view's detail each capture face drew every node,
// and sixteen cut a capture's vertices by an order.
#define VOE_3D_TERRAIN_BOUNCE_NODES 16

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
// solid in the part's record and white, a painted one's paint and layers
// named. None for an entry that is no loaded
// landscape of `models`, or a `world` scaled to nothing.
struct voe_3d_terrain_nodes
voe_3d_draw_terrain_nodes(const voe_3d_models *models,
			  const voe_3d_model_entry *entry,
			  voe_math_float4x4 world, voe_math_float4x4 normal,
			  voe_base_arena *scratch);

// Every node `cull` sees drawn solid, NULL seeing all; each node counted into
// `counts`' `drawn` or `skipped` unless NULL. False when render refuses a
// draw, which stops here.
bool voe_3d_draw_terrain_solid(voe_render_device *device,
			       const struct voe_3d_terrain_nodes *nodes,
			       const struct voe_3d_cull *cull,
			       voe_3d_view_counts *counts);

// `entry`'s nodes as voe_3d_draw_terrain_nodes chooses and records them, but
// at most `capacity` (1 to VOE_3D_LANDSCAPE_NODES), drawn solid into the pass
// that is open with no scratch: a shadow, capture or sun map's caster. A
// capacity below what the eye wants draws coarser nodes, never other ground.
// A node `cull` cannot see is skipped, NULL seeing all; each counted into
// `counts`' `cast` or `cast_skipped` unless NULL. True, nothing drawn, where
// none is chosen; false when render refuses a draw, which stops here.
bool voe_3d_draw_terrain_cast(voe_render_device *device,
			      const voe_3d_models *models,
			      const voe_3d_model_entry *entry,
			      voe_math_float4x4 world, voe_math_float4x4 normal,
			      uint32_t capacity, const struct voe_3d_cull *cull,
			      voe_3d_view_counts *counts);

// Every node `cull` sees held in `group` in the part's faded record at alpha
// 1 − `fade`, as a fading model's part is (0336 point 3); culled and counted
// as voe_3d_draw_terrain_solid's.
void voe_3d_draw_terrain_hold(struct voe_3d_draw_group *group,
			      const struct voe_3d_terrain_nodes *nodes,
			      float fade, voe_math_float4x4 view,
			      const struct voe_3d_cull *cull,
			      voe_3d_view_counts *counts);
