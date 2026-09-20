// A person's contrast and surface separation, remembered per theme and per
// person on the machine — `<settings>/voe3d/theme_scalars`, one line per
// adjusted theme (ADR-0197):
//
//     <contrast> <separation> <identity>
//
// THE IDENTITY IS THE TAIL OF THE LINE because a theme file's name may hold a
// blank and nothing here quotes anything: the two numbers are read first and
// whatever follows the blank run after the second one is the name, to the end
// of the line. An identity is `near_black` for Near black, `near_white` for
// Near white — neither can be a `*.theme` file's name (ADR-0178) — and
// otherwise a theme file's own name, the same identity `<settings>/voe3d/theme`
// remembers the chosen theme by (ADR-0172).
//
//     voe_editor_theme_scalars scalars;
//     voe_editor_theme_scalars_read(&scalars, arena);
//     const voe_editor_theme_scalars_line *line =
//             voe_editor_theme_scalars_find(&scalars, "near_white");
//     ... a drag ended:
//     voe_editor_theme_scalars_set(&scalars, arena, "near_white", 1.2f, 0.8f);
//     (void)voe_editor_theme_scalars_write(&scalars);
//
// A LINE THAT DOES NOT PARSE IS SKIPPED WITHOUT A REPORT, and so is one whose
// numbers fall outside VOE_UI_THEME_SCALAR_MIN..MAX. Nobody asked whether this
// file was well formed and nobody can act on a stale or hand-edited line; the
// rest of the file is still read. No file at all, and no settings folder at
// all, are an empty list and no report the same way.
//
// LINES FOR THEMES NOT LISTED RIGHT NOW ARE KEPT AND WRITTEN BACK. What is in
// the list is what the file held, and a write puts every line back — so a
// theme file that is momentarily gone, renamed or on another machine's disk
// keeps its numbers instead of losing them to the first save made without it.
//
// The read pushes the file's bytes into arena and every identity points into
// them, as an identity handed to _set points into the same arena; arena has to
// outlive the struct. Constraint: at most VOE_EDITOR_THEME_SCALARS_LINES lines
// are held, which is many more themes than a folder is listed with; past that
// a theme not already in the file is not remembered — _set appends nothing and
// the file keeps what it has. A growable list, owning an arena of its own,
// would lift it.
#pragma once

#include <base/arena.h>

#include <stdint.h>

// How many themes' numbers are held at once. See the constraint above.
#define VOE_EDITOR_THEME_SCALARS_LINES 64

// One line: which theme, and the two scalars it is drawn with.
typedef struct {
	const char *identity;
	float contrast_strength;
	float surface_separation;
} voe_editor_theme_scalars_line;

// The whole file, read into memory.
typedef struct {
	voe_editor_theme_scalars_line lines[VOE_EDITOR_THEME_SCALARS_LINES];
	uint32_t count;
} voe_editor_theme_scalars;

// Reads the file into scalars, pushing what it keeps into arena. An empty list
// when there is no settings folder, no file, or nothing in it that parses.
void voe_editor_theme_scalars_read(voe_editor_theme_scalars *scalars,
				   voe_base_arena *arena);

// The line remembered for identity, or NULL when there is none.
const voe_editor_theme_scalars_line *
voe_editor_theme_scalars_find(const voe_editor_theme_scalars *scalars,
			      const char *identity);

// Remembers contrast and separation for identity, replacing that theme's line
// or appending one, with identity copied into arena.
void voe_editor_theme_scalars_set(voe_editor_theme_scalars *scalars,
				  voe_base_arena *arena, const char *identity,
				  float contrast, float separation);

// Drops identity's line, if it has one. The other lines keep their order.
void voe_editor_theme_scalars_forget(voe_editor_theme_scalars *scalars,
				     const char *identity);

// Writes every line back, making `<settings>` and `<settings>/voe3d` first if
// either is missing. False on a failure at any step, reported through
// base/report.h at the site.
[[nodiscard]] bool
voe_editor_theme_scalars_write(const voe_editor_theme_scalars *scalars);
