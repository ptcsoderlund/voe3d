// The Inspector's sections and foliage kinds a person folded, remembered in
// `<settings>/voe3d/editor_settings`, and this frame's fold buttons, read after
// the frame to flip them.
//
// A FOLD IS NAMED WITHOUT AN ENTITY (0502, 0503 point 5): `section <word>` for
// a section, the word a described component's key or `sculpt`, `layers`,
// `foliage`, `brush` or `lod`, and `kind <k> <path>` for a foliage kind's box,
// `k` its index in the landscape and `path` the landscape's project-relative
// path as the model row names it. Folding a section folds it on every entity
// that shows it, in every project.
//
// ONLY FOLDED THINGS ARE WRITTEN, one `fold <name>` line each; no line means
// open, so the file does not grow with every section ever shown. These lines
// are this file's, kept beside every other key's by settings.h's shared write.
//
// THE FILE IS WRITTEN WHEN A FOLD FLIPS, once per frame however many flipped:
// a flip is rare, and writing at once survives a crash. A write that fails is
// reported at the site and the fold still holds for this run.
//
// FOLDS ARE THE PERSON'S, NOT THE PROJECT'S: never undone, never unsaved and
// never in a project, so every project opens with them as they were left.
//
// Constraints: at most VOE_EDITOR_FOLDS folded names; a fold past that, or a
// line too long or repeated, is dropped and nothing is reported, the thing
// then reading as open. At most VOE_EDITOR_FOLDS_SHOWN fold buttons a frame;
// a further section draws but its button is not recorded, so it does not fold.
// Lifting either is a bigger array. Lookups are a linear scan of the names.
#pragma once

#include <3d/model_component.h>

#include <base/arena.h>

#include <ui/layout.h>
#include <ui/widgets.h>

#include <stddef.h>
#include <stdint.h>

// How many folded names are kept, and the room for one: `kind 15 ` and a
// model path, its terminator included.
#define VOE_EDITOR_FOLDS 128u
#define VOE_EDITOR_FOLD_NAME (sizeof "kind 15 " - 1u + VOE_3D_MODEL_PATH)

// How many fold buttons one frame records.
#define VOE_EDITOR_FOLDS_SHOWN 64u

// The folded names, loaded at start, and this frame's fold buttons: each a
// node and its name, copied into the frame's arena. Mutated only here.
typedef struct {
	char folded[VOE_EDITOR_FOLDS][VOE_EDITOR_FOLD_NAME];
	uint32_t folded_count;
	voe_ui_node shown[VOE_EDITOR_FOLDS_SHOWN];
	const char *shown_name[VOE_EDITOR_FOLDS_SHOWN];
	uint32_t shown_count;
	// This frame's arena, where the shown names live; NULL before the
	// first frame.
	voe_base_arena *frame;
} voe_editor_folds;

// Empties the store, then takes every `fold <name>` line in editor_settings.
void voe_editor_folds_load(voe_editor_folds *folds);

// Whether the thing `name` names is open: true unless it has a line.
bool voe_editor_folds_open(const voe_editor_folds *folds, const char *name);

// Empties this frame's buttons and takes the frame's arena. Called at frame
// open, before anything draws a section.
void voe_editor_folds_forget(voe_editor_folds *folds, voe_base_arena *frame);

// Opens a ui section keyed `key` and `index`, open as `name`'s line says, and
// records its fold button under `name`. Whether it is open. The header row is
// left open for the caller's title; the caller closes it and the box.
bool voe_editor_folds_begin(voe_ui_context *ui, voe_editor_folds *folds,
			    const char *name, const char *key, uint32_t index);

// After voe_ui_frame_end: each fired fold button adds its name or drops it,
// and on any change the file is written once.
void voe_editor_folds_read(const voe_ui_context *ui, voe_editor_folds *folds);

// Drops `name`, writing the file only if it was there: a new foliage kind
// starts open (0502 point 2).
void voe_editor_folds_clear(voe_editor_folds *folds, const char *name);

// `section <word>` into `out`, which has `room` bytes.
void voe_editor_folds_section_name(char *out, size_t room, const char *word);

// `kind <k> <landscape>` into `out`, which has `room` bytes.
void voe_editor_folds_kind_name(char *out, size_t room, const char *landscape,
				uint32_t kind);
