// The derivation. See include/ui/theme.h for what every field means and why
// nothing here can fail.
//
// EVERY ROLE CARRIES THE ONE AUTHORED HUE AND THEY DIFFER ONLY IN LIGHTNESS
// (ADR-0194). `inputs->hue` is read for its OKLab a and b alone — its own
// lightness is thrown away — and those two, chroma-clamped once for `mode`,
// are handed to every role below. hue_rgba is the one place that happens, and
// every surface, border, text role and the inverse pair goes through it.
//
// CHROMA SHRINKS PER ROLE ONLY WHERE THE GAMUT MAKES IT. A lightness near
// either end of the scale has little room for chroma, and voe_ui_oklab_to_linear
// CLAMPS rather than failing (see oklab.h), so asking for more than the gamut
// holds would silently bend the hue instead of dimming it. hue_rgba therefore
// scales a and b by the largest factor that survives a round trip through
// linear, found by twelve steps of bisection — the hue angle is untouched, so
// the shrinking can only cost saturation, never the shared hue this whole
// file exists to keep. Twelve steps is a thousandth of the factor, well under
// what an eye or a test can see; a closed-form gamut boundary would be faster
// and is not worth a second approximation of the same curve here.
//
// THE LADDER FROM `ground` TO `control_hovered` IS FIVE EQUAL STEPS OF
// surface_separation, ALL IN THE SAME DIRECTION: away from whichever extreme
// `mode` put the ground at. Dark surfaces lighten as they rise; light ones
// darken. A control and its hovered state are two rungs further up the same
// ladder rather than a different rule, which is what makes "a control stands
// out a bit more than a raised surface, and hovering it stands out a bit more
// again" the same sentence at every value of the scalar.
//
// THE THREE TEXT LIGHTNESSES AND THE BORDER ARE STEPPED FROM `ground`, NOT
// FROM WHICHEVER SURFACE THEY ARE ACTUALLY DRAWN ON. ADR-0171 says a text role
// is stepped from the surface it sits on, and `ground` is a surface — the
// bottom rung of the one ladder this derivation has — chosen as the reference
// rather than `surface` FOR A NAMED REASON: `ground` never moves with
// surface_separation, so contrast_strength moves text and the border and
// nothing else, and surface_separation moves the ladder and nothing else.
// Anchoring text to `surface` instead would make text drift every time
// surface_separation changed the surface under it, so the two scalars could
// never be shown to move only what they name — which is a claim this folder's
// own tests have to be able to make. The honest cost is the same one either
// reference would have: a widget actually drawn on `surface` or
// `surface_raised` reads text a little closer to, or further from, its own
// background than the formula assumed, because there is no per-surface
// variant of a text colour in this struct to compute.
//
// DEVIATION: ADR-0171 ("a text role is stepped from the surface it sits
// on"), read narrowly as `ground` — the ladder's own bottom surface — rather
// than `surface`, because only the `ground` reading keeps contrast_strength
// and surface_separation independent, which task 3's own required tests
// ("each scalar moves what it names and nothing else") could not otherwise
// pass.
//
// role_lightness IS THE "STEPPED UNTIL IT CLEARS ITS SURFACE" IN ADR-0171: the
// step is contrast_strength's own, or a floor that already clears the
// surface, whichever is bigger. That is what makes "both modes stay legible
// at both ends of both scalars" true by construction rather than by
// coincidence of the constants below — turning contrast_strength down to
// VOE_UI_THEME_SCALAR_MIN can shrink the step nothing further, because the
// floor is what is left standing once the scaled step drops under it.
#include <ui/theme.h>

#include "oklab.h"

#include <base/assert.h>

#include <math.h>

// The ground's own lightness at each end of `mode`, near enough to black or
// white that ADR-0097's "near-black" default reads as such while still
// leaving room for four rungs of ladder above it before either clips.
#define GROUND_L_DARK 0.16f
#define GROUND_L_LIGHT 0.94f

// One rung of the ground/surface/raised/control/control_hovered ladder, before
// surface_separation scales it. Four rungs above ground at the reference
// scalar (1.0) move it a fifth of the way to the opposite lightness extreme.
#define SURFACE_STEP_BASE 0.05f

// Text and the border step from `ground`, sized by contrast_strength, but
// never by less than the paired floor — see role_lightness and this file's
// header.
#define TEXT_PRIMARY_STEP_BASE 0.62f
#define TEXT_PRIMARY_FLOOR 0.35f
#define TEXT_SECONDARY_STEP_BASE 0.42f
#define TEXT_SECONDARY_FLOOR 0.22f
#define TEXT_DISABLED_STEP_BASE 0.24f
#define TEXT_DISABLED_FLOOR 0.10f
#define BORDER_STEP_BASE 0.14f
#define BORDER_FLOOR 0.05f

// The hue's chroma ceiling, per mode (ADR-0097's bench figures, kept by
// ADR-0171 and by ADR-0194 for the one hue): a saturated colour fringes on a
// dark ground well before it does on a light one, so the dark ceiling is under
// half the light one.
#define HUE_CHROMA_MAX_DARK 0.10f
#define HUE_CHROMA_MAX_LIGHT 0.22f

// A chroma at or under this is no colour at all — a grey the author wrote as
// #808080 or as a value a hair off it — and is taken as exactly zero so that a
// grey theme is bit-for-bit grey in every role.
#define CHROMA_NONE 1e-4f

// How close a round trip through linear has to land for hue_rgba to call a
// colour in gamut, and how many times it halves the interval looking for the
// largest factor that does.
#define IN_GAMUT_TOLERANCE 1e-3f
#define GAMUT_BISECTION_STEPS 12

static float clampf(float v, float lo, float hi)
{
	if (v < lo)
		return lo;
	if (v > hi)
		return hi;
	return v;
}

static float scalar_clamp(float v)
{
	return clampf(v, VOE_UI_THEME_SCALAR_MIN, VOE_UI_THEME_SCALAR_MAX);
}

static voe_math_float4 rgba_from_linear(voe_math_float3 linear)
{
	return (voe_math_float4){ linear.x, linear.y, linear.z, 1.0f };
}

// Whether a monitor can actually show `lab`: voe_ui_oklab_to_linear clamps
// into 0..1 per channel, so a colour outside the gamut comes back as a
// different colour, and converting that back is how this file sees it happen.
static bool in_gamut(voe_ui_oklab lab)
{
	voe_ui_oklab back = voe_ui_oklab_from_linear(voe_ui_oklab_to_linear(lab));

	return fabsf(back.l - lab.l) < IN_GAMUT_TOLERANCE &&
	       fabsf(back.a - lab.a) < IN_GAMUT_TOLERANCE &&
	       fabsf(back.b - lab.b) < IN_GAMUT_TOLERANCE;
}

// One role: the lightness `l`, in the theme's hue, at as much of the hue's
// chroma (`a`, `b`) as that lightness can hold. See this file's header for why
// the factor is found by bisection and why shrinking it cannot move the hue.
static voe_math_float4 hue_rgba(float l, float a, float b)
{
	// a = b = 0 is grey, which is in gamut at every lightness, so the low
	// end of the interval is known good before the first step.
	float lo = 0.0f;
	float hi = 1.0f;

	for (int step = 0; step < GAMUT_BISECTION_STEPS; step++) {
		float mid = 0.5f * (lo + hi);

		if (in_gamut((voe_ui_oklab){ l, a * mid, b * mid }))
			lo = mid;
		else
			hi = mid;
	}
	return rgba_from_linear(voe_ui_oklab_to_linear((voe_ui_oklab){ l, a * lo, b * lo }));
}

// One text or border lightness: `ground_l` plus the bigger of the scaled step
// and its floor, moved `direction` (+1 or -1) and clamped into 0..1. See this
// file's header for why the reference is `ground` and not `surface`, and for
// why the floor is what keeps every text role legible regardless of how low
// contrast_strength goes.
static float role_lightness(float ground_l, float base_step, float floor_step,
			    float scalar, int direction)
{
	float step = base_step * scalar;

	if (step < floor_step)
		step = floor_step;
	return clampf(ground_l + (float)direction * step, 0.0f, 1.0f);
}

voe_ui_theme_inputs voe_ui_theme_default_inputs(void)
{
	return (voe_ui_theme_inputs){
		// #808080: a grey, so the built-in Near black and Near white
		// have no chroma in any role (ADR-0194).
		.hue = (voe_math_float3){ 0.502f, 0.502f, 0.502f },
		.contrast_strength = 1.0f,
		.surface_separation = 1.0f,
		.mode = VOE_UI_THEME_MODE_DARK,
		// Matches TEXT_EM in widgets.c at the text_scale of 1.0 every
		// caller uses today, so the built-in theme is the size the
		// editor already draws at.
		.text_size = 4.0f,
	};
}

voe_ui_theme voe_ui_theme_derive(const voe_ui_theme_inputs *inputs,
				 const voe_text_font *font)
{
	VOE_BASE_ASSERT(inputs != NULL, "deriving a theme from no inputs");

	bool dark = inputs->mode == VOE_UI_THEME_MODE_DARK;
	float contrast = scalar_clamp(inputs->contrast_strength);
	float separation = scalar_clamp(inputs->surface_separation);

	float ground_l = dark ? GROUND_L_DARK : GROUND_L_LIGHT;
	// Surfaces rise away from the ground's own extreme; text goes further
	// the same way, which is the same "away from the ground" direction.
	int direction = dark ? 1 : -1;

	float surface_l = clampf(ground_l + (float)direction * 1.0f * SURFACE_STEP_BASE * separation, 0.0f, 1.0f);
	float raised_l = clampf(ground_l + (float)direction * 2.0f * SURFACE_STEP_BASE * separation, 0.0f, 1.0f);
	float control_l = clampf(ground_l + (float)direction * 3.0f * SURFACE_STEP_BASE * separation, 0.0f, 1.0f);
	float control_hovered_l = clampf(ground_l + (float)direction * 4.0f * SURFACE_STEP_BASE * separation, 0.0f, 1.0f);

	float text_primary_l = role_lightness(ground_l, TEXT_PRIMARY_STEP_BASE,
					      TEXT_PRIMARY_FLOOR, contrast, direction);
	float text_secondary_l = role_lightness(ground_l, TEXT_SECONDARY_STEP_BASE,
						TEXT_SECONDARY_FLOOR, contrast, direction);
	float text_disabled_l = role_lightness(ground_l, TEXT_DISABLED_STEP_BASE,
					       TEXT_DISABLED_FLOOR, contrast, direction);
	float border_l = role_lightness(ground_l, BORDER_STEP_BASE, BORDER_FLOOR,
					contrast, direction);

	// The hue's two chroma axes, clamped for the mode. Its lightness is
	// read and thrown away: a role's lightness is the ladder's, never the
	// authored colour's.
	voe_ui_oklab hue_lab = voe_ui_oklab_from_srgb(inputs->hue);
	float max_chroma = dark ? HUE_CHROMA_MAX_DARK : HUE_CHROMA_MAX_LIGHT;
	float chroma = sqrtf(hue_lab.a * hue_lab.a + hue_lab.b * hue_lab.b);
	float a = hue_lab.a;
	float b = hue_lab.b;

	if (chroma <= CHROMA_NONE) {
		a = 0.0f;
		b = 0.0f;
	} else if (chroma > max_chroma) {
		a *= max_chroma / chroma;
		b *= max_chroma / chroma;
	}

	// `inverse` is the text_primary lightness and `inverse_ink` the
	// ground's, which is what makes a held or selected control as legible
	// as ordinary text on the ground (ADR-0196).
	voe_math_float4 inverse = hue_rgba(text_primary_l, a, b);
	voe_math_float4 inverse_ink = hue_rgba(ground_l, a, b);

	return (voe_ui_theme){
		.ground = hue_rgba(ground_l, a, b),
		.surface = hue_rgba(surface_l, a, b),
		.surface_raised = hue_rgba(raised_l, a, b),
		.border = hue_rgba(border_l, a, b),
		.control = hue_rgba(control_l, a, b),
		.control_hovered = hue_rgba(control_hovered_l, a, b),
		.text_primary = hue_rgba(text_primary_l, a, b),
		.text_secondary = hue_rgba(text_secondary_l, a, b),
		.text_disabled = hue_rgba(text_disabled_l, a, b),
		.inverse = inverse,
		.inverse_ink = inverse_ink,
		.font = font,
		.text_size = inputs->text_size,
	};
}
