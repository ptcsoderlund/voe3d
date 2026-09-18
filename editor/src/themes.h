// The themes the editor has and the one in force (ADR-0170, ADR-0172). The
// list's first entry is the built-in theme — ui's default inputs, Oxanium, no
// file, shown as `Built-in` — and after it comes one entry per `*.theme` file in
// `<settings>/voe3d/themes/`, in the order the folder lists them. The one
// chosen is remembered by its file name in `<settings>/voe3d/theme`, one line,
// empty or absent meaning the built-in.
//
//     voe_editor_themes themes = { 0 };
//     if (!voe_editor_themes_load(&themes, oxanium, pixel_operator))
//             voe_editor_notice_from_report(&notice, themes.remembered);
//     voe_ui_theme_set(ui, &voe_editor_themes_chosen(&themes)->palette);
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
// shorter list and nothing more; the built-in entry is always there, so the
// list is never empty.
//
// voe_editor_themes_load ANSWERS FALSE ONLY FOR THE REMEMBERED THEME. A
// remembered file that is gone or refused falls back to the built-in, and the
// false return is how the caller learns it should say so. The load clears
// base/report.h's kept error itself, so that afterwards the first kept error is
// that file's own refusal — or nothing, for a file that is gone — and a caller
// hands `remembered` to voe_editor_notice_from_report to name it. A folder
// that cannot be listed or made, or no settings folder at all, is not a
// failure: the list is the built-in theme alone.
//
// voe_editor_themes_choose WRITES THE REMEMBERED FILE the way last_project.h
// writes its own, making `<settings>` and `<settings>/voe3d` as needed.
//
// Constraints. The fonts are the caller's and must outlive every palette
// derived with them. A palette is kept by pointer by ui (ui/widgets.h), so the
// list must outlive the interface context it is set on. The folder is read
// once, at load; nothing here re-reads a file.
#pragma once

#include <base/arena.h>
#include <text/font.h>
#include <theme/theme.h>
#include <ui/theme.h>

#include <stdint.h>

// One theme the editor can draw in.
typedef struct {
	// What a person reads: the file's section name, or `Built-in`.
	const char *name;
	// The file inside the themes folder, or NULL for the built-in theme.
	const char *file;
	voe_theme theme;
	// Derived with whichever of the two fonts `theme.typeface` names.
	voe_ui_theme palette;
	// This theme's own memory, or NULL for the built-in theme.
	voe_base_arena *arena;
} voe_editor_theme;

typedef struct {
	voe_editor_theme *entries;
	uint32_t count;
	// Index into entries of the theme in force.
	uint32_t chosen;
	// The file name `<settings>/voe3d/theme` holds, or NULL when it holds
	// none — kept even when it could not be loaded, so it can be named.
	const char *remembered;
	// The list's own memory: entries and `remembered`.
	voe_base_arena *arena;
} voe_editor_themes;

// Fills an empty `themes` from the themes folder and the remembered choice,
// making the folder when it is missing. False when a remembered file is gone
// or refused, with the built-in chosen instead; see this file's header.
[[nodiscard]] bool voe_editor_themes_load(voe_editor_themes *themes,
					  const voe_text_font *oxanium,
					  const voe_text_font *pixel_operator);

// The theme in force.
const voe_editor_theme *voe_editor_themes_chosen(const voe_editor_themes *themes);

// Puts entry `index` in force and remembers its file name, or an empty line
// for the built-in. False when the remembered file could not be written —
// reported at the site by platform, or not at all when the machine has no
// settings folder — and the theme is in force either way.
[[nodiscard]] bool voe_editor_themes_choose(voe_editor_themes *themes,
					    uint32_t index);

// Frees every theme's arena and the list's own. `themes` is empty afterwards.
void voe_editor_themes_destroy(voe_editor_themes *themes);
