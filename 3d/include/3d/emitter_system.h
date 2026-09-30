// The emitter system: the one place emitter and particles rows change (0298
// points 2-4). The game runs it once a fixed step, the editor once a frame.
//
//     voe_scene_transform_register(world, capacity);
//     voe_3d_emitter_register(world, capacity);
//     ...
//     voe_3d_emitter_system_run(world, seconds);            // each step
//
// A RUN, IN ORDER:
// - it drains the replace intents: a dead entity or one with no emitter is
//   dropped; a texture with no NUL is ended and reported, as the model's
//   (3d/model_component.h), the first of a run named and the rest counted;
// - it drains the controls: play sets playing and adds the row's burst, stop
//   clears playing, burst adds `count`, or the row's burst when it is 0;
// - it adds a particles row to each emitter lacking one and drops the rows whose
//   emitter is gone, as the shape system adds its rows (3d/shape_system.h), so a
//   removed emitter's row is gone after the next run; a new row whose emitter
//   plays takes the burst;
// - it spawns `rate x seconds` particles, the fraction carried in the debt, plus
//   the pending burst, up to VOE_3D_EMITTER_PARTICLES live; what does not fit is
//   dropped. Each is born at the offset through the entity's world matrix, with
//   `speed` along a random direction within `spread` degrees of `direction`
//   through the world rotation, and a life of `life`;
// - it integrates: velocity gains `rise` along world up and loses `drag` a
//   second; position moves; age grows; a particle past its life is removed by
//   swapping the last one in, so particle order is not birth order;
// - `seconds` of 0 moves and spawns nothing but drains, adds and drops.
//
// THE RANDOM NUMBERS ARE A XORSHIFT IN THE ROW, seeded from the entity when the
// row is added, so a run from the same world is the same each time.
//
// AN EMITTER WITH NO TRANSFORM SPAWNS NOTHING: the component needs one, and a
// row that lacks it anyway is not placed anywhere. Its live particles still age.
//
// Constraints: the report's run and count are per process, as the model's are;
// a control or replace for an entity with no emitter is dropped silently.
#pragma once

#include <3d/emitter_component.h>

#include <ecs/world.h>

// Drains both intents, adds and drops particles rows, spawns, integrates and
// kills, `seconds` on. Asserts seconds >= 0 and the emitter registered.
void voe_3d_emitter_system_run(voe_ecs_world *world, float seconds);
