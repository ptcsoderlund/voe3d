// The recoverable-failure codes. One enum for the whole engine, so that a
// caller who wants to say what went wrong has one vocabulary to learn and one
// place to look it up.
//
// A failure the world caused is returned; a failure the program caused is an
// assert. This header is the first half of that sentence — see base/assert.h
// for the second, and for why running out of memory is not on this page.
//
// TWO SHAPES, AND THE NUMBER OF WAYS TO FAIL PICKS ONE. A function that can
// fail exactly one way returns NULL or false and says so in its header; there is
// no code to hand back and no out-parameter for it. A function that can fail in
// ways a caller could tell apart returns NULL and takes a voe_base_error out
// beside it:
//
//     voe_base_error error;
//     voe_render_device *device = voe_render_device_new(native, size, &error);
//     if (device == NULL)
//             fprintf(stderr, "%s\n", voe_base_error_string(error));
//
// error may be NULL when the caller has decided it does not need to know which
// way. Where there is nothing to return, the enum is the return value, and
// VOE_BASE_OK is zero so that `if (error)` reads the right way round.
//
// THE CODES ARE CATEGORIES, NOT INCIDENTS. There is no code for "the swapchain
// would not be created" and there never will be. The category is what a caller
// can act on — reinstall a driver, buy a graphics card, try again later — and it
// is the only thing this enum promises to carry. What exactly happened is a
// message written at the site of the failure, where the detail still exists.
//
// A code is added when a card needs one, never in advance.
#pragma once

typedef enum {
	VOE_BASE_OK = 0,

	// Something the machine was supposed to provide is not on it. No driver,
	// no shared library, no compositor. Nothing the program does will change
	// that; a person has to install something.
	VOE_BASE_ERROR_UNAVAILABLE,

	// It is there, and it cannot do what the engine requires. A graphics
	// card too old for the version we ask for is this and not the one above:
	// the thing exists, it is merely not enough.
	VOE_BASE_ERROR_UNSUPPORTED,

	// It is there, it is capable, and the call failed anyway. Out of driver
	// memory, a device that went away, a handle refused for a reason the
	// caller cannot see. Retrying is sometimes reasonable; that is the
	// caller's call.
	VOE_BASE_ERROR_REFUSED,
} voe_base_error;

// One short phrase per code, never NULL, never allocated. It names the category
// only — the detail was printed at the site — so it is a line to put in front of
// a person, not a diagnosis.
const char *voe_base_error_string(voe_base_error error);
