// Where a sound is heard from: a listener made from the camera, and the left
// and right gains a point in the world gets from it (ADR-0304 point 5).
//
//     const voe_audio_listener ear = voe_audio_listener_make(
//             camera_position, camera_right, camera_forward, tan_half_width);
//     const voe_audio_gains g = voe_audio_place(&ear, hit_position);
//
// THE LOUDNESS FALLS OFF FROM A REFERENCE: min(1, (reference / distance)^2).
// The reference is the distance along the view axis to the ground plane y = 0
// when the axis reaches it ahead, else VOE_AUDIO_REFERENCE. Taking it from the
// view keeps a high top-down camera from making everything quiet: the centre
// of the screen is full, its edges quieter, what is off screen quieter still.
//
// PAN IS SCREEN X, NOT ANGLE. A point in front pans by where it falls across
// the screen, dot(d, right) / (dot(d, forward) * tan_half_width), clamped to
// ±1, so the edges of the screen are the edges of the stereo field. A point
// not in front pans to the sign of its side.
//
// THE BALANCE LAW: left min(1, 1 - pan), right min(1, 1 + pan). Centred is 1
// and 1, so an unplaced sound (the coin) keeps the loudness it always had;
// constant-power panning would have made it quieter.
//
// THE DIFFERENCE IS TAKEN IN DOUBLE AND TURNED TO FLOAT, as the render view
// does (ADR-0250): positions far out stay exact, the offset is small.
//
// A pure value computation: no state, no allocation. right and forward are
// the caller's unit vectors and are not normalised here.
#pragma once

#include <math/double3.h>

#define VOE_AUDIO_REFERENCE 10.0f

typedef struct {
	voe_math_double3 position;
	voe_math_float3 right;
	voe_math_float3 forward;
	float tan_half_width;
	float reference;
} voe_audio_listener;

typedef struct {
	float left;
	float right;
} voe_audio_gains;

// A listener at position looking along forward, its reference found as above.
voe_audio_listener voe_audio_listener_make(voe_math_double3 position,
					   voe_math_float3 right,
					   voe_math_float3 forward,
					   float tan_half_width);

// The gains of a point heard by listener; a point at the listener is full and
// centred.
voe_audio_gains voe_audio_place(const voe_audio_listener *listener,
				voe_math_double3 where);

// 1 and 1: a sound with no place.
voe_audio_gains voe_audio_unplaced(void);
