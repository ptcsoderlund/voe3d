// Tank light fade: the tank game's own fade of a point light's intensity, from
// full to dark, which its shots and wrecks flash by (0322 point 6).
//
//     tank_light_fade_register(world);           // in voe_game_project_register
//     tank_light_fade_start(world, gun);         // per shot fired
//     tank_light_fade_system_run(step);          // after the enemy, before the camera
//
// IT IS GAME CODE, NOT THE ENGINE'S (0321 point 2): a light shines at its
// intensity, and a game that wants a flash fades that intensity itself.
//
// `peak` is the intensity at full, default 4. `seconds` is how long it fades,
// default 0.12. `left` is the seconds still to fade, default 0; one written in
// a prefab is a fade from full when it is spawned, as a wreck's is. Each step
// the light shows `peak × left / seconds`, then `left` counts down by the
// step, never below 0. A `seconds` of 0 or less is dark. A fade needs a point
// light.
//
// The light is set only through its replace, and only when its intensity
// differs, so a resting lamp costs no intent.
//
// Constraints: at most VOE_GAME_WORLD_POINT_LIGHTS rows, one a light. A full
// point light queue skips that step's fade; `left` still counts down, so the
// fade ends on time. A negative `peak` is the drain's to refuse.
#pragma once

#include <base/describe.h>

#include <ecs/world.h>

#include <game/project.h>

#include <stdbool.h>

#define TANK_LIGHT_FADE_FIELDS(F, F_READ_ONLY) \
	F(float, peak, FLOAT32)                \
	F(float, seconds, FLOAT32)             \
	F(float, left, FLOAT32)

VOE_BASE_DESCRIBE_STRUCT(tank_light_fade, TANK_LIGHT_FADE_FIELDS)

extern const struct voe_ecs_key tank_light_fade_key;

// Registers the type under "Tank / Light fade" with its defaults, needing a
// point light. False, reported, when game refuses it.
[[nodiscard]] bool tank_light_fade_register(voe_ecs_world *world);

// Restarts the entity's fade from full: `left` becomes `seconds`. False when
// it has no fade row.
bool tank_light_fade_start(voe_ecs_world *world, voe_ecs_entity entity);

// Sets every faded light to this step's intensity, then counts each `left`
// down by the step's seconds.
void tank_light_fade_system_run(const voe_game_project_step *step);
