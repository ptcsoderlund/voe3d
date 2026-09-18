// The derivation, pinned down against the five claims ADR-0171 and spec 006's
// criterion 4 make about it, one group of cases per claim:
//
//   - A ROUND TRIP THROUGH OKLAB holds for an in-gamut colour, which is what
//     makes every other case below trustworthy: if the conversion itself lost
//     colour, a legibility or chroma case could pass or fail for the wrong
//     reason.
//   - BOTH MODES STAY LEGIBLE AT BOTH ENDS OF BOTH SCALARS, checked at all
//     four corners of contrast_strength x surface_separation in each mode,
//     because the floor in role_lightness (theme.c) is exactly the thing that
//     keeps VOE_UI_THEME_SCALAR_MIN from producing unreadable text, and a
//     case anywhere in the middle would not exercise it.
//   - ONLY THE ACCENT MOVES WHEN ONLY THE ACCENT MOVES: every role but the
//     accent and its ink is a pure function of contrast_strength,
//     surface_separation and mode, so changing the authored colour and
//     nothing else must leave every one of them bit-identical.
//   - EACH SCALAR MOVES WHAT IT NAMES AND NOTHING ELSE: contrast_strength
//     moves the text roles and the border and nothing above them;
//     surface_separation moves the ground/surface/raised/control ladder and
//     nothing else. theme.c anchors text to `ground` rather than `surface`
//     for exactly this reason — see its header — and this is the case that
//     would catch the reference drifting back to `surface`.
//   - THE DARK/LIGHT CHROMA ASYMMETRY, pinned to the two figures ADR-0097's
//     bench found and ADR-0171 turned into a rule: a saturated accent keeps
//     less chroma in dark mode than in light.
//
// NEEDS NO GRAPHICS CARD. voe_ui_theme_derive only copies a font pointer
// through, never dereferences it, so every case here passes NULL and the
// whole file needs no device — see ui/theme.h on why that is safe.
//
// grey_lightness READS AN OKLAB LIGHTNESS BACK OUT OF A LINEAR ROLE COLOUR BY
// CUBE ROOT, RELYING ON THE ONE FACT THIS FILE IS ENTITLED TO ASSUME: every
// non-accent role is grey (a = b = 0), and for a = b = 0 the OKLab-to-linear
// matrices in oklab.c sum to exactly 1 on every channel, so r = g = b = L^3
// with no clamping in the range this derivation ever produces. That is a fact
// about this implementation and not about OKLab in general, which is why it
// stays in the test that already reaches into ../src/oklab.h rather than
// becoming a third public conversion.
#include <ui/theme.h>

#include "../src/oklab.h"

#include <math/float3.h>
#include <math/float4.h>

#include <testing/test.h>

#include <math.h>
#include <stddef.h>

static float grey_lightness(voe_math_float4 c)
{
	return cbrtf(c.x);
}

static bool float4_equal(voe_math_float4 a, voe_math_float4 b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}

// ---- a round trip through OKLab ----

static void check_round_trip(voe_math_float3 srgb)
{
	voe_ui_oklab lab = voe_ui_oklab_from_srgb(srgb);
	voe_math_float3 back = voe_ui_oklab_to_srgb(lab);

	VOE_TEST_CHECK_FLOAT(back.x, srgb.x, 0.002);
	VOE_TEST_CHECK_FLOAT(back.y, srgb.y, 0.002);
	VOE_TEST_CHECK_FLOAT(back.z, srgb.z, 0.002);
}

static void test_round_trip(void)
{
	check_round_trip((voe_math_float3){ 0.0f, 0.0f, 0.0f });
	check_round_trip((voe_math_float3){ 1.0f, 1.0f, 1.0f });
	check_round_trip((voe_math_float3){ 0.5f, 0.5f, 0.5f });
	check_round_trip((voe_math_float3){ 0.20f, 0.55f, 0.90f });
	check_round_trip((voe_math_float3){ 0.85f, 0.30f, 0.10f });
}

// ---- both modes stay legible at both ends of both scalars ----

static void check_legible(voe_ui_theme_mode mode, float contrast, float separation)
{
	voe_ui_theme_inputs inputs = voe_ui_theme_default_inputs();

	inputs.mode = mode;
	inputs.contrast_strength = contrast;
	inputs.surface_separation = separation;

	voe_ui_theme theme = voe_ui_theme_derive(&inputs, NULL);

	float ground_l = grey_lightness(theme.ground);
	float primary_l = grey_lightness(theme.text_primary);
	float secondary_l = grey_lightness(theme.text_secondary);
	float disabled_l = grey_lightness(theme.text_disabled);
	float border_l = grey_lightness(theme.border);

	float sign = mode == VOE_UI_THEME_MODE_DARK ? 1.0f : -1.0f;

	// Each floor in theme.c: 0.35, 0.22, 0.10, 0.05, given a small margin for
	// the cube-root readback. A step this coarse still catches a floor gone
	// missing entirely — the failure this case exists for is text landing at
	// or near the ground itself, not a change of one hundredth.
	VOE_TEST_CHECK(sign * (primary_l - ground_l) >= 0.33f);
	VOE_TEST_CHECK(sign * (secondary_l - ground_l) >= 0.20f);
	VOE_TEST_CHECK(sign * (disabled_l - ground_l) >= 0.08f);
	VOE_TEST_CHECK(sign * (border_l - ground_l) >= 0.03f);
}

static void test_legible_at_scalar_extremes(void)
{
	voe_ui_theme_mode modes[] = { VOE_UI_THEME_MODE_DARK, VOE_UI_THEME_MODE_LIGHT };
	float scalars[] = { VOE_UI_THEME_SCALAR_MIN, VOE_UI_THEME_SCALAR_MAX };

	for (size_t m = 0; m < 2; m++)
		for (size_t c = 0; c < 2; c++)
			for (size_t s = 0; s < 2; s++)
				check_legible(modes[m], scalars[c], scalars[s]);
}

// At the reference scalars (1.0, no clamp anywhere) the three text roles are
// strictly ordered by how far they clear the ground, which is what "three
// lightnesses" is supposed to buy over one.
static void test_text_roles_ordered_at_reference(void)
{
	voe_ui_theme_inputs inputs = voe_ui_theme_default_inputs();
	voe_ui_theme theme = voe_ui_theme_derive(&inputs, NULL);

	float ground_l = grey_lightness(theme.ground);
	float primary_gap = fabsf(grey_lightness(theme.text_primary) - ground_l);
	float secondary_gap = fabsf(grey_lightness(theme.text_secondary) - ground_l);
	float disabled_gap = fabsf(grey_lightness(theme.text_disabled) - ground_l);

	VOE_TEST_CHECK(primary_gap > secondary_gap);
	VOE_TEST_CHECK(secondary_gap > disabled_gap);
}

// ---- only the accent moves when only the accent moves ----

static void test_only_accent_moves(void)
{
	voe_ui_theme_inputs a = voe_ui_theme_default_inputs();
	voe_ui_theme_inputs b = a;

	b.accent = (voe_math_float3){ 0.15f, 0.80f, 0.25f };

	voe_ui_theme ta = voe_ui_theme_derive(&a, NULL);
	voe_ui_theme tb = voe_ui_theme_derive(&b, NULL);

	VOE_TEST_CHECK(float4_equal(ta.ground, tb.ground));
	VOE_TEST_CHECK(float4_equal(ta.surface, tb.surface));
	VOE_TEST_CHECK(float4_equal(ta.surface_raised, tb.surface_raised));
	VOE_TEST_CHECK(float4_equal(ta.border, tb.border));
	VOE_TEST_CHECK(float4_equal(ta.control, tb.control));
	VOE_TEST_CHECK(float4_equal(ta.control_hovered, tb.control_hovered));
	VOE_TEST_CHECK(float4_equal(ta.text_primary, tb.text_primary));
	VOE_TEST_CHECK(float4_equal(ta.text_secondary, tb.text_secondary));
	VOE_TEST_CHECK(float4_equal(ta.text_disabled, tb.text_disabled));
	VOE_TEST_CHECK(!float4_equal(ta.accent, tb.accent));
}

// ---- each scalar moves what it names and nothing else ----

static void test_contrast_moves_only_text_and_border(void)
{
	voe_ui_theme_inputs lo = voe_ui_theme_default_inputs();
	voe_ui_theme_inputs hi = lo;

	lo.contrast_strength = VOE_UI_THEME_SCALAR_MIN;
	hi.contrast_strength = VOE_UI_THEME_SCALAR_MAX;

	voe_ui_theme tlo = voe_ui_theme_derive(&lo, NULL);
	voe_ui_theme thi = voe_ui_theme_derive(&hi, NULL);

	VOE_TEST_CHECK(float4_equal(tlo.ground, thi.ground));
	VOE_TEST_CHECK(float4_equal(tlo.surface, thi.surface));
	VOE_TEST_CHECK(float4_equal(tlo.surface_raised, thi.surface_raised));
	VOE_TEST_CHECK(float4_equal(tlo.control, thi.control));
	VOE_TEST_CHECK(float4_equal(tlo.control_hovered, thi.control_hovered));
	VOE_TEST_CHECK(float4_equal(tlo.accent, thi.accent));
	VOE_TEST_CHECK(float4_equal(tlo.accent_ink, thi.accent_ink));

	VOE_TEST_CHECK(!float4_equal(tlo.text_primary, thi.text_primary));
	VOE_TEST_CHECK(!float4_equal(tlo.text_secondary, thi.text_secondary));
	VOE_TEST_CHECK(!float4_equal(tlo.text_disabled, thi.text_disabled));
	VOE_TEST_CHECK(!float4_equal(tlo.border, thi.border));
}

static void test_separation_moves_only_the_ladder(void)
{
	voe_ui_theme_inputs lo = voe_ui_theme_default_inputs();
	voe_ui_theme_inputs hi = lo;

	lo.surface_separation = VOE_UI_THEME_SCALAR_MIN;
	hi.surface_separation = VOE_UI_THEME_SCALAR_MAX;

	voe_ui_theme tlo = voe_ui_theme_derive(&lo, NULL);
	voe_ui_theme thi = voe_ui_theme_derive(&hi, NULL);

	VOE_TEST_CHECK(float4_equal(tlo.ground, thi.ground));
	VOE_TEST_CHECK(float4_equal(tlo.border, thi.border));
	VOE_TEST_CHECK(float4_equal(tlo.text_primary, thi.text_primary));
	VOE_TEST_CHECK(float4_equal(tlo.text_secondary, thi.text_secondary));
	VOE_TEST_CHECK(float4_equal(tlo.text_disabled, thi.text_disabled));
	VOE_TEST_CHECK(float4_equal(tlo.accent, thi.accent));
	VOE_TEST_CHECK(float4_equal(tlo.accent_ink, thi.accent_ink));

	VOE_TEST_CHECK(!float4_equal(tlo.surface, thi.surface));
	VOE_TEST_CHECK(!float4_equal(tlo.surface_raised, thi.surface_raised));
	VOE_TEST_CHECK(!float4_equal(tlo.control, thi.control));
	VOE_TEST_CHECK(!float4_equal(tlo.control_hovered, thi.control_hovered));
}

// ---- the dark/light chroma asymmetry ----

static float role_chroma(voe_math_float4 role)
{
	voe_ui_oklab lab = voe_ui_oklab_from_linear((voe_math_float3){ role.x, role.y, role.z });

	return sqrtf(lab.a * lab.a + lab.b * lab.b);
}

static void test_dark_clamps_chroma_harder_than_light(void)
{
	// Pure sRGB red: well outside both chroma ceilings, so both modes clamp
	// and the case is about which one clamps harder rather than whether
	// either clamps at all.
	voe_math_float3 saturated = { 1.0f, 0.0f, 0.0f };

	voe_ui_theme_inputs dark = voe_ui_theme_default_inputs();
	voe_ui_theme_inputs light = voe_ui_theme_default_inputs();

	dark.accent = saturated;
	dark.mode = VOE_UI_THEME_MODE_DARK;
	light.accent = saturated;
	light.mode = VOE_UI_THEME_MODE_LIGHT;

	voe_ui_theme tdark = voe_ui_theme_derive(&dark, NULL);
	voe_ui_theme tlight = voe_ui_theme_derive(&light, NULL);

	float chroma_dark = role_chroma(tdark.accent);
	float chroma_light = role_chroma(tlight.accent);

	VOE_TEST_CHECK(chroma_dark < chroma_light);
	// The figures ADR-0097's bench found and ADR-0171 fixed as the rule.
	VOE_TEST_CHECK_FLOAT(chroma_dark, 0.10, 0.01);
	VOE_TEST_CHECK_FLOAT(chroma_light, 0.22, 0.01);
}

int main(void)
{
	test_round_trip();
	test_legible_at_scalar_extremes();
	test_text_roles_ordered_at_reference();
	test_only_accent_moves();
	test_contrast_moves_only_text_and_border();
	test_separation_moves_only_the_ladder();
	test_dark_clamps_chroma_harder_than_light();

	return voe_test_result();
}
