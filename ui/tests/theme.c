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
//   - ONE HUE IN EVERY ROLE (ADR-0194): whatever colour is authored, every
//     role that has any chroma at all comes out at that colour's hue angle
//     and under the mode's chroma ceiling, so no role is ever set apart by
//     colour; a grey hue gives a palette with no chroma anywhere; and the
//     authored colour moves only the tint, never a role's lightness, which
//     is what lets the legibility cases above stand whatever the hue is.
//   - EACH SCALAR MOVES WHAT IT NAMES AND NOTHING ELSE: contrast_strength
//     moves the text roles and the border and nothing above them;
//     surface_separation moves the ground/surface/raised/control ladder and
//     nothing else. theme.c anchors text to `ground` rather than `surface`
//     for exactly this reason — see its header — and this is the case that
//     would catch the reference drifting back to `surface`.
//   - THE DARK/LIGHT CHROMA ASYMMETRY, pinned to the two figures ADR-0097's
//     bench found and ADR-0171 turned into a rule: a saturated hue keeps
//     less chroma in dark mode than in light, measured on `surface` now that
//     there is no accent role to measure it on.
//
// NEEDS NO GRAPHICS CARD. voe_ui_theme_derive only copies a font pointer
// through, never dereferences it, so every case here passes NULL and the
// whole file needs no device — see ui/theme.h on why that is safe.
//
// grey_lightness READS AN OKLAB LIGHTNESS BACK OUT OF A LINEAR ROLE COLOUR BY
// CUBE ROOT, AND EVERY CASE THAT USES IT DERIVES FROM THE DEFAULT INPUTS,
// WHOSE HUE IS GREY. For a = b = 0 the OKLab-to-linear matrices in oklab.c sum
// to exactly 1 on every channel, so r = g = b = L^3 with no clamping in the
// range this derivation ever produces. That is a fact about this
// implementation and not about OKLab in general, which is why it stays in the
// test that already reaches into ../src/oklab.h rather than becoming a third
// public conversion. A case that authors a hue with chroma in it reads
// lightness through role_lab instead.
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

static voe_ui_oklab role_lab(voe_math_float4 role)
{
	return voe_ui_oklab_from_linear((voe_math_float3){ role.x, role.y, role.z });
}

static float role_chroma(voe_math_float4 role)
{
	voe_ui_oklab lab = role_lab(role);

	return sqrtf(lab.a * lab.a + lab.b * lab.b);
}

// Every role of a derived palette, in one array, so a case about ALL of them
// cannot quietly miss one that was added later.
#define ROLE_COUNT 11

static void theme_roles(const voe_ui_theme *theme, voe_math_float4 *out)
{
	out[0] = theme->ground;
	out[1] = theme->surface;
	out[2] = theme->surface_raised;
	out[3] = theme->border;
	out[4] = theme->control;
	out[5] = theme->control_hovered;
	out[6] = theme->text_primary;
	out[7] = theme->text_secondary;
	out[8] = theme->text_disabled;
	out[9] = theme->inverse;
	out[10] = theme->inverse_ink;
}

// The ceilings in ../src/theme.c, which this file is allowed to name because
// ADR-0171 fixed them as the rule rather than as an implementation detail.
#define CHROMA_MAX_DARK 0.10f
#define CHROMA_MAX_LIGHT 0.22f

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
	float inverse_l = grey_lightness(theme.inverse);
	float inverse_ink_l = grey_lightness(theme.inverse_ink);

	float sign = mode == VOE_UI_THEME_MODE_DARK ? 1.0f : -1.0f;

	// Each floor in theme.c: 0.35, 0.22, 0.10, 0.05, given a small margin for
	// the cube-root readback. A step this coarse still catches a floor gone
	// missing entirely — the failure this case exists for is text landing at
	// or near the ground itself, not a change of one hundredth.
	VOE_TEST_CHECK(sign * (primary_l - ground_l) >= 0.33f);
	VOE_TEST_CHECK(sign * (secondary_l - ground_l) >= 0.20f);
	VOE_TEST_CHECK(sign * (disabled_l - ground_l) >= 0.08f);
	VOE_TEST_CHECK(sign * (border_l - ground_l) >= 0.03f);
	// The inverted pair is the text_primary/ground pair swapped round, so
	// it clears the same floor: a held control's ink is as far from its
	// fill as ordinary text is from the ground.
	VOE_TEST_CHECK(sign * (inverse_l - inverse_ink_l) >= 0.33f);
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

// ---- one hue in every role ----

// The authored colour tints the palette and moves nothing else: a role's
// lightness is the ladder's, never the hue's own, so the legibility cases
// above hold for every hue and not just for the grey default.
static void test_hue_moves_only_tint(void)
{
	voe_ui_theme_inputs a = voe_ui_theme_default_inputs();
	voe_ui_theme_inputs b = a;

	b.hue = (voe_math_float3){ 0.15f, 0.80f, 0.25f };

	voe_ui_theme ta = voe_ui_theme_derive(&a, NULL);
	voe_ui_theme tb = voe_ui_theme_derive(&b, NULL);
	voe_math_float4 ra[ROLE_COUNT];
	voe_math_float4 rb[ROLE_COUNT];

	theme_roles(&ta, ra);
	theme_roles(&tb, rb);

	for (size_t i = 0; i < ROLE_COUNT; i++) {
		VOE_TEST_CHECK_FLOAT(role_lab(rb[i]).l, role_lab(ra[i]).l, 0.01);
		VOE_TEST_CHECK(!float4_equal(ra[i], rb[i]));
	}
}

static void check_one_hue(voe_ui_theme_mode mode, float contrast, float separation,
			  voe_math_float3 hue)
{
	voe_ui_theme_inputs inputs = voe_ui_theme_default_inputs();

	inputs.mode = mode;
	inputs.contrast_strength = contrast;
	inputs.surface_separation = separation;
	inputs.hue = hue;

	voe_ui_theme theme = voe_ui_theme_derive(&inputs, NULL);
	voe_ui_oklab authored = voe_ui_oklab_from_srgb(hue);
	float authored_angle = atan2f(authored.b, authored.a);
	float max_chroma = mode == VOE_UI_THEME_MODE_DARK ? CHROMA_MAX_DARK : CHROMA_MAX_LIGHT;
	voe_math_float4 roles[ROLE_COUNT];

	theme_roles(&theme, roles);

	for (size_t i = 0; i < ROLE_COUNT; i++) {
		voe_ui_oklab lab = role_lab(roles[i]);
		float chroma = sqrtf(lab.a * lab.a + lab.b * lab.b);

		// The ceiling is the hue's, and the gamut fit in theme.c may
		// only shrink chroma further; the margin is the readback's.
		VOE_TEST_CHECK(chroma <= max_chroma + 0.002f);
		// Below this there is no hue to speak of — a role the gamut
		// squeezed to nearly grey — and atan2 on it says nothing. None
		// of the three hues below sits at the +/-pi seam, so comparing
		// angles directly needs no wrapping.
		if (chroma > 0.005f)
			VOE_TEST_CHECK(fabsf(atan2f(lab.b, lab.a) - authored_angle) < 0.02f);
	}
}

// ADR-0194's rule, across both modes, both ends and the middle of both
// scalars, and three authored hues: every role that has any chroma has the
// authored hue's, and none has more chroma than its mode allows.
static void test_every_role_shares_one_hue(void)
{
	voe_ui_theme_mode modes[] = { VOE_UI_THEME_MODE_DARK, VOE_UI_THEME_MODE_LIGHT };
	float scalars[] = { VOE_UI_THEME_SCALAR_MIN, 1.0f, VOE_UI_THEME_SCALAR_MAX };
	voe_math_float3 hues[] = {
		{ 0xD4 / 255.0f, 0xA0 / 255.0f, 0x2B / 255.0f },
		{ 0x2B / 255.0f, 0x7F / 255.0f, 0xD4 / 255.0f },
		{ 0x33 / 255.0f, 0xAA / 255.0f, 0x33 / 255.0f },
	};

	for (size_t m = 0; m < 2; m++)
		for (size_t c = 0; c < 3; c++)
			for (size_t s = 0; s < 3; s++)
				for (size_t h = 0; h < 3; h++)
					check_one_hue(modes[m], scalars[c], scalars[s], hues[h]);
}

static void check_no_chroma(voe_ui_theme_inputs inputs)
{
	voe_ui_theme theme = voe_ui_theme_derive(&inputs, NULL);
	voe_math_float4 roles[ROLE_COUNT];

	theme_roles(&theme, roles);

	for (size_t i = 0; i < ROLE_COUNT; i++)
		VOE_TEST_CHECK(role_chroma(roles[i]) < 0.002f);
}

// A grey hue gives a neutral theme — the built-in Near black and Near white
// (ADR-0194), which is why the default inputs are checked here as well.
static void test_grey_hue_is_grey(void)
{
	voe_ui_theme_mode modes[] = { VOE_UI_THEME_MODE_DARK, VOE_UI_THEME_MODE_LIGHT };

	for (size_t m = 0; m < 2; m++) {
		voe_ui_theme_inputs grey = voe_ui_theme_default_inputs();
		voe_ui_theme_inputs fallback = voe_ui_theme_default_inputs();

		grey.mode = modes[m];
		grey.hue = (voe_math_float3){ 0x80 / 255.0f, 0x80 / 255.0f, 0x80 / 255.0f };
		fallback.mode = modes[m];

		check_no_chroma(grey);
		check_no_chroma(fallback);
	}
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
	// inverse_ink is the ground's lightness, which contrast_strength never
	// touches; inverse is text_primary's and moves with it, by design.
	VOE_TEST_CHECK(float4_equal(tlo.inverse_ink, thi.inverse_ink));
	VOE_TEST_CHECK(!float4_equal(tlo.inverse, thi.inverse));

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
	VOE_TEST_CHECK(float4_equal(tlo.inverse, thi.inverse));
	VOE_TEST_CHECK(float4_equal(tlo.inverse_ink, thi.inverse_ink));

	VOE_TEST_CHECK(!float4_equal(tlo.surface, thi.surface));
	VOE_TEST_CHECK(!float4_equal(tlo.surface_raised, thi.surface_raised));
	VOE_TEST_CHECK(!float4_equal(tlo.control, thi.control));
	VOE_TEST_CHECK(!float4_equal(tlo.control_hovered, thi.control_hovered));
}

// ---- the dark/light chroma asymmetry ----

static void test_dark_clamps_chroma_harder_than_light(void)
{
	// Pure sRGB green: far outside both chroma ceilings, so both modes
	// clamp, and green is a hue the gamut still has room for at the light
	// surface's own lightness — which is what makes the light figure land
	// on the ceiling rather than on the gamut, so the case is about the two
	// ceilings and not about green.
	voe_math_float3 saturated = { 0.0f, 1.0f, 0.0f };

	voe_ui_theme_inputs dark = voe_ui_theme_default_inputs();
	voe_ui_theme_inputs light = voe_ui_theme_default_inputs();

	dark.hue = saturated;
	dark.mode = VOE_UI_THEME_MODE_DARK;
	light.hue = saturated;
	light.mode = VOE_UI_THEME_MODE_LIGHT;

	voe_ui_theme tdark = voe_ui_theme_derive(&dark, NULL);
	voe_ui_theme tlight = voe_ui_theme_derive(&light, NULL);

	float chroma_dark = role_chroma(tdark.surface);
	float chroma_light = role_chroma(tlight.surface);

	VOE_TEST_CHECK(chroma_dark < chroma_light);
	// The figures ADR-0097's bench found and ADR-0171 fixed as the rule:
	// the light surface keeps the whole light ceiling, and the dark one is
	// held under the dark ceiling — by it, and by the smaller gamut a dark
	// lightness has, both pushing the same way.
	VOE_TEST_CHECK(chroma_dark <= CHROMA_MAX_DARK + 0.002f);
	VOE_TEST_CHECK_FLOAT(chroma_light, CHROMA_MAX_LIGHT, 0.01);
}

int main(void)
{
	test_round_trip();
	test_legible_at_scalar_extremes();
	test_text_roles_ordered_at_reference();
	test_hue_moves_only_tint();
	test_every_role_shares_one_hue();
	test_grey_hue_is_grey();
	test_contrast_moves_only_text_and_border();
	test_separation_moves_only_the_ladder();
	test_dark_clamps_chroma_harder_than_light();

	return voe_test_result();
}
