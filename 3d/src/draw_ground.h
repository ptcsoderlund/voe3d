// The painted grounds a frame's model rows may blend into (0481 points 2, 3
// and 5). Internal to 3d: draw_system.c's run builds them on its scratch once
// a frame, before the model walk, and hands them to draw_models.c's walk.
//
//     struct voe_3d_grounds grounds = voe_3d_draw_grounds(world, &frame, arena);
//     const struct voe_3d_ground *under =
//             voe_3d_draw_ground_under(&grounds, origin_about_the_eye);
//     if (under != NULL)
//             voe_3d_draw_ground_fill(under, row.blend_height, &object);
//
// A GROUND IS A LANDSCAPE ROW THE RUN DRAWS SOLID (0481 point 3): among its
// first VOE_3D_LANDSCAPES_DRAWN landscape rows, as draw_system.c's landscape
// walk counts them, with off, hidden and fading rows left out, at the world
// place that walk takes. Only a painted one (`paint[0]` not nought) is a
// ground; its paint and layers are asked of the store each frame as
// draw_terrain.c asks them, so a repaint or a tint shows at once.
//
// WHICH GROUND: the first whose square, in its own x and z about its centre,
// holds the point. Its height plays no part: a model above or below the
// landscape's square blends as far as the reach and the shader's weight say.
//
// The grounds are on the caller's scratch, given back by its rewind; nothing
// here holds anything between frames.
#pragma once

#include <3d/draw_system.h>
#include <base/arena.h>
#include <ecs/world.h>
#include <math/float3.h>
#include <math/float4.h>
#include <render/device.h>

#include <stdint.h>

// One painted landscape row as a ground record names it.
struct voe_3d_ground {
	// The row in the model table.
	uint32_t row;
	// Its heights texture index plus one, its paint planes' and its layers'
	// shading records, as a node record names them.
	uint32_t heights;
	uint32_t paint[2];
	uint32_t layers[8];
	// The top three rows of the map from camera-relative space to the
	// landscape's own metres: the inverse of its world place about the eye.
	voe_math_float4 rows[3];
	// The landscape's side, metres.
	float side;
};

// A frame's grounds, `count` of them; none is NULL and 0.
struct voe_3d_grounds {
	const struct voe_3d_ground *grounds;
	uint32_t count;
};

// Every painted landscape row `frame` draws solid, at most
// VOE_3D_LANDSCAPES_DRAWN, on `arena`. None with no store.
struct voe_3d_grounds voe_3d_draw_grounds(const voe_ecs_world *world,
					  const voe_3d_frame *frame,
					  voe_base_arena *arena);

// The first of `grounds` whose square holds camera-relative point `at` in its
// own x and z; NULL with none.
const struct voe_3d_ground *
voe_3d_draw_ground_under(const struct voe_3d_grounds *grounds,
			 voe_math_float3 at);

// `object` made a ground record of `ground` (render/device.h): its `ground`,
// `paint`, `layers`, `ground_rows`, `terrain.w` and `ground_reach`, `reach`
// clamped to VOE_3D_MODEL_BLEND_HEIGHT_LEAST-_GREATEST, 0.05-10 m, the model's
// blend height range (3d/model_component.h). A reach not above nought, or not a
// number, fills nothing.
void voe_3d_draw_ground_fill(const struct voe_3d_ground *ground, float reach,
			     voe_render_object *object);
