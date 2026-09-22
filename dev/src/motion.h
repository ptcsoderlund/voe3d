// How the eye and the sun move each frame: the keys flying reads, where the
// orbit is at a given second, and where the sun is pointing at one. Three pure
// functions of the loop's clock and the keyboard, each returning what main.c
// submits.
//
// ITS OWN FILE BECAUSE IT IS THE SCENE'S MOTION AND NOT AN EXHIBIT. Nothing
// here builds anything or keeps state between frames; the camera path and the
// sun's path live here only until the folder that owns them exists, and the
// turning cube's spin stayed in main.c because it is one line beside the cube.
//
// main.c CALLS ONE OF THE FIRST TWO AND THEN THE THIRD, EVERY FRAME, BEFORE THE
// CAMERA SYSTEM RUNS: the motion while the camera is flown, the placement while
// it orbits, never both — main.c says at length why the handover is seamless.
#pragma once

#include <ecs/world.h>
#include <platform/window.h>
#include <scene/camera_system.h>
#include <scene/light_system.h>

// What the keyboard and the mouse are asking of the camera this frame, over a
// step of `seconds`. See motion.c for which key means what and what is wrong if
// flying feels wrong.
voe_scene_camera_motion voe_dev_camera_motion(voe_platform_window *window,
					      voe_ecs_entity eye,
					      float seconds);

// Where the orbit is at this many seconds in, as a placement: an eye and the two
// angles that look at the origin from it.
voe_scene_camera_placement voe_dev_orbit(voe_ecs_entity eye, float seconds);

// Where the sun is pointing at this many seconds in, as a light intent.
voe_scene_light_intent voe_dev_sunlight(voe_ecs_entity sun, float seconds);
