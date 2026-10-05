// Day Night: a directional light's strength and shadows by day and by night.
// Teaches that game code changes a light the way the Inspector does, through
// its intent with the whole row (scene/light_system.h, 0349): the system
// reads the light, changes the intensity and cast_shadows it cares about and
// submits the result, so colour, fill and bounces stay as authored.
//
//     day_night_register(world);                // in voe_game_project_register
//     day_night_system_run(world, seconds);     // each fixed step
//
// `day` and `night` are the light's intensity by day and by night;
// `day_shadows` and `night_shadows` whether it casts shadows then. A sun is
// bright and casting by day and dark by night; a moon the other way, faint
// and shadowless by day. `period` is seconds for a whole day and night,
// default 20. `clock` is seconds into the period, read-only: the system
// writes it every step.
//
// Constraints: at most VOE_GAME_WORLD_AUTHORED rows. A row on an entity with
// no light, or with a period that is not a positive number, does nothing.
#pragma once

#include <base/describe.h>

#include <ecs/world.h>

#include <stdbool.h>

#define DAY_NIGHT_FIELDS(F, F_READ_ONLY) \
	F(float, day, FLOAT32)               \
	F(float, night, FLOAT32)             \
	F(bool, day_shadows, BOOL)           \
	F(bool, night_shadows, BOOL)         \
	F(float, period, FLOAT32)            \
	F_READ_ONLY(float, clock, FLOAT32)

VOE_BASE_DESCRIBE_STRUCT(day_night, DAY_NIGHT_FIELDS)

extern const struct voe_ecs_key day_night_key;

// Registers the type under "Day Night", its default period 20. False,
// reported, when game refuses it.
[[nodiscard]] bool day_night_register(voe_ecs_world *world);

// Moves every clock on by seconds and submits each light whose intensity or
// shadows the time of day changes.
void day_night_system_run(voe_ecs_world *world, double seconds);
