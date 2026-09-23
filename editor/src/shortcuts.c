// The one read of what this frame's keyboard asked for. See the header for why
// which edge means which command is answered once, why a guard is handed in
// rather than asked for, why the rest a step is recorded at is worked out here
// too, and why acting on any of it is the caller's.
#include "shortcuts.h"

#include <base/assert.h>

#include <stddef.h>

voe_editor_shortcuts voe_editor_shortcuts_read(const voe_editor_keys_frame *keys,
					       voe_editor_shortcuts_guards guards)
{
	VOE_BASE_ASSERT(keys != NULL, "reading shortcuts out of no keyboard");

	voe_editor_shortcuts out = { 0 };
	const bool shift = keys->down[VOE_PLATFORM_KEY_SHIFT];
	const bool control = keys->down[VOE_PLATFORM_KEY_CONTROL];
	// Neither a command nor a step while the browser shows or while a field
	// or number box holds the keyboard — a person typing is not also
	// commanding (ui/widgets.h's voe_ui_typing) — nor while a view flies,
	// whose W, S, A, D, E and Q are the fly's (view.h).
	const bool quiet = guards.browser_showing || guards.typing ||
			   guards.flying;

	// CTRL+N, CTRL+O AND CTRL+S DO WHAT THEIR BUTTON DOES, on the frame the
	// letter goes down with Control already held — the letter's own edge,
	// and Control the level read beside it. A capture has no window, so
	// `control` stays false and none of these ever reads true.
	//
	// AND NONE OF THE THREE FIRES WHILE THE BROWSER SHOWS — "top bar
	// commands and shortcuts are ignored" (browser.h) — though keys.h
	// tracks the edge either way.
	out.new_project = control && keys->pressed[VOE_PLATFORM_KEY_N] &&
			  !guards.browser_showing && !guards.flying;
	out.open = control && keys->pressed[VOE_PLATFORM_KEY_O] &&
		   !guards.browser_showing && !guards.flying;
	out.save = control && keys->pressed[VOE_PLATFORM_KEY_S] &&
		   !guards.browser_showing && !guards.flying;

	// DELETE AND CTRL+D ARE THE SAME SHAPE OF EDGE, quiet while anybody is
	// typing or the browser shows.
	out.delete_entity = keys->pressed[VOE_PLATFORM_KEY_DELETE] && !quiet;
	out.duplicate = control && keys->pressed[VOE_PLATFORM_KEY_D] && !quiet;

	// CTRL+Z AND CTRL+SHIFT+Z OR CTRL+Y ARE THE SAME SHAPE OF EDGE AGAIN,
	// and beyond `quiet` they ask for the rest a step is recorded at
	// (undo.h): no drag, no picker and no open list in the middle of
	// something.
	out.at_rest = !guards.pointer_down && !guards.flying && !guards.typing &&
		      !guards.browser_showing && !guards.picker_open &&
		      !guards.dropdown_open;
	out.undo = control && !shift && keys->pressed[VOE_PLATFORM_KEY_Z] &&
		   !quiet && out.at_rest;
	out.redo = control &&
		   ((shift && keys->pressed[VOE_PLATFORM_KEY_Z]) ||
		    keys->pressed[VOE_PLATFORM_KEY_Y]) &&
		   !quiet && out.at_rest;

	// THIS FRAME'S ESCAPE EDGE, which the caller hands to the interface as
	// `ui`'s keyboard for whichever field or number box is focused.
	out.escape = keys->pressed[VOE_PLATFORM_KEY_ESCAPE];
	// WHOEVER IS TYPING KEEPS ESCAPE — voe_ui_typing says a field or number
	// box held the keyboard at the last frame's end — so the edge anybody
	// else may spend is the one nobody is typing through.
	out.escape_free = out.escape && !guards.typing;

	VOE_BASE_ASSERT(!guards.flying || (!out.at_rest && !out.save &&
					   !out.undo && !out.delete_entity),
			"a flying view let a command through");
	VOE_BASE_ASSERT(!out.escape_free || out.escape,
			"a free Escape with no Escape edge behind it");
	return out;
}
