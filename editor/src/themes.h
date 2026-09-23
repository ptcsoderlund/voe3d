// The themes the editor has and the one in force (ADR-0170, ADR-0172,
// ADR-0178). The list's first two entries have no file: entry 0 is `Near
// black` — ui's default inputs, Oxanium — and entry 1 is `Near white` — the
// same inputs in the light mode. After them comes one entry per `*.theme` file
// in `<settings>/voe3d/themes/`, in the order the folder lists them. The one
// chosen is remembered in `<settings>/voe3d/theme` as one line: empty or absent
// for Near black, `near_white` for Near white — which no `*.theme` file can be
// named — and otherwise a file's name.
//
//     voe_editor_themes themes = { 0 };
//     if (!voe_editor_themes_load(&themes, font))
//             voe_editor_notice_from_report(&notice, themes.remembered);
//     voe_ui_theme_set(ui, &voe_editor_themes_chosen(&themes)->palette);
//     ... every frame:
//     if (voe_editor_themes_check(&themes) == VOE_EDITOR_THEMES_REFUSED)
//             voe_editor_notice_from_report(&notice, themes.remembered);
//     ...
//     voe_editor_themes_destroy(&themes);
//
// EVERY THEME READ FROM A FILE HAS AN ARENA OF ITS OWN. A file that refuses is
// dropped by destroying the one arena its bytes and strings went into, and a
// theme read again later can be dropped the same way, without rewinding past —
// or touching — any other theme's memory. The list itself, and the remembered
// file name, are in a further arena the list owns.
//
// A FILE THAT CANNOT BE READ IS LEFT OUT AND NOTHING ELSE STOPS. Its reason is
// already on stderr (platform/file.h, theme/theme.h), its arena is gone, and
// the rest of the folder is still read. What such a failure leaves behind is a
// shorter list and nothing more; the two themes with no file are always
// there, so the list is never empty.
//
// EVERY PALETTE IS DERIVED WITH THE ONE FONT, Oxanium, whatever typeface a
// theme names (ADR-0185).
//
// WHAT A PERSON SET IS REMEMBERED PER THEME, NOT IN THE THEME FILE
// (ADR-0197, ADR-0224). Every entry carries the two scalars and the text
// scale it is drawn with, and its palette is derived with those: the pair
// instead of the one in `theme.inputs`, and a text size of the theme's own
// `text_size` times the scale. They are the theme's own two and 1.0 until a
// line in theme_scalars.h's file, or a call to voe_editor_themes_adjust,
// replaces them. A remembered line replaces them at load and at every live
// re-read, so an edit to the file changes everything but what a person has
// set by hand; voe_editor_themes_reset puts the file's own two and its own
// `text_size` (scale 1.0) back and drops its line.
// THE THEME FILE IS NEVER WRITTEN — Near black and Near white have none, and
// what one person adjusts is not an edit to a document anyone may share.
//
// Constraints. The font is the caller's and must outlive every palette
// derived with it. A palette is kept by pointer by ui (ui/widgets.h), so the
// list must outlive the interface context it is set on. The folder is listed
// once, at load: a file added later appears after a restart, and only the
// chosen file is ever re-read.
#pragma once

#include "theme_scalars.h"

#include <base/arena.h>
#include <text/font.h>
#include <theme/theme.h>
#include <ui/theme.h>

#include <stddef.h>
#include <stdint.h>

// One theme the editor can draw in.
typedef struct {
	// What a person reads: the file's section name, `Near black` or
	// `Near white`.
	const char *name;
	// The file inside the themes folder, or NULL for a theme with no file.
	const char *file;
	// Which theme this is in theme_scalars.h's file: the file's name, or
	// `near_black` and `near_white` for entries 0 and 1 (ADR-0197).
	const char *identity;
	voe_theme theme;
	// The two scalars in force: the theme's own, from `theme.inputs`,
	// until a remembered line or a slider replaces them.
	float contrast_strength;
	float surface_separation;
	// What the theme's own `text_size` is multiplied by: 1.0 until a
	// remembered line or a slider replaces it.
	float text_scale;
	// Derived with the list's one font, whatever `theme.typeface` names,
	// and with the two scalars and the text scale above.
	voe_ui_theme palette;
	// The file's bytes it was read from, in `arena`; NULL and 0 for a
	// theme with no file.
	const uint8_t *bytes;
	size_t size;
	// This theme's own memory, or NULL for a theme with no file.
	voe_base_arena *arena;
} voe_editor_theme;

// What voe_editor_themes_check found.
typedef enum {
	// Not a second since the last look, a theme with no file in force,
	// the same bytes, or no file there right now.
	VOE_EDITOR_THEMES_UNCHANGED,
	// The chosen entry's palette was replaced by the file's new one.
	VOE_EDITOR_THEMES_CHANGED,
	// The file's new bytes will not read; the last good palette stands.
	VOE_EDITOR_THEMES_REFUSED,
} voe_editor_themes_check_result;

typedef struct {
	voe_editor_theme *entries;
	uint32_t count;
	// Index into entries of the theme in force.
	uint32_t chosen;
	// The file name `<settings>/voe3d/theme` holds, or NULL when it holds
	// none or `near_white` — kept even when it could not be loaded, so it
	// can be named.
	const char *remembered;
	// The themes folder's path, or NULL when there is no settings folder.
	const char *folder;
	// The one font every palette is derived with, kept for re-reading.
	const voe_text_font *font;
	// voe_platform_clock_now at the last look at the chosen file.
	double checked;
	// The chosen file's last refused bytes, in their own arena, or NULL
	// when the last bytes seen were good.
	voe_base_arena *refused;
	const uint8_t *refused_bytes;
	size_t refused_size;
	// What a person set, per theme, read at load and written back by
	// voe_editor_themes_scalars_write. Its identities live in `arena`.
	voe_editor_theme_scalars scalars;
	// An adjust or a reset the file has not been written with yet.
	bool scalars_unwritten;
	// The list's own memory: entries, `remembered`, `folder` and the
	// remembered scalars.
	voe_base_arena *arena;
} voe_editor_themes;

// voe_editor_themes_load ANSWERS FALSE ONLY FOR THE REMEMBERED THEME. A
// remembered file that is gone or refused falls back to Near black, and the
// false return is how the caller learns it should say so. The load clears
// base/report.h's kept error itself, so that afterwards the first kept error is
// that file's own refusal — or nothing, for a file that is gone — and a caller
// hands `remembered` to voe_editor_notice_from_report to name it. A folder
// that cannot be listed or made, or no settings folder at all, is not a
// failure: the list is the two themes with no file alone.
//
// Fills an empty `themes` from the themes folder and the remembered choice,
// making the folder when it is missing. False when a remembered file is gone
// or refused, with Near black chosen instead.
[[nodiscard]] bool voe_editor_themes_load(voe_editor_themes *themes,
					  const voe_text_font *font);

// The theme in force.
const voe_editor_theme *voe_editor_themes_chosen(const voe_editor_themes *themes);

// voe_editor_themes_choose WRITES THE REMEMBERED FILE the way last_project.h
// writes its own, making `<settings>` and `<settings>/voe3d` as needed.
//
// Puts entry `index` in force and remembers its file name, `near_white` for
// entry 1, or an empty line for entry 0. False when the remembered file could not be written —
// reported at the site by platform, or not at all when the machine has no
// settings folder — and the theme is in force either way.
[[nodiscard]] bool voe_editor_themes_choose(voe_editor_themes *themes,
					    uint32_t index);

// Puts contrast and separation in force on entry `index`, both clamped into
// VOE_UI_THEME_SCALAR_MIN..MAX, and the text scale, clamped into
// VOE_EDITOR_TEXT_SCALE_MIN..MAX, derives that entry's palette again where it
// stands — the address ui keeps does not move — and remembers all three for
// that theme, to be written by voe_editor_themes_scalars_write.
void voe_editor_themes_adjust(voe_editor_themes *themes, uint32_t index,
			      float contrast, float separation,
			      float text_scale);

// Puts entry `index`'s own two scalars and a text scale of 1.0 back, derives
// its palette again the same way, and drops its remembered line.
void voe_editor_themes_reset(voe_editor_themes *themes, uint32_t index);

// Writes theme_scalars.h's file when an adjust or a reset is unwritten.
// False only when that write failed, reported at the site; nothing to write
// is true.
[[nodiscard]] bool voe_editor_themes_scalars_write(voe_editor_themes *themes);

// voe_editor_themes_check IS LIVE EDITING (ADR-0172). Called every frame, it
// does something at most once a second by voe_platform_clock_now: the chosen
// theme's file — that file only, and nothing for a theme with no file — is
// read again and its bytes compared with the last ones seen. THE COMPARISON
// IS THE BYTES AND NOT A TIMESTAMP because a modification time lies across a copy or a
// checkout, and would be an API `platform` does not have, for a file of a few
// hundred bytes that costs nothing to read once a second. Changed bytes are
// read and derived into a fresh arena; on success the entry's palette is
// replaced where it stands — the address ui keeps is the same — and the old
// arena destroyed, and the answer is CHANGED so the caller sets it (and its
// font) on the context.
//
// A REFUSAL LEAVES THE DRAWING PALETTE AND ITS ARENA UNTOUCHED. The entry
// still holds the last good theme; the refused bytes are kept in an arena of
// their own only so the same mistake is not read, reported and answered again
// every second — the next check compares against them, and a save that
// differs from them is read again. The answer is REFUSED, once per distinct
// mistake, with base/report.h cleared first so its first kept error is the
// reader's line and what is wrong, and the caller names the file
// (voe_editor_notice_from_report). A file that is not there at a check is not
// a refusal — an editor saving by rename leaves that gap — and answers
// UNCHANGED. Choosing another theme forgets the refused bytes.
//
// Once a second, re-reads the chosen theme's file and replaces its palette
// when the bytes changed and read.
voe_editor_themes_check_result voe_editor_themes_check(voe_editor_themes *themes);

// Frees every theme's arena and the list's own. `themes` is empty afterwards.
void voe_editor_themes_destroy(voe_editor_themes *themes);
