// The derivation. See include/ui/theme.h for what every field means and why
// nothing here can fail.
//
// EVERY ROLE BUT THE ACCENT IS GREY — a = b = 0 in OKLab — which is what keeps
// the interface monochrome plus one accent (ADR-0097) rather than every
// surface picking up a tint of the accent's hue nobody asked for. grey_rgba
// below is the one place that is true, and every surface, border and text
// role goes through it.
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
// FROM WHICHEVER SURFACE THEY ARE ACTUALLY DRAWN ON. ADR-0169 says a text role
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
// DEVIATION: ADR-0169 ("a text role is stepped from the surface it sits
// on"), read narrowly as `ground` — the ladder's own bottom surface — rather
// than `surface`, because only the `ground` reading keeps contrast_strength
// and surface_separation independent, which task 3's own required tests
// ("each scalar moves what it names and nothing else") could not otherwise
// pass.
//
// role_lightness IS THE "STEPPED UNTIL IT CLEARS ITS SURFACE" IN ADR-0169: the
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

// The accent's chroma ceiling, per mode (ADR-0097's bench figures, kept by
// ADR-0169): a saturated colour fringes on a dark ground well before it does
// on a light one, so the dark ceiling is under half the light one.
#define ACCENT_CHROMA_MAX_DARK 0.10f
#define ACCENT_CHROMA_MAX_LIGHT 0.22f

// Which ink goes on the accent: near-black above this lightness, near-white
// at or below it, so the ink is always the end of the scale furthest from the
// accent's own lightness.
#define ACCENT_INK_SPLIT_L 0.6f
#define INK_DARK_L 0.08f
#define INK_LIGHT_L 0.97f

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

static voe_math_float4 grey_rgba(float l)
{
	return rgba_from_linear(voe_ui_oklab_to_linear((voe_ui_oklab){ l, 0.0f, 0.0f }));
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
		.accent = (voe_math_float3){ 0.30f, 0.55f, 0.95f },
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

	voe_ui_oklab accent_lab = voe_ui_oklab_from_srgb(inputs->accent);
	float max_chroma = dark ? ACCENT_CHROMA_MAX_DARK : ACCENT_CHROMA_MAX_LIGHT;
	float chroma = sqrtf(accent_lab.a * accent_lab.a + accent_lab.b * accent_lab.b);

	if (chroma > max_chroma && chroma > 0.0f) {
		float scale = max_chroma / chroma;

		accent_lab.a *= scale;
		accent_lab.b *= scale;
	}

	float ink_l = accent_lab.l > ACCENT_INK_SPLIT_L ? INK_DARK_L : INK_LIGHT_L;

	return (voe_ui_theme){
		.ground = grey_rgba(ground_l),
		.surface = grey_rgba(surface_l),
		.surface_raised = grey_rgba(raised_l),
		.border = grey_rgba(border_l),
		.control = grey_rgba(control_l),
		.control_hovered = grey_rgba(control_hovered_l),
		.text_primary = grey_rgba(text_primary_l),
		.text_secondary = grey_rgba(text_secondary_l),
		.text_disabled = grey_rgba(text_disabled_l),
		.accent = rgba_from_linear(voe_ui_oklab_to_linear(accent_lab)),
		.accent_ink = grey_rgba(ink_l),
		.font = font,
		.text_size = inputs->text_size,
	};
}
