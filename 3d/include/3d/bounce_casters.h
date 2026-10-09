// Where each caster of the probe bounce stood, kept from one frame to the next
// so a removed caster can mark where it stood (0394). An entity's rows leave
// with it when it is destroyed, the previous transform table's too, so nothing
// in the world remembers its place; this does.
//
//     static voe_3d_bounce_casters memory;    // zeroed is empty
//     voe_3d_frame frame = voe_3d_draw_system_frame(world, size, lag);
//     frame.casters = &memory;                // this target's, every frame
//     (void)voe_3d_draw_system_shadows(world, device, &frame);
//
// ONE PER TARGET, AND THE CALLER KEEPS IT. A memory feeds the bounce of the
// target it is handed with, since each target has its own volumes: one per
// editor view, one for the preview, one for the game's window. The caller
// owns the storage and sets `frame->casters` each frame; only 3d writes it,
// in the shadows call, where the bounce reads it and then refills it with
// this frame's casters. NULL in the frame keeps none, and a removed caster
// then marks nothing, as before 0394.
//
// Constraints: VOE_3D_BOUNCE_CASTERS entries inline, no arena, about 20 KB a
// target. Past that many casters the rest are not remembered and their
// removal marks nothing; a larger count or an arena on the caller would lift
// it.
#pragma once

#include <ecs/world.h>
#include <math/double3.h>

#include <stdint.h>

// How many casters one memory holds (0394 point 1).
#define VOE_3D_BOUNCE_CASTERS 512u

// One remembered caster: its entity, its world centre at lag 0 in double about
// the world origin, and its world bounding radius (0389 point 7).
typedef struct {
	voe_ecs_entity entity;
	voe_math_double3 centre;
	float radius;
} voe_3d_bounce_caster;

// The casters of a target's last bounce, `count` of them; zeroed is empty.
typedef struct {
	uint32_t count;
	voe_3d_bounce_caster entries[VOE_3D_BOUNCE_CASTERS];
} voe_3d_bounce_casters;
