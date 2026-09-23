// The transforms that turn an exhibit to the camera: the heads-up line, the
// dark panel behind it and the readout, each a transform intent square to the
// camera and a fixed distance in front of it.
//
// ITS OWN FILE BECAUSE IT IS GEOMETRY AND NOT AN EXHIBIT. The three things it
// places are text.h's; this file only answers where they stand this frame,
// from where the camera is and how big the window is.
//
// main.c CALLS ALL THREE EVERY FRAME, BEFORE THE TRANSFORM SYSTEM, and submits
// what they return. Each is handed the eye's pose for this frame — the one
// main.c submits for the eye in the same gap — so nothing lags (0223).
#pragma once

#include <ecs/world.h>
#include <math/float2.h>
#include <platform/window.h>
#include <scene/camera_component.h>
#include <scene/transform_system.h>

// The heads-up line: in front of the eye, square to it, centred across it on
// its width `size.x`, a little below the middle.
voe_scene_transform_intent voe_dev_facing_the_camera(voe_scene_transform eye,
						     voe_ecs_entity text,
						     voe_math_float2 size);

// The readout: square to the camera in the top-left corner of a window `size`
// pixels big, seen through `lens`, left-aligned, for writing `em` metres to the
// em.
voe_scene_transform_intent voe_dev_top_left_of_the_view(
	voe_scene_transform eye, voe_scene_camera lens, voe_ecs_entity readout,
	voe_platform_size size, float em);

// The panel behind the heads-up line: a shade further than it, around the
// line's box of `size` and a margin larger.
voe_scene_transform_intent voe_dev_behind_the_line(voe_scene_transform eye,
						   voe_ecs_entity quad,
						   voe_math_float2 size);
