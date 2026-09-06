// The sprite exhibit: one sheet built in code, one quad, and the twelve entities
// that show what a 2D sprite is in this engine. Placeholder scene data and
// placement at a call site, the same kind of thing cubes.h and quad.h are.
//
// IT IS A CALL SITE AND NOT A SECOND ENGINE. Everything here is `sprite`'s and
// `3d`'s public calls in the order a program would make them; the only thing
// this file decides is where the sprites stand and what the sheet's pixels are.
//
// THE ENGINE DOES NOT BILLBOARD (ADR-0080), so the turning towards the camera is
// in here rather than behind an API. voe_dev_sprites_face is the whole of it:
// two rotations built out of the camera's own yaw and pitch, one line apart, so
// that a person can see both what the difference is and that the engine had
// nothing to do with it.
#pragma once

#include <base/arena.h>
#include <base/error.h>
#include <ecs/world.h>
#include <render/device.h>

// The two sprites that turn to face the camera. Nothing else in the exhibit
// moves, so nothing else is kept.
typedef struct {
	// Rotates about up only: it stays upright as the camera climbs, and a
	// sprite standing on the ground stays standing on it.
	voe_ecs_entity cylindrical;
	// Faces the camera entirely: it leans back as the camera climbs, and its
	// feet leave the ground.
	voe_ecs_entity spherical;
} voe_dev_sprites;

// The sheet, the quad, the nine materials and the twelve entities. `arena` holds
// the sheet's pixels while they are uploaded and is rewound before this returns.
[[nodiscard]] bool voe_dev_sprites_add(voe_ecs_world *world,
				       voe_render_device *gpu,
				       voe_base_arena *arena,
				       voe_dev_sprites *out,
				       voe_base_error *error);

// This frame's rotation for both of the facing sprites, submitted as two
// ordinary transform intents.
//
// CALLED BETWEEN THE CAMERA SYSTEM AND THE TRANSFORM SYSTEM, for the reason
// main.c gives at length about the heads-up line: it is derived from where the
// camera is, so it has to be worked out after the camera has moved and submitted
// before the transforms drain.
void voe_dev_sprites_face(voe_ecs_world *world, voe_ecs_entity eye,
			  const voe_dev_sprites *sprites);
