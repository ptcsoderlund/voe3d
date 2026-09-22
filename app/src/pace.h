// One step of the pace voe_app_frame_open keeps before it opens a frame: the
// window's three answers and two clock readings in, draw-now or wait-this-long
// out. Internal to app; app.c runs it in a loop and app/tests/pace.c drives it
// with numbers, so a test needs no window (ADR-0215).
//
// THE LOOP IS THE HEARTBEAT, NOT THE WAIT (ADR-0216). voe_platform_window_wait
// may return the moment anything at all reaches the window's connection, and an
// unfocused Wayland window gets traffic every frame, so one wait sleeps for no
// time at all. The caller therefore asks again after each wait and its poll, and
// the clock — not the event — decides when the quarter second is over.
//
// Closing draws, from every state and before anything else is asked: a program
// asked to stop has a last frame to open and nothing to wait for, and a hidden
// window would otherwise wait with no timeout for an event that has already
// been and gone.
// Not visible waits with a negative seconds, which voe_platform_window_wait
// reads as no timeout. Nothing drawn while hidden can be seen, so any timeout is
// a wake-up that buys nothing.
// Focused draws: the present (FIFO) paces the frame and nothing else should.
// Otherwise it waits what is left of VOE_APP_HEARTBEAT_SECONDS since the
// previous frame opened — about four frames a second.
//
// FOCUS REGAINED, BEING SHOWN AND A CLOSE REQUEST END THE HEARTBEAT EARLY, and
// nothing here has to arrange it: the _poll after each wait folds them into the
// window's answers, and the next step reads the new ones and says draw.
//
// A REST BELOW THE FLOOR IS A DRAW. A timeout is truncated to whole
// milliseconds on its way to the compositor, so a rest under one would be no
// wait at all and the loop would spin out the remainder.
#pragma once

#include <stdbool.h>

#define VOE_APP_HEARTBEAT_SECONDS 0.25
#define VOE_APP_PACE_FLOOR_SECONDS 0.001

typedef enum { VOE_APP_PACE_DRAW, VOE_APP_PACE_WAIT } voe_app_pace_kind;

typedef struct {
	voe_app_pace_kind kind;
	double seconds;
} voe_app_pace_step;

voe_app_pace_step voe_app_pace_next(bool focused, bool visible, bool closing,
				    double now, double last_open);
