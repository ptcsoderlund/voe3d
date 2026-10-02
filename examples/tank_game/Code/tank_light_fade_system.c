// The tank light fade system: each fade row into its point light's intensity
// this step, then its `left` counted down (tank_light_fade.h). Teaches
// writing an engine row from game code: the light is the scene's, so it
// changes only through voe_scene_point_light_submit, the whole light with
// its intensity replaced; the fade's own row is written whole through
// voe_ecs_component_set, as the gun's `wait` is.
//
// Constraints: a row whose entity has no point light sets nothing and still
// counts down. A full point light queue loses that step's level, not the
// countdown.
#include "tank_light_fade.h"

#include <base/assert.h>

#include <ecs/component.h>

#include <game/world.h>

#include <scene/parent_component.h>
#include <scene/point_light_system.h>

#include <math.h>

const struct voe_ecs_key tank_light_fade_key = { "tank_light_fade" };

static const tank_light_fade tank_light_fade_default = {
	.peak = 4.0f,
	.seconds = 0.12f,
	.left = 0.0f,
};

bool tank_light_fade_register(voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "registering tank light fade in no world");
	return voe_game_project_component(world, &(voe_game_project_type){
		&tank_light_fade_key, sizeof(tank_light_fade),
		VOE_GAME_WORLD_POINT_LIGHTS,
		VOE_GAME_PROJECT_DESCRIPTION(tank_light_fade),
		&tank_light_fade_default, "Tank / Light fade" }) &&
	       voe_game_project_component_needs(world, &tank_light_fade_key,
						&voe_scene_point_light_key);
}

bool tank_light_fade_start(voe_ecs_world *world, voe_ecs_entity entity)
{
	VOE_BASE_ASSERT(world != NULL, "starting a light fade in no world");
	const voe_ecs_type type =
		voe_ecs_component_type(world, &tank_light_fade_key);
	const tank_light_fade *found =
		voe_ecs_component_get(world, type, entity);

	if (found == NULL)
		return false;
	tank_light_fade next = *found;

	next.left = next.seconds;
	const bool ok = voe_ecs_component_set(world, type, entity, &next);

	VOE_BASE_ASSERT(ok, "a tank light fade row vanished while starting it");
	return ok;
}

bool tank_light_fade_under(const voe_ecs_world *world, voe_ecs_entity parent,
			   voe_ecs_entity *out)
{
	VOE_BASE_ASSERT(world != NULL && out != NULL, "finding no light fade");
	const voe_ecs_type type =
		voe_ecs_component_type(world, &tank_light_fade_key);
	const voe_ecs_entity *entities = voe_ecs_component_entities(world, type);
	const uint32_t count = voe_ecs_component_count(world, type);

	VOE_BASE_ASSERT(count <= VOE_GAME_WORLD_POINT_LIGHTS,
			"more tank light fade rows than were registered");
	for (uint32_t i = 0; i < count; i++) {
		const voe_scene_parent *row =
			voe_scene_parent_get(world, entities[i]);

		if (row != NULL && row->parent.index == parent.index &&
		    row->parent.generation == parent.generation) {
			*out = entities[i];
			return true;
		}
	}
	return false;
}

// The intensity a fade shows: peak × left / seconds, dark with no seconds.
static float faded(const tank_light_fade *fade)
{
	VOE_BASE_ASSERT(fade != NULL, "fading no light");
	const float intensity =
		fade->seconds > 0.0f ? fade->peak * fade->left / fade->seconds
				     : 0.0f;

	VOE_BASE_DEBUG_ASSERT(fade->seconds > 0.0f || intensity == 0.0f,
			      "a light lit by a fade of no seconds");
	return intensity;
}

// Replaces the entity's light with one at `intensity`; nothing when it has no
// light or already shines so. A full queue loses this step's level.
static void set_intensity(voe_ecs_world *world, voe_ecs_entity entity,
			  float intensity)
{
	VOE_BASE_ASSERT(world != NULL, "setting a light in no world");
	const voe_scene_point_light *light =
		voe_scene_point_light_get(world, entity);

	if (light == NULL || light->intensity == intensity)
		return;
	voe_scene_point_light next = *light;

	next.intensity = intensity;
	(void)voe_scene_point_light_submit(
		world, (voe_scene_point_light_intent){ .entity = entity,
							.light = next });
}

void tank_light_fade_system_run(const voe_game_project_step *step)
{
	VOE_BASE_ASSERT(step != NULL && step->world != NULL,
			"fading lights in no world");
	VOE_BASE_ASSERT(step->seconds >= 0.0, "fading lights back in time");
	const voe_ecs_type type =
		voe_ecs_component_type(step->world, &tank_light_fade_key);
	const tank_light_fade *rows = voe_ecs_component_rows(step->world, type);
	const voe_ecs_entity *entities =
		voe_ecs_component_entities(step->world, type);
	const uint32_t count = voe_ecs_component_count(step->world, type);

	VOE_BASE_ASSERT(count <= VOE_GAME_WORLD_POINT_LIGHTS,
			"more tank light fade rows than were registered");
	for (uint32_t i = 0; i < count; i++) {
		tank_light_fade next = rows[i];

		set_intensity(step->world, entities[i], faded(&next));
		next.left = fmaxf(0.0f, next.left - (float)step->seconds);
		const bool ok = voe_ecs_component_set(step->world, type,
						      entities[i], &next);

		VOE_BASE_ASSERT(ok,
				"a tank light fade row vanished while stepping it");
	}
}
