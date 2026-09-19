// voe_theme_read's tests: a good file's every value, the fallbacks when `font`
// and `text_size` are absent, any `font` reading as Oxanium, every refusal
// the header lists with the line the kept error names, and 006's criterion 10 — a light theme's text read here,
// derived by `ui` on a real font, deciding the colours a SURFACE panel, a
// label and a button emit, with no editor anywhere.
//
// The criterion case needs a headless device (ADR-0176): a label and a button
// measure a string, a NULL font asserts, and a font uploads an atlas. Where
// there is no driver it skips, naming what went unchecked, exactly as
// ui/tests/widgets.c does. Every other case is text in and values out.
#include <theme/theme.h>

#include <base/arena.h>
#include <base/report.h>
#include <render/device.h>
#include <ui/layout.h>
#include <ui/widgets.h>

#include <testing/test.h>

#include <stdio.h>
#include <string.h>

#define SCRATCH 65536

// Reads `text` with the kept error cleared first, so a refusal's message is
// this read's own.
static bool read_text(const char *text, voe_base_arena *arena, voe_theme *out)
{
	voe_base_report_error_clear();
	return voe_theme_read(text, strlen(text), arena, out);
}

static void a_good_file_gives_every_value(voe_base_arena *arena)
{
	voe_theme theme;

	VOE_TEST_CHECK(read_text("// a comment first\n"
				 "[HarbourLight]\n"
				 "accent=\"#FF8000\"\n"
				 "contrast_strength=1.25\n"
				 "surface_separation=0.5\n"
				 "mode=light\n"
				 "font=pixel_operator\n"
				 "text_size=4.5\n",
				 arena, &theme));
	VOE_TEST_CHECK(strcmp(theme.name, "HarbourLight") == 0);
	VOE_TEST_CHECK_FLOAT(theme.inputs.accent.x, 1.0f, 0.0001f);
	VOE_TEST_CHECK_FLOAT(theme.inputs.accent.y, 128.0f / 255.0f, 0.0001f);
	VOE_TEST_CHECK_FLOAT(theme.inputs.accent.z, 0.0f, 0.0001f);
	VOE_TEST_CHECK_FLOAT(theme.inputs.contrast_strength, 1.25f, 0.0001f);
	VOE_TEST_CHECK_FLOAT(theme.inputs.surface_separation, 0.5f, 0.0001f);
	VOE_TEST_CHECK_INT((int)theme.inputs.mode,
			   (int)VOE_UI_THEME_MODE_LIGHT);
	VOE_TEST_CHECK_INT((int)theme.typeface,
			   (int)VOE_TEXT_TYPEFACE_OXANIUM);
	VOE_TEST_CHECK_FLOAT(theme.inputs.text_size, 4.5f, 0.0001f);
}

static void absent_font_and_text_size_fall_back(voe_base_arena *arena)
{
	voe_theme theme;

	VOE_TEST_CHECK(read_text("[Night]\n"
				 "accent=#2b7fd4\n"
				 "contrast_strength=1\n"
				 "surface_separation=3\n"
				 "mode=dark\n",
				 arena, &theme));
	VOE_TEST_CHECK_INT((int)theme.inputs.mode, (int)VOE_UI_THEME_MODE_DARK);
	VOE_TEST_CHECK_INT((int)theme.typeface,
			   (int)VOE_TEXT_TYPEFACE_OXANIUM);
	VOE_TEST_CHECK_FLOAT(theme.inputs.text_size,
			     voe_ui_theme_default_inputs().text_size, 0.0001f);
}

// Refused, `out` untouched, and the kept error names `line`.
static void refused_at(const char *text, const char *line,
		       voe_base_arena *arena)
{
	voe_theme theme = { .name = "untouched" };
	const char *error;

	VOE_TEST_CHECK(!read_text(text, arena, &theme));
	VOE_TEST_CHECK(strcmp(theme.name, "untouched") == 0);
	error = voe_base_report_error_first();
	VOE_TEST_CHECK(error != NULL && strstr(error, line) != NULL);
	if (error == NULL || strstr(error, line) == NULL)
		fprintf(stderr, "      expected '%s' in: %s\n", line,
			error == NULL ? "(no error)" : error);
}

// The four required keys, then whatever the case adds.
#define GOOD_KEYS                    \
	"accent=#112233\n"           \
	"contrast_strength=1\n"      \
	"surface_separation=1\n"     \
	"mode=dark\n"

// Any `font` value, in an otherwise good file, reads as Oxanium and reports
// nothing (ADR-0185).
static void any_font_reads_as_oxanium(voe_base_arena *arena)
{
	const char *const files[] = {
		"[One]\n" GOOD_KEYS "font=pixel_operator\n",
		"[One]\n" GOOD_KEYS "font=comic_sans\n",
	};

	for (size_t i = 0; i < sizeof files / sizeof files[0]; i++) {
		voe_theme theme;

		VOE_TEST_CHECK(read_text(files[i], arena, &theme));
		VOE_TEST_CHECK_INT((int)theme.typeface,
				   (int)VOE_TEXT_TYPEFACE_OXANIUM);
		VOE_TEST_CHECK(voe_base_report_error_first() == NULL);
	}
}

static void every_refusal_names_its_line(voe_base_arena *arena)
{
	fprintf(stderr, "-- the voe theme lines that follow are this test's "
			"own --\n");

	refused_at("// only a comment\n", "line 1:", arena);
	refused_at("[One]\n" GOOD_KEYS "[Two]\n" GOOD_KEYS, "line 6:", arena);
	// A missing key names the section's line.
	refused_at("\n\n[One]\n"
		   "accent=#112233\n"
		   "contrast_strength=1\n"
		   "mode=dark\n",
		   "line 3:", arena);
	refused_at("[One]\n" GOOD_KEYS "colour=red\n", "line 6:", arena);
	refused_at("[One]\naccent=#11223\ncontrast_strength=1\n"
		   "surface_separation=1\nmode=dark\n",
		   "line 2:", arena);
	refused_at("[One]\naccent=#11223g\ncontrast_strength=1\n"
		   "surface_separation=1\nmode=dark\n",
		   "line 2:", arena);
	refused_at("[One]\naccent=#112233\ncontrast_strength=1x\n"
		   "surface_separation=1\nmode=dark\n",
		   "line 3:", arena);
	refused_at("[One]\naccent=#112233\ncontrast_strength=1\n"
		   "surface_separation=1\nmode=dusk\n",
		   "line 5:", arena);
	refused_at("[One]\n" GOOD_KEYS "text_size=0\n", "line 6:", arena);
	refused_at("[One]\n" GOOD_KEYS "text_size=big\n", "line 6:", arena);
	// Out of range at either end.
	refused_at("[One]\naccent=#112233\ncontrast_strength=0.1\n"
		   "surface_separation=1\nmode=dark\n",
		   "line 3:", arena);
	refused_at("[One]\naccent=#112233\ncontrast_strength=1\n"
		   "surface_separation=3.5\nmode=dark\n",
		   "line 4:", arena);
}

// Criterion 10. A light theme read from text, derived on a real font, set on a
// context; one frame of a SURFACE panel holding a label and a button holding
// its own label. The records, in order: the panel's border and fill, the
// label's two letters, the button's border and fill, the button label's two
// letters — and every fill and letter is the role of the palette this text
// derived to.
static void a_read_theme_colours_a_panel_a_label_and_a_button(
	voe_base_arena *arena)
{
	voe_platform_size size = { 64, 64 };
	voe_render_capacities capacities = {
		.vertices = 4,
		.indices = 6,
		.geometries = 1,
		.objects = 1,
		.shadings = 1,
		.passes = 1,
	};
	voe_render_device *device;
	voe_text_font *font;
	voe_ui_context *ui;
	voe_base_error error = VOE_BASE_OK;
	voe_theme read;
	voe_ui_theme palette;
	voe_ui_theme dark;
	voe_ui_theme_inputs defaults = voe_ui_theme_default_inputs();

	VOE_TEST_CHECK(read_text("[Paper]\n"
				 "accent=#2B7FD4\n"
				 "contrast_strength=1\n"
				 "surface_separation=1\n"
				 "mode=light\n",
				 arena, &read));

	device = voe_render_device_new_headless(arena, size, capacities,
						&error);
	if (device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED) {
			// ADR-0106: a skip names what went unchecked.
			printf("skip: no graphics driver — the check that a "
			       "read theme colours a panel, a label and a "
			       "button did not run\n");
			return;
		}
		VOE_TEST_CHECK(device != NULL);
		return;
	}
	font = voe_text_font_new(read.typeface, device, arena, &error);
	if (font == NULL) {
		VOE_TEST_CHECK(font != NULL);
		voe_render_device_destroy(device);
		return;
	}

	palette = voe_ui_theme_derive(&read.inputs, font);
	// The light theme is not the built-in dark one, so equal colours below
	// cannot be the default palette passing by accident.
	dark = voe_ui_theme_derive(&defaults, font);
	VOE_TEST_CHECK(palette.surface.x != dark.surface.x);

	ui = voe_ui_context_new(arena, (voe_ui_capacities){ .nodes = 16,
							    .elements = 16 });
	voe_ui_font_set(ui, font);
	voe_ui_theme_set(ui, &palette);

	voe_ui_frame_begin(ui, arena);
	voe_ui_pointer_set(ui, (voe_ui_pointer){ .over = false });
	voe_ui_panel_begin(ui, "panel", 0, VOE_UI_SURFACE_SURFACE,
			   (voe_ui_container){ .gap = 2.0f });
	voe_ui_label(ui, "Hi");
	voe_ui_button_begin(ui, "ok", 0);
	voe_ui_label(ui, "OK");
	voe_ui_end(ui);
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));

	VOE_TEST_CHECK_INT((int)voe_ui_element_count(ui), 8);
	if (voe_ui_element_count(ui) == 8) {
		struct {
			uint32_t index;
			voe_math_float4 colour;
		} expected[] = {
			{ 0, palette.border },	     { 1, palette.surface },
			{ 2, palette.text_primary }, { 3, palette.text_primary },
			{ 4, palette.border },	     { 5, palette.control },
			{ 6, palette.text_primary }, { 7, palette.text_primary },
		};

		for (size_t i = 0; i < sizeof expected / sizeof expected[0];
		     i++) {
			voe_math_float4 got =
				voe_ui_element(ui, expected[i].index).colour;

			VOE_TEST_CHECK_FLOAT(got.x, expected[i].colour.x,
					     0.0001f);
			VOE_TEST_CHECK_FLOAT(got.y, expected[i].colour.y,
					     0.0001f);
			VOE_TEST_CHECK_FLOAT(got.z, expected[i].colour.z,
					     0.0001f);
		}
	}

	voe_text_font_destroy(font);
	voe_render_device_destroy(device);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);

	a_good_file_gives_every_value(arena);
	absent_font_and_text_size_fall_back(arena);
	any_font_reads_as_oxanium(arena);
	every_refusal_names_its_line(arena);
	a_read_theme_colours_a_panel_a_label_and_a_button(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
