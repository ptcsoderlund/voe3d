// The only way the sun changes: an intent, drained by this system. Which way it
// shines is its transform's, and changes through transform_system.h.
//
//     voe_scene_transform_register(world, 64);              // once, at startup
//     voe_scene_light_register(world, 2);
//     voe_scene_light_add(world, sun, (voe_scene_light){
//             .colour = { 1.0f, 0.98f, 0.92f }, .intensity = 3.0f,
//             .fill_colour = { 0.6f, 0.7f, 1.0f }, .fill_intensity = 0.2f });
//     // turned with a transform whose rotation is voe_scene_light_facing(...)
//
//     // from anywhere: a time of day, a key, the Inspector
//     voe_scene_light_submit(world, (voe_scene_light_intent){
//             .entity = sun, .light = dimmer });
//
//     // once a frame, in the frame loop, before the draw
//     voe_scene_light_system_run(world);
//
// THE INTENT CARRIES THE WHOLE LIGHT AND NOT A DELTA, for the reason
// transform's does: a submitter reads the current one — reading is anybody's —
// changes the field it cares about and submits the result. Two submitters in
// one frame then resolve as last-writer-wins rather than by adding up in an
// order nobody chose. It is the light's replace, so the Inspector edits it live.
//
// CREATION IS A DIRECT CALL AND NOT AN INTENT, the same asymmetry the other
// modules in this folder have. An intent is applied to a component that exists;
// giving an entity its first light is what makes it exist.
//
// NOTHING IS NORMALIZED OR CORRECTED; A BAD LIGHT IS REFUSED. A number that is
// not finite, a colour channel outside [0, 1] or a negative intensity has no
// nearest right answer, so `add` asserts on one and the drain keeps the last
// valid row and says so on stderr, one line per refused intent.
//
// AN INTENT NAMING A DESTROYED ENTITY IS DROPPED, SILENTLY AND ON PURPOSE. An
// entity dying between a submit and the drain is ordinary — it is what a queue
// costs — and it is not the submitter's mistake.
//
// IT SITS AT "Rendering / Light" IN ADD COMPONENT, a menu path registered with
// the type (ecs/component.h, 0221), so the menu is never a list kept by hand.
#pragma once

#include <ecs/world.h>
#include <scene/light_component.h>

#include <stdint.h>

// Registers the table, the intent queue as its replace, the transform it needs
// and the default row: white of strength one, a white fill of nought. The
// default row is what "add at default" gives (0190). Call it once per world,
// after the transform table (it asserts on none) and before anything adds a
// light. capacity is how many lights the world may hold and also how many
// intents may be waiting at once.
//
// A WORLD THAT IS DRAWN NEEDS THIS EVEN IF IT HOLDS NO SUN, because the draw
// system reads the table — see 3d/draw_system.h, which is also where "at most
// one" is said and a world with no light draws unshaded (0238).
void voe_scene_light_register(voe_ecs_world *world, uint32_t capacity);

// Gives the entity its light, as given; a light the drain would refuse asserts.
// False when the table is full or the entity is not alive.
[[nodiscard]] bool voe_scene_light_add(voe_ecs_world *world,
				       voe_ecs_entity entity,
				       voe_scene_light light);

// Make this entity's light the one the submitter says.
typedef struct {
	voe_ecs_entity entity;
	voe_scene_light light;
} voe_scene_light_intent;

// False when the queue is full — the system has not run for long enough, and
// the caller is the one that can do something about that.
[[nodiscard]] bool voe_scene_light_submit(voe_ecs_world *world,
					  voe_scene_light_intent intent);

// Applies every waiting intent, in submission order, and empties the queue.
void voe_scene_light_system_run(voe_ecs_world *world);
