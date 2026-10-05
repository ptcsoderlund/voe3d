// The day and night system: a clock per row, and the light it turns. Night
// is a weight n in [0, 1] over the period: day for the first 0.3, a
// smoothstep up over the next 0.2, night for 0.3, and down over the last 0.2.
// The light's intensity is day and night mixed at n, and it casts
// day_shadows while n < 0.5, else night_shadows, so a sun and a moon swap
// their shadows at the middle of each dusk.
//
// The clock is the system's own row, written directly; the light is scene's,
// so the change is its intent carrying the whole row, submitted only when it
// differs from what the light already is. The intent lands at the light
// drain, so a step reads the light as the last drain left it.
//
// Constraints: a light refused by the drain (a negative day or night) keeps
// its last row and says so on stderr; a full light queue drops the rest of
// this step's changes.
#include "day_night.h"

#include <base/assert.h>

#include <ecs/component.h>

#include <game/project.h>
#include <game/world.h>

#include <scene/light_component.h>
#include <scene/light_system.h>

#include <math.h>

const struct voe_ecs_key day_night_key = { "day_night" };

static const day_night day_night_default = { .period = 20.0f };

bool day_night_register(voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "registering day night in no world");
	return voe_game_project_component(world, &(voe_game_project_type){
		&day_night_key, sizeof(day_night), VOE_GAME_WORLD_AUTHORED,
		VOE_GAME_PROJECT_DESCRIPTION(day_night), &day_night_default,
		"Day Night" });
}

static float smoothstep(float x)
{
	return x * x * (3.0f - 2.0f * x);
}

// How much night it is, 0 to 1, at `t` of the way through the period.
static float night_weight(float t)
{
	if (t < 0.3f)
		return 0.0f;
	if (t < 0.5f)
		return smoothstep((t - 0.3f) / 0.2f);
	if (t < 0.8f)
		return 1.0f;
	return 1.0f - smoothstep((t - 0.8f) / 0.2f);
}

void day_night_system_run(voe_ecs_world *world, double seconds)
{
	VOE_BASE_ASSERT(world != NULL, "turning day into night in no world");
	VOE_BASE_ASSERT(seconds >= 0.0, "turning day into night back in time");
	const voe_ecs_type type = voe_ecs_component_type(world, &day_night_key);
	const day_night *rows = voe_ecs_component_rows(world, type);
	const voe_ecs_entity *entities = voe_ecs_component_entities(world, type);
	const uint32_t count = voe_ecs_component_count(world, type);

	VOE_BASE_ASSERT(count <= VOE_GAME_WORLD_AUTHORED,
			"more day night rows than were registered");
	for (uint32_t i = 0; i < count; i++) {
		const voe_scene_light *light =
			voe_scene_light_get(world, entities[i]);

		if (light == NULL || !isfinite(rows[i].period) ||
		    rows[i].period <= 0.0f)
			continue;
		day_night row = rows[i];

		row.clock = (float)fmod((double)row.clock + seconds,
					(double)row.period);
		if (!(row.clock >= 0.0f))
			row.clock = 0.0f;
		const bool set = voe_ecs_component_set(world, type, entities[i],
						       &row);

		VOE_BASE_ASSERT(set, "a listed day night row is gone");
		const float n = night_weight(row.clock / row.period);
		voe_scene_light lit = *light;

		lit.intensity = row.day + (row.night - row.day) * n;
		lit.cast_shadows = n < 0.5f ? row.day_shadows : row.night_shadows;
		if (lit.intensity == light->intensity &&
		    lit.cast_shadows == light->cast_shadows)
			continue;
		if (!voe_scene_light_submit(world, (voe_scene_light_intent){
							   entities[i], lit }))
			return;
	}
}
