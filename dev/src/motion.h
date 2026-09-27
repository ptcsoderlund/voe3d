// How the eye and the sun move each frame: the keys flying reads, where the
// orbit is at a given second, the pose a flight is seen from, and the sun's pose
// at one. Pure functions of the loop's clock and the keyboard, each
// returning what main.c keeps or submits.
//
// ITS OWN FILE BECAUSE IT IS THE SCENE'S MOTION AND NOT AN EXHIBIT. Nothing
// here builds anything or keeps state between frames: the flight is main.c's,
// handed in and handed back. The camera path and the sun's path live here only
// until the folder that owns them exists, and the turning cube's spin stayed in
// main.c because it is one line beside the cube.
//
// THE CAMERA IS A LENS ON A TRANSFORMED ENTITY (0222), so the eye is moved like
// anything else: main.c flies or orbits its voe_dev_flight, turns it into a
// transform with voe_dev_flight_pose and submits that for the eye, every frame,
// before the transform system runs. One of fly and orbit, never both — main.c
// says why the handover is seamless.
#pragma once

#include <math/double3.h>
#include <math/float3.h>
#include <platform/window.h>
#include <scene/transform_component.h>

// Where the eye is and the two angles it looks along. Zero yaw and pitch look
// along -Z; a positive yaw turns towards -X and a positive pitch looks up.
typedef struct {
	voe_math_double3 eye;
	float yaw;
	float pitch;
} voe_dev_flight;

// `flight` moved by what the keyboard and the mouse ask this frame, over a step
// of `seconds`. See motion.c for which key means what and what is wrong if
// flying feels wrong.
voe_dev_flight voe_dev_fly(voe_platform_window *window, voe_dev_flight flight,
			   float seconds);

// Where the orbit is at this many seconds in: an eye and the two angles that
// look at the origin from it.
voe_dev_flight voe_dev_orbit(float seconds);

// The flight as a transform: at the eye, turned by the yaw about +Y and then by
// the pitch about the turned X, no roll, scale one (0223).
voe_scene_transform voe_dev_flight_pose(voe_dev_flight flight);

// The sun's pose at this many seconds in: turned to shine along its lap's
// direction (0273), standing above the origin, scale one.
voe_scene_transform voe_dev_sun_pose(float seconds);
