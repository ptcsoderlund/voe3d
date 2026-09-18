// Logical Wayland units to buffer pixels, at a fractional scale (ADR-0180).
// Included by window_wayland.c, which calls both functions on every size and
// every pointer position it hands out; built on both platforms because, like
// input.h, it includes no OS header, and that is what lets tests/scale.c check
// it without a compositor.
//
// scale IS IN 120THS, AS wp_fractional_scale_v1's preferred_scale SENDS IT:
// 120 is 1.0, 150 is 1.25, 180 is 1.5. It is taken as the protocol's integer
// rather than a float so that 120 is exactly the identity and a length never
// picks up a float's error on its way to a whole number of pixels.
//
// A LENGTH ROUNDS HALF AWAY FROM ZERO, which is what the protocol asks a client
// to do when sizing its buffer; a position is not rounded at all, for the reason
// include/platform/input.h gives for keeping the fraction.
#pragma once

#include <stdint.h>

int voe_platform_scale_length(int logical, uint32_t scale);
double voe_platform_scale_position(double logical, uint32_t scale);
