// The only way a transform changes: an intent, drained by this system.
//
//     voe_scene_transform_register(world, 4096);            // once, at startup
//     voe_scene_transform_add(world, entity, (voe_scene_transform){ ... });
//
//     // from anywhere, any number of times:
//     voe_scene_transform_submit(world, (voe_scene_transform_intent){
//             .entity = entity, .transform = moved });
//
//     // once a frame, in the frame loop:
//     voe_scene_transform_system_run(world);
//
// THE INTENT CARRIES THE WHOLE TRANSFORM AND NOT A DELTA. A submitter that wants
// to move one thing reads the current transform — reading is anybody's — changes
// the field it cares about and submits the result. That is one intent type
// instead of three, and it makes two submitters in one frame resolve the way
// last-writer-wins rather than by adding up in an order nobody chose. It is
// registered as the component's replace intent (ecs/component.h), which is how
// an inspector that knows nothing about this folder finds the door to knock on.
//
// CREATION IS A DIRECT CALL AND NOT AN INTENT, which is the one asymmetry here.
// An intent is applied to a component that exists; giving an entity its first
// transform is what makes it exist, and it happens where the entity is being
// built rather than in the middle of a frame. The importer uses this and so does
// any call site placing something by hand.
//
// NEITHER CALL BELOW CHECKS THE ROTATION, AND THE DRAIN CHECKS EVERY ONE. There
// is no assert here to match the identity's: a rotation that has drifted is
// ordinary arithmetic rather than a caller's bug, the importer hands these
// numbers straight from a file, and what reaches the queue may have come through
// voe_ecs_intent_submit from a tool that only knows the offsets. One place that
// settles every door is the whole of why the drain is where the rules live — see
// scene/transform_component.h for what it settles and why each is a correction
// or a refusal.
//
// THE REPORT IS ONE LINE AT THE START OF A RUN AND ONE AT ITS END, AND ITS STATE
// IS PER PROCESS AND NOT PER WORLD. A drain that settles an intent after a drain
// that settled none writes one line naming that first intent — the system, the
// entity, the field, the value submitted and what was applied — and every
// settling after it is counted, until the first drain that settles none closes
// the run with the count. A normalisation inside the tolerance is not a settling
// and starts no run. The closing line reads `error` when anything in the run was
// kept and `warning` when everything in it was corrected, so the worse of the
// two is the word that survives. The flag and the counters are file-scope
// statics in transform_system.c, so two worlds in one process share them. That
// is enough for a report: it is there to be noticed by whoever is watching
// stderr, not to be attributed or parsed, and a second world in one process is a
// test harness rather than a shipped program.
//
// THE ENTITY IS NAMED BY ITS IDENTITY WHERE IT HAS ONE. A world that registered
// no identities — dev registers none — gets the index and generation instead,
// and the drain finds that out by walking the world's component types rather
// than by a lookup that would assert on the world most likely to be reported on.
//
// IT SITS AT "Transform" IN ADD COMPONENT, a menu path registered with the type
// (ecs/component.h, 0221), so the menu is never a list kept by hand.
#pragma once

#include <ecs/world.h>
#include <scene/transform_component.h>

#include <stdint.h>

// Registers the table, its description, the intent queue, the intent as the
// component's replace, and the default row: at the origin, unrotated, scale one.
// The default row is what "add at default" gives (0190). Call it once per world,
// before anything adds a transform.
// capacity is how many transforms the world may hold and also how many intents
// may be waiting at once: one submission per transform per frame is what that
// sizing assumes.
void voe_scene_transform_register(voe_ecs_world *world, uint32_t capacity);

// Gives the entity its transform. False when the table is full or the entity is
// not alive.
[[nodiscard]] bool voe_scene_transform_add(voe_ecs_world *world,
					   voe_ecs_entity entity,
					   voe_scene_transform transform);

// Put this entity's transform where the submitter says.
typedef struct {
	voe_ecs_entity entity;
	voe_scene_transform transform;
} voe_scene_transform_intent;

// False when the queue is full — the system has not run for long enough, and
// the caller is the one that can do something about that.
[[nodiscard]] bool voe_scene_transform_submit(voe_ecs_world *world,
					      voe_scene_transform_intent intent);

// Settles every waiting intent, applies it in submission order, and empties the
// queue.
//
// AN INTENT NAMING A DESTROYED ENTITY, OR ONE WITH NO TRANSFORM, IS DROPPED,
// SILENTLY AND ON PURPOSE. An entity dying between a submit and the drain is
// ordinary — it is what a queue costs — and it is not the submitter's mistake.
// The drain asks for the current row before it settles anything, so a dropped
// intent is never reported: there is no last valid row to keep, and a line about
// an entity that no longer exists would be noise in front of the lines that
// matter.
void voe_scene_transform_system_run(voe_ecs_world *world);
