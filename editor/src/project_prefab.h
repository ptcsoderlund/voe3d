// The open prefab's save, private to project.c and project_prefab.c: project.h
// says what voe_editor_project_save does with a prefab open, and this is the
// call it hands that case to. Nothing else calls it.
#pragma once

#include "disk_watch.h"
#include "notice.h"
#include "project.h"

#include <stdbool.h>

// The open prefab's world written to `<folder>/<prefab>`: refused unless it is
// one tree whose root voe_editor_prefab_refused lets through (0283 point 1).
// True clears unsaved and tells `watch` (NULL nothing) the file; false says
// why, nothing written. Needs a folder and a prefab open.
[[nodiscard]] bool voe_editor_project_prefab_save(voe_editor_project *project,
						  voe_editor_disk_watch *watch,
						  voe_editor_notice *why);
