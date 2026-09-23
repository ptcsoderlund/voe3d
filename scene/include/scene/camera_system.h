// The only way a camera's lens changes: an intent, drained by this system.
//
//     voe_scene_transform_register(world, 4096);   // first: a camera needs one
//     voe_scene_camera_register(world, 1);
//     voe_scene_camera_add(world, entity, (voe_scene_camera){ ... });
//
//     voe_scene_camera_submit(world, (voe_scene_camera_intent){
//             .entity = entity, .camera = wider });
//
//     voe_scene_camera_system_run(world);
//
// THE INTENT CARRIES THE WHOLE LENS AND NOT A DELTA, as the transform's carries
// the whole transform. A submitter reads the current row, changes the field it
// cares about and submits the result, so two submitters in one frame resolve as
// last-writer-wins rather than by adding up in an order nobody chose. It is
// registered as the component's replace intent (ecs/component.h), which is how
// an inspector that knows nothing about this folder edits a lens.
//
// A LENS THAT CANNOT PROJECT IS REFUSED AND THE ROW IS KEPT (0223): any number
// not finite, a fov_y not inside (0, π), a near_plane not above nought, or a
// far_plane not above the near one. The drain writes one `error:` line to stderr
// naming the entity and the field, and the last valid row stays.
//
// A CAMERA NEEDS A TRANSFORM, registered as its needed type, and is moved only by
// transform intents (0222): there is no camera intent that places or turns it,
// and nothing here reads or writes where it is.
//
// IT SETS NO MENU PATH, SO ADD COMPONENT NEVER OFFERS A CAMERA: a scene has
// exactly one, made with it (0218), and a type without a path is not offered
// (ecs/component.h, 0221).
#pragma once

#include <ecs/world.h>
#include <scene/camera_component.h>

#include <stdint.h>

// Registers the table, its description, the intent queue, the intent as the
// component's replace, the default row (60° of field of view, planes at 0.1 and
// 1000) and the transform as the type a camera needs. The default row is what
// "add at default" gives (0190). The transform must be registered first; that
// asserts. capacity is how many cameras the world may hold, and how many intents
// may be waiting.
void voe_scene_camera_register(voe_ecs_world *world, uint32_t capacity);

// False when the table is full or the entity is not alive.
[[nodiscard]] bool voe_scene_camera_add(voe_ecs_world *world,
					voe_ecs_entity entity,
					voe_scene_camera camera);

// Put this entity's lens where the submitter says.
typedef struct {
	voe_ecs_entity entity;
	voe_scene_camera camera;
} voe_scene_camera_intent;

// False when the queue is full.
[[nodiscard]] bool voe_scene_camera_submit(voe_ecs_world *world,
					   voe_scene_camera_intent intent);

// Applies every waiting intent in submission order, and empties the queue. An
// intent naming a destroyed entity, or one with no camera, is dropped silently:
// an entity dying between a submit and the drain is what a queue costs.
void voe_scene_camera_system_run(voe_ecs_world *world);
