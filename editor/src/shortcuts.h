// What this frame's keyboard asked the editor to do, worked out once out of
// keys.h's frame and the guards the caller is holding. The loop reads it
// straight after the keyboard and acts on the flags it gets back.
//
// WHICH KEY EDGE MEANS WHICH COMMAND IS ONE ANSWER GIVEN ONCE. Ctrl+N is New,
// Delete is Delete, R switches the gizmo between moving and turning,
// Ctrl+Shift+Z and Ctrl+Y are both Redo, and each is decided
// here and nowhere else, so no two callers can disagree about what a person
// pressed. keys.h answers whether a key went down; this file answers what that
// edge was for.
//
// A GUARD IS A FACT THE CALLER HOLDS AND NEVER SOMETHING THIS FILE ASKS A PANEL
// FOR. Whether the browser shows, whether a field holds the keyboard, whether
// the colour picker or a dropdown is open and whether the primary button is
// down are all read by the caller, out of the things it owns, and handed in;
// this file names no panel and asks none of them anything. That is what keeps
// it a sum over its arguments: the same frame and the same guards give the same
// answer, whatever else is on screen.
//
// THE SHORTCUT FLAGS AND THE REST A STEP IS RECORDED AT ARE WORKED OUT TOGETHER
// because they are the same question asked twice. Ctrl+Z and Ctrl+Y may only
// fire while the editor is at rest — no drag, no typing, no browser, no picker,
// no open list — and that rest is also what undo.h settles a step at, so the
// caller gets `at_rest` out of the same read rather than building it a second
// time and drifting from the one the shortcuts were silenced by.
//
// ACTING ON A FLAG IS THE CALLER'S. Carrying out a command, closing the picker,
// hiding Preferences and the order Escape goes through them in are decisions of
// the call site, and this file may name neither `session.h` nor
// `preferences.h`: it reads the keyboard and answers, and changes nothing.
#pragma once

#include "keys.h"

#include <stdbool.h>

// The facts the caller holds while the keyboard is read. Each is named for
// where the caller reads it: browser.showing, voe_ui_typing(ui),
// scene.picking.open, scene.dropdown.open, the primary button's level, and what
// voe_editor_views_fly returned (view.h): a flying view keeps every key it reads,
// so then no command fires and the editor is not at rest.
typedef struct {
	bool browser_showing;
	bool typing;
	bool picker_open;
	bool dropdown_open;
	bool pointer_down;
	bool flying;
} voe_editor_shortcuts_guards;

// One frame's answer. Every flag is an edge, true for one frame per press.
typedef struct {
	bool new_project, open, save;
	bool delete_entity, duplicate;
	// R alone: the gizmo's arrows become rings or back (scene.h).
	bool gizmo_switch;
	bool undo, redo;
	// No drag, no fly, no typing, no browser, no picker, no open list.
	bool at_rest;
	// The raw edge, handed to `ui` as this frame's keyboard, and that edge
	// when nobody is typing.
	bool escape;
	bool escape_free;
} voe_editor_shortcuts;

// Reads `*keys` against `guards` and answers what was asked for. A capture's
// keyboard has nothing down and no edge on it, so every flag but `at_rest` is
// false there (keys.h).
voe_editor_shortcuts voe_editor_shortcuts_read(const voe_editor_keys_frame *keys,
					       voe_editor_shortcuts_guards guards);
