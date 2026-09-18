// A theme: the handful of values a person authors, and the palette of roles a
// widget actually draws with. Splitting the two is the whole point of this
// file (ADR-0097, ADR-0170, ADR-0171) — a theme file or a game names four
// things and a size, and everything a panel, a label or a button needs comes
// out the other end already worked out.
//
//     voe_ui_theme_inputs inputs = voe_ui_theme_default_inputs();
//     inputs.accent = (voe_math_float3){ 0.2f, 0.6f, 0.9f };
//     voe_ui_theme theme = voe_ui_theme_derive(&inputs, font);
//     voe_ui_theme_set(ui, &theme);                 // task 4
//
// WHO READS THE FILE AND WHO DERIVES THE PALETTE ARE TWO DIFFERENT FOLDERS
// (ADR-0170). `theme` turns a `.theme` file's bytes into a voe_ui_theme_inputs
// and hands it here; this folder never opens a file, never names `assets` and
// never picks a typeface's bytes apart — it only turns the five authored
// values into the roles below. A game that wants a theme with no file at all
// may fill in voe_ui_theme_inputs itself and call voe_ui_theme_derive directly.
//
// THE DERIVATION RUNS IN OKLAB AND NOTHING HERE IS ARITHMETIC ON sRGB
// CHANNELS (ADR-0171). `accent` is authored sRGB exactly as a person or a file
// wrote it — this struct does not convert it — and every voe_ui_theme field is
// LINEAR, ready for an element record, because the conversion is the
// derivation's first and last step and nowhere else. The OKLab conversion
// itself lives in ui/src/oklab.h, internal to this folder, and moves to `math`
// the day a second folder needs it (ADR-0170).
//
// ONE COLOUR AND TWO NUMBERS ARE ENOUGH BECAUSE HUE IS NEVER ROTATED. Every
// role but the accent is a step of LIGHTNESS ALONE — zero chroma, so the
// interface stays the monochrome-plus-one-accent look ADR-0097 asked for —
// and `contrast_strength` and `surface_separation` only ever move how big that
// step is. `mode` decides which way the steps run: dark surfaces lighten as
// they rise off the ground and light ones darken, and text moves further from
// its ground the same way. NEITHER SCALAR CAN MAKE TEXT UNREADABLE: each of
// the three text lightnesses and the border is stepped from `ground` — chosen
// over `surface` so that surface_separation cannot move text at all, which is
// what lets each scalar be shown to move only what it names — by whichever is
// bigger, the scaled step or a floor that already clears it, so a
// contrast_strength of nought would still hand back legible text rather than
// none.
//
// THE ACCENT'S CHROMA IS CLAMPED HARDER IN DARK MODE THAN IN LIGHT (ADR-0097,
// ADR-0171): a saturated colour that is comfortable on a light ground fringes
// on a dark one, so VOE_UI_THEME_ACCENT_CHROMA_MAX_DARK is under half of
// VOE_UI_THEME_ACCENT_CHROMA_MAX_LIGHT. Only chroma moves; the accent's own
// hue and lightness are exactly what the derivation found in the authored
// colour.
//
// THE ROLES ARE WHAT THE WIDGETS DRAW WITH AND NO MORE (ADR-0171, rule 10):
// three surfaces to sit a panel on, a hairline border, a control and its
// hovered state, three text lightnesses, the accent, and the ink that goes on
// it. A pressed control and a dragged number box are the accent itself
// (task 4) rather than a fourth control colour — there is no role for a state
// nothing here asks for.
//
// A NULL FONT IS ALLOWED HERE. voe_ui_theme_derive only copies the pointer
// into the palette it returns; nothing in this file measures a string, so
// nothing here needs the font to exist. A widget that goes on to measure a
// label against a NULL font asserts there (task 4), not in this derivation.
//
// VOE_UI_THEME_SCALAR_MIN AND _MAX ARE PART OF THE PUBLIC SURFACE ON PURPOSE:
// the derivation clamps `contrast_strength` and `surface_separation` into this
// range defensively, but `theme` (the folder that reads a theme file) has to
// refuse an authored value outside it before this code ever sees it, and it
// can only do that by naming the same range. One pair of constants, so the
// two folders can never disagree about what "in range" means.
#pragma once

#include <math/float3.h>
#include <math/float4.h>
#include <text/font.h>

// Light or dark, in the sense of which end of the lightness scale the ground
// sits at. There is no built-in light theme (spec 006 leaves that to a file
// anyone can write); this enum exists so one can be authored.
typedef enum {
	VOE_UI_THEME_MODE_DARK,
	VOE_UI_THEME_MODE_LIGHT,
} voe_ui_theme_mode;

// Every `contrast_strength` and every `surface_separation` is clamped into
// this range, in here and in `theme` alike. 1.0 sits in the middle of it and
// is the reference look every ADR describes; going outside it either flattens
// the theme to nothing or pushes a lightness step past what stays legible.
#define VOE_UI_THEME_SCALAR_MIN 0.25f
#define VOE_UI_THEME_SCALAR_MAX 3.0f

// The five things a person authors, and nothing else (ADR-0097). This is
// exactly what a `.theme` file says, in the units it says them in: `accent` is
// SRGB, NOT LINEAR, because that is what a person picks with a colour wheel
// and what a file writes as `#RRGGBB` — the derivation converts it, once, on
// the way to voe_ui_theme, so this struct is never touched by that
// conversion and stays a faithful copy of what was authored.
typedef struct {
	// 0..1 per channel, sRGB-encoded.
	voe_math_float3 accent;
	// How far every text role and the border step from `ground`. 1.0 is the
	// reference; see VOE_UI_THEME_SCALAR_MIN/MAX.
	float contrast_strength;
	// How far apart ground, surface, raised surface, control and its
	// hovered state sit from each other. Same range as contrast_strength,
	// and the two move independently of one another.
	float surface_separation;
	voe_ui_theme_mode mode;
	// Millimetres per em. The one place a text size is said (task 4 removes
	// voe_ui_text_scale_set for exactly this reason) — a caller wanting a
	// bigger interface authors a bigger theme rather than scaling on top of
	// it.
	float text_size;
} voe_ui_theme_inputs;

// The derived palette: what a widget actually draws with. Every colour is
// LINEAR RGBA, fully opaque (alpha 1) — there is no role here for
// transparency, which is a widget's own concern (task 4's NONE surface, for
// one).
typedef struct {
	// The panel's own background — the least raised surface there is.
	voe_math_float4 ground;
	// The ordinary content surface a panel sits its children on.
	voe_math_float4 surface;
	// A surface that reads as sitting above `surface` — a floating panel, a
	// popped-up row.
	voe_math_float4 surface_raised;
	// The hairline rectangle a bordered widget draws behind its fill,
	// stepped from `ground` by `contrast_strength` exactly as the text
	// roles are (see ui/src/theme.c for why `ground` and not `surface`).
	voe_math_float4 border;
	// An interactive control's own fill at rest — a button, a field, a
	// scrollbar's track.
	voe_math_float4 control;
	// The same control while the pointer is over it. A control being
	// pressed, or a number box being dragged, is the accent instead
	// (task 4) — there is no third control colour.
	voe_math_float4 control_hovered;
	// Ordinary text.
	voe_math_float4 text_primary;
	// A caption, a hint, anything meant to read as quieter than ordinary
	// text without being unreadable.
	voe_math_float4 text_secondary;
	// The dimmest of the three, for a disabled control's label.
	voe_math_float4 text_disabled;
	// The one authored colour, converted to linear and chroma-clamped for
	// `mode` (see this file's header). What a selection, a pressed control
	// and a dragged number box are drawn in.
	voe_math_float4 accent;
	// The text or icon colour that goes ON the accent — chosen for contrast
	// against it and nothing else, so it is always legible whatever the
	// accent's own lightness is.
	voe_math_float4 accent_ink;
	// The font this theme draws with (ADR-0167), and how big an em is on
	// this surface. NULL is allowed — see this file's header — and is what
	// a theme derived with no font in hand carries until a caller sets one.
	const voe_text_font *font;
	float text_size;
} voe_ui_theme;

// The built-in theme's inputs: near-black, a blue accent, the reference
// scalars, and the editor's present text size (4 millimetres per em) so that
// criterion 1 is a look the sponsor can compare against today's interface.
// Its font is the caller's to set on the voe_ui_theme this derives — this
// struct alone says nothing about a typeface.
voe_ui_theme_inputs voe_ui_theme_default_inputs(void);

// Works out the palette. `inputs` is read once and never kept; `font` is
// copied by pointer into the result and must outlive it, exactly as
// voe_ui_font_set already requires — and may be NULL, per this file's header.
//
// NEVER FAILS. contrast_strength and surface_separation are clamped into
// VOE_UI_THEME_SCALAR_MIN..MAX before anything is derived from them, so there
// is no authored combination this call refuses; a caller wanting to refuse an
// out-of-range file does so before calling this, in `theme`.
voe_ui_theme voe_ui_theme_derive(const voe_ui_theme_inputs *inputs,
				 const voe_text_font *font);
