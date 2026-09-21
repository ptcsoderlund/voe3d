// This frame's keyboard, read once: the level `platform` reports for every key
// and the down edge of each against last frame's. The loop reads it beside the
// pointer and the wheel, and everything that acts on a key reads it out of the
// frame it was handed.
//
// EVERY KEY EDGE IN THIS PROGRAM IS FOUND HERE AND NOWHERE ELSE. One key going
// down is one edge, and it is the same edge for every caller that asks, so no
// two of them can disagree about the frame a key went down on.
//
// AN EDGE IS TRACKED WHETHER OR NOT THE CALLER MAY ACT ON IT. Every key is read
// every frame, browser or no browser, typing or not, so a shortcut held through
// the browser opening and closing does not fire the moment it is let through:
// the edge it would have fired on passed while it was not the caller's to take.
//
// A KEY IS A PLACE AND NEVER A LETTER (platform/input.h). The enum names where
// on the keyboard a key sits, so the key this program calls Z is wherever Z
// sits on the keyboard in front of the person, whatever is printed on it.
//
// WHAT AN EDGE MEANS STAYS THE CALLER'S. Which shortcut it is, which guard
// silences it and whether a modifier is folded into it are all decided where it
// is acted on, because the guards differ per shortcut: Delete is quiet while a
// field holds the keyboard, Escape is the browser's Cancel while it shows, and
// Ctrl+N is nobody's while either does. This file answers two questions — is it
// down, did it go down this frame — and asks none.
#pragma once

#include <platform/input.h>
#include <platform/window.h>

#include <stdbool.h>

// What the keyboard remembers between frames: last frame's level, per key.
typedef struct {
	bool was_down[VOE_PLATFORM_KEY_COUNT];
} voe_editor_keys;

// One frame's keyboard. `down` is the level `platform` reports; `pressed` is
// the down edge against last frame's, and is true for one frame per press.
typedef struct {
	bool down[VOE_PLATFORM_KEY_COUNT];
	bool pressed[VOE_PLATFORM_KEY_COUNT];
} voe_editor_keys_frame;

// Reads every key once, after the poll, and fills `*out`. A NULL window — a
// capture, which has none — leaves every level and every edge false and still
// clears what was remembered, so no key is ever left standing down.
void voe_editor_keys_read(voe_editor_keys *keys, voe_platform_window *window,
			  voe_editor_keys_frame *out);
