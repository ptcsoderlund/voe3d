// The open prefab's save and the untitled scene's light, private to the
// project files: project.h says what voe_editor_project_save does with a
// prefab open, and the save is the call it hands that case to; the light is
// the one project.c builds the untitled scene with, which the species view
// (project_species.c) stands in its world as its sun. Nothing else calls them.
#pragma once

#include "disk_watch.h"
#include "notice.h"
#include "project.h"

#include <ecs/world.h>

#include <stdbool.h>
#include <stdint.h>

// The open prefab's world written to `<folder>/<prefab>`: refused unless it is
// one tree whose root voe_editor_prefab_refused lets through (0283 point 1).
// True clears unsaved and tells `watch` (NULL nothing) the file; false says
// why, nothing written. Needs a folder and a prefab open.
[[nodiscard]] bool voe_editor_project_prefab_save(voe_editor_project *project,
						  voe_editor_disk_watch *watch,
						  voe_editor_notice *why);

// The untitled scene's `Light`, with id `id`, added straight into world: its
// transform turned to shine down from the front-right and its light row the
// untitled one. Asserts the world has room; that no authored entity holds
// `id` already is the caller's to see to.
void voe_editor_project_sun_add(voe_ecs_world *world, uint64_t id);
