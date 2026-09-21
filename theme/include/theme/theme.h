// Reading a theme file: the bytes of one `.theme` file in, the authored values
// `ui` derives a palette from out, plus the typeface the file named and the
// name a person reads (ADR-0170). This is layer two of ADR-0096: `assets`
// splits the text into sections and keys and interprets nothing, and this file
// knows the schema and converts every value itself.
//
//     voe_theme theme;
//     if (voe_theme_read(bytes, size, arena, &theme)) {
//             voe_text_font *font = <the program's font for theme.typeface>;
//             voe_ui_theme palette = voe_ui_theme_derive(&theme.inputs, font);
//             voe_ui_theme_set(ui, &palette);
//     }
//
// THE FILE'S SHAPE IS EXACTLY ONE SECTION (ADR-0172):
//
//     [HarbourLight]
//     hue="#D4A02B"
//     contrast_strength=1.25
//     surface_separation=0.8
//     mode=light
//     font=oxanium
//     text_size=4.5
//
// The section name has no blanks in it (the parser refuses one). `hue` is the
// one colour the whole palette is tinted with, its lightness ignored
// (ADR-0194): `#RRGGBB`, quoted or not (the parser strips the quotes), kept as
// the authored sRGB in 0..1 per channel — the derivation converts it, not
// this. `contrast_strength` and `surface_separation` are numbers within
// VOE_UI_THEME_SCALAR_MIN..MAX, the very constants ui clamps to, so the two
// folders cannot disagree about range. `mode` is `light` or `dark`. These four
// are required. `font` and `text_size` (a number above nought, millimetres
// per em) are optional: absent, they are Oxanium and
// voe_ui_theme_default_inputs()'s text size. `font` may name anything and
// always reads as Oxanium, the one face the engine carries, with no report
// (ADR-0185). `accent` is not read any more and has no alias: a file that
// still names it is refused at that line as any unknown key is (ADR-0194).
//
// A FILE IS REFUSED WHOLE: false, `out` untouched, and one VOE_BASE_ERROR
// ("theme") naming the line and what is wrong, which a program reads back with
// voe_base_report_error_first(). The refusals, each at the line given:
//
//   - text the sectioned parser refuses — its own report, from `assets`;
//   - no section at all — line 1;
//   - a second section — that section's header line;
//   - a required key missing — the section's header line;
//   - a key this schema does not know — that key's line;
//   - a value that will not convert (not `#RRGGBB`, not a number, not `light`
//     or `dark`, a text size not above nought) — its line;
//   - a scalar outside VOE_UI_THEME_SCALAR_MIN..MAX — its line.
//
// Constraints. The strings — `name` among them — are pushed on `arena` and
// live as long as it does; what was pushed before a refusal is the caller's to
// rewind, as with every reader over `assets`. The first problem found is the
// one reported, so a file with two mistakes is fixed one line at a time.
// Neither this header nor theme/src/ names a `render` header (ADR-0176):
// `render` is on this folder's row only for the test's headless device.
#pragma once

#include <base/arena.h>
#include <text/font.h>
#include <ui/theme.h>

#include <stddef.h>

// One theme file, read.
typedef struct {
	// The section's name, NUL-terminated, in the arena the reader was
	// handed. For showing to a person; not an identity.
	//
	// THE NAME A PERSON READS IS THE SECTION'S, AND THE IDENTITY IS THE FILE
	// NAME (ADR-0172). A display name is the author's to change and two files
	// may claim the same one, so a program remembers a theme by the file it
	// came from; the section name is only what a list of themes shows. That is
	// also why this reader opens no file and never learns the file's name: the
	// caller hands it bytes, keeps the name, and adds it to the refusal it
	// shows a person.
	const char *name;
	voe_ui_theme_inputs inputs;
	voe_text_typeface typeface;
} voe_theme;

// Reads `size` bytes of `text` (need not be NUL-terminated) into `out`. True
// on success; false with `out` untouched and one report naming the line on
// refusal, per this file's header.
[[nodiscard]] bool voe_theme_read(const char *text, size_t size,
				  voe_base_arena *arena, voe_theme *out);
