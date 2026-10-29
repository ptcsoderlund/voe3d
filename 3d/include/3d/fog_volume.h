// A world's fog volumes as a pass's records (0493 points 1 to 3): each Fog
// volume row's box placed by its transform, and the rows as render's
// voe_render_fog about an eye.
//
//     voe_physics_shape box;
//     if (voe_3d_fog_volume_shape(world, entity, frame.lag, &box)) { ... }
//     camera.fog = voe_3d_fog_volumes(world, frame.eye, frame.lag);
//
// ONE BOX FOR THE PASS AND THE EDITOR'S LINES, as the light blocker's
// (3d/light_blocker.h): voe_3d_fog_volumes and the editor's lines for every
// volume both start from voe_3d_fog_volume_shape, so the lines drawn are the
// box the fog fills. Its centre is the world place at the lag in double
// (ADR-0250), its rotation the world rotation, its half |scale| × size / 2.
//
// THE DENSITY IS 0.1 × THICKNESS², extinction per metre in the thickest place.
// Squared so a slider's low end is thin fog: linear, the first tenth of the
// slider would already hide the far trees.
//
// THE FIELDS WRAP IN DOUBLE. `patches` is eye − drift and `change` eye −
// (1.5 × drift x, 0.25 × |drift|, 1.5 × drift z), each wrapped per axis into
// 0 to VOE_RENDER_FOG_PATCHES and 0 to VOE_RENDER_FOG_CHANGE before it becomes
// a float, so a far eye or a long wind keeps its precision, as the clouds'
// origin does. A row with no drift yet has moved nowhere.
//
// ANY EYE TAKES IT: the pass's fog takes the frame's eye, and an editor view
// with its own eye calls it with that eye, as it calls the clouds'.
//
// Constraints: the fog volume table is looked for by a walk of the world's
// types, as draw_clouds.c's is, because voe_ecs_component_type asserts on a key
// nothing registered. Rows past VOE_RENDER_FOG_VOLUMES in table order are left
// out; lifting that is render's record growing.
#pragma once

#include <ecs/world.h>
#include <math/double3.h>
#include <physics/shape.h>
#include <render/device.h>

// The box `entity`'s fog volume is at `lag` of a step back (0254), into *box.
// False, *box untouched, when the entity is dead or has no fog volume row or no
// transform.
[[nodiscard]] bool voe_3d_fog_volume_shape(const voe_ecs_world *world,
					   voe_ecs_entity entity, float lag,
					   voe_physics_shape *box);

// Every row whose entity is on and has a transform, in table order, at most
// VOE_RENDER_FOG_VOLUMES, about `eye` at `lag`; a box with a half of nought on
// an axis, or thickness 0, left out. Zeroed with no table or no row.
voe_render_fog voe_3d_fog_volumes(const voe_ecs_world *world,
				  voe_math_double3 eye, float lag);
