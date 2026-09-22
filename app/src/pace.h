// How long voe_app_frame_open waits before it opens a frame, as a pure decision:
// the window's two answers and two clock readings in, seconds out. Internal to
// app; app.c asks it once per frame and app/tests/pace.c drives it with numbers,
// so a test needs no window (ADR-0215).
//
// Focused: 0. The present (FIFO) paces the frame and nothing else should.
// Not focused but visible: what is left of VOE_APP_HEARTBEAT_SECONDS since the
// previous frame opened, never below 0 — about four frames a second.
// Hidden: negative, which voe_platform_window_wait reads as no timeout.
//
// THE WAIT ENDS EARLY ON ANY EVENT, and the caller relies on it: the heartbeat
// is a ceiling on how stale the picture is, not a sleep. A click back into the
// window or a close request is an event, so regaining focus draws at once and
// closing is never held up by a quarter second.
//
// HIDDEN IS NO TIMEOUT, NOT A LONG HEARTBEAT. Nothing drawn while hidden can be
// seen, so any timeout is a wake-up that buys nothing; being shown again or
// asked to close is itself an event and ends the wait.
#pragma once

#include <stdbool.h>

#define VOE_APP_HEARTBEAT_SECONDS 0.25

double voe_app_pace_wait(bool focused, bool visible, double now,
			 double last_open);
