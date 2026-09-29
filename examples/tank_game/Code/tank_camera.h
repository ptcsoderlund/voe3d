// The tank game's lens: keeps the level's width in view at any window shape
// (0291 point 5).
//
//     tank_camera_register(world);   // in voe_game_project_register
//     tank_camera_run(step);         // last before the move, each fixed step
//
// THE LEVEL'S WIDTH IS WHAT THE SCENE'S CAMERA FRAMES AT 16:9: the old
// 1280x720 window and the editor's 480x270 preview, the framing the sponsor
// built the level in. At 16:9 or wider the lens is the authored fov_y, so a
// wider window sees more water at the sides; narrower, fov_y widens so the
// horizontal field stays the one at 16:9, capped at 3.0 rad (the lens must
// stay under pi, and past the cap a very tall window loses the edges).
//
// tank_camera_fit is one runtime-only row on the scene's one camera (0261):
// the camera's authored fov_y, added by tank_camera_run's first step with a
// window through the structural queue, so the lens changes from the step
// after. Never saved, never in the Inspector, no menu; the sponsor's scene
// keeps its own lens (0272). ONE WRITER, THIS MODULE.
//
// Constraints: one row (capacity 1). Headless (no window), nothing changes.
// A window of no size, or a full camera queue, leaves the lens as it is.
#pragma once

#include <ecs/world.h>

#include <game/project.h>

#include <stdbool.h>

typedef struct {
	float fov_y;
} tank_camera_fit;

extern const struct voe_ecs_key tank_camera_fit_key;

// Registers tank_camera_fit runtime-only with no menu. False, reported, when
// game refuses it.
[[nodiscard]] bool tank_camera_register(voe_ecs_world *world);

// Adds the row on the first step with a window; after, submits the whole
// lens fitted to the window's aspect when it differs from the current one.
void tank_camera_run(const voe_game_project_step *step);
