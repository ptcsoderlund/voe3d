// The swatch and the colour picker: that a swatch is one record of its colour,
// that a press in the square, a typed hex and a press outside each give the
// answer worked out by hand, that a bad hex is refused leaving the colour, and
// that a colour made grey keeps its hue into the next frame.
//
// THE SWATCH NEEDS NOTHING; THE PICKER NEEDS A FONT, because its hex field
// composes a label, and a font needs a headless device. Where there is no
// driver the picker cases skip, saying so (ADR-0106).
//
// THE GEOMETRY IS BY HAND. The picker is the root at the origin, padding 2 and
// gap 2, so its square is 32 x 32 at (2, 2), its strip 32 x 4 at (2, 36) and
// its hex field 32 wide at (2, 42).
#include <ui/colour.h>
#include <ui/layout.h>
#include <ui/theme.h>
#include <ui/widgets.h>

#include <base/arena.h>
#include <math/float2.h>
#include <math/float3.h>
#include <render/device.h>
#include <text/font.h>

#include <testing/test.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

#define SCRATCH 65536

#define SQUARE_TOP_RIGHT ((voe_math_float2){ 33.999f, 2.0f })
#define SQUARE_LEFT_MIDDLE ((voe_math_float2){ 2.0f, 18.0f })
#define IN_HEX ((voe_math_float2){ 10.0f, 45.0f })
#define OUTSIDE ((voe_math_float2){ 100.0f, 100.0f })

static voe_ui_theme TEST_THEME;

// The sRGB transfer function worked out here rather than asked of `ui`, so a
// wrong curve there cannot agree with itself.
static float linear(float srgb)
{
	return srgb <= 0.04045f ? srgb / 12.92f
				: powf((srgb + 0.055f) / 1.055f, 2.4f);
}

static void check_colour(voe_math_float3 got, float r, float g, float b)
{
	VOE_TEST_CHECK_FLOAT(got.x, r, 1e-3f);
	VOE_TEST_CHECK_FLOAT(got.y, g, 1e-3f);
	VOE_TEST_CHECK_FLOAT(got.z, b, 1e-3f);
}

static void a_swatch_emits_one_element_of_its_colour(voe_base_arena *arena)
{
	voe_ui_context *ui = voe_ui_context_new(
		arena, (voe_ui_capacities){ .nodes = 8, .elements = 8 });
	voe_render_element e;

	voe_ui_frame_begin(ui, arena);
	voe_ui_column_begin(ui, (voe_ui_container){ 0 });
	voe_ui_swatch(ui, (voe_math_float3){ 0.2f, 0.4f, 0.6f },
		      (voe_ui_sizing){ { VOE_UI_SIZE_FIXED, 10.0f },
				       { VOE_UI_SIZE_FIXED, 10.0f } });
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));

	VOE_TEST_CHECK_INT(voe_ui_element_count(ui), 1);
	e = voe_ui_element(ui, 0);
	VOE_TEST_CHECK_FLOAT(e.colour.x, 0.2f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(e.colour.y, 0.4f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(e.colour.z, 0.6f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(e.colour.w, 1.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(e.bounds.z, 10.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(e.bounds.w, 10.0f, 1e-6f);
}

// One frame of a picker showing `colour`, rewinding the frame's arena first.
static voe_ui_colour_result picker_frame(voe_ui_context *ui,
					 voe_base_arena *frame,
					 voe_math_float3 colour,
					 voe_ui_pointer pointer,
					 voe_ui_keyboard keyboard)
{
	voe_ui_node p;

	voe_base_arena_clear(frame);
	voe_ui_frame_begin(ui, frame);
	voe_ui_pointer_set(ui, pointer);
	voe_ui_keyboard_set(ui, keyboard);
	p = voe_ui_colour_picker(ui, "colour", 0, colour);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));
	return voe_ui_colour_picker_action(ui, p);
}

static voe_ui_pointer press_at(voe_math_float2 at)
{
	return (voe_ui_pointer){ .at = at, .over = true, .down = true };
}

static voe_ui_pointer up_at(voe_math_float2 at)
{
	return (voe_ui_pointer){ .at = at, .over = true };
}

static const voe_ui_keyboard NO_KEYS = { 0 };

static void a_press_at_the_top_right_with_hue_nought_is_red(
	voe_ui_context *ui, voe_base_arena *frame)
{
	voe_math_float3 grey = { 0.2f, 0.2f, 0.2f };
	voe_ui_colour_result r;

	r = picker_frame(ui, frame, grey, up_at(OUTSIDE), NO_KEYS);
	VOE_TEST_CHECK(!r.changed);
	check_colour(r.value, 0.2f, 0.2f, 0.2f);

	r = picker_frame(ui, frame, grey, press_at(SQUARE_TOP_RIGHT), NO_KEYS);
	VOE_TEST_CHECK(r.changed);
	VOE_TEST_CHECK(!r.outside);
	check_colour(r.value, 1.0f, 0.0f, 0.0f);
	picker_frame(ui, frame, r.value, up_at(SQUARE_TOP_RIGHT), NO_KEYS);
}

// Focus the hex field by a press and a release, then type `text` and Enter.
static voe_ui_colour_result type_hex(voe_ui_context *ui, voe_base_arena *frame,
				     voe_math_float3 colour, const char *text)
{
	picker_frame(ui, frame, colour, press_at(IN_HEX), NO_KEYS);
	picker_frame(ui, frame, colour, up_at(IN_HEX), NO_KEYS);
	return picker_frame(ui, frame, colour, up_at(IN_HEX),
			    (voe_ui_keyboard){ .text = text,
					       .size = (uint32_t)strlen(text),
					       .enter = true });
}

static void a_typed_hex_is_taken_in_either_form(voe_ui_context *ui,
						voe_base_arena *frame)
{
	voe_math_float3 red = { 1.0f, 0.0f, 0.0f };
	voe_ui_colour_result r;

	r = type_hex(ui, frame, red, "#FFC800");
	VOE_TEST_CHECK(r.changed);
	VOE_TEST_CHECK(!r.refused);
	check_colour(r.value, 1.0f, linear(200.0f / 255.0f), 0.0f);

	r = type_hex(ui, frame, red, "ffc800");
	VOE_TEST_CHECK(r.changed);
	VOE_TEST_CHECK(!r.refused);
	check_colour(r.value, 1.0f, linear(200.0f / 255.0f), 0.0f);
}

static void a_short_hex_is_refused_and_the_colour_stays(voe_ui_context *ui,
							voe_base_arena *frame)
{
	voe_math_float3 red = { 1.0f, 0.0f, 0.0f };
	voe_ui_colour_result r = type_hex(ui, frame, red, "#12");
	uint32_t before;

	VOE_TEST_CHECK(r.refused);
	VOE_TEST_CHECK(!r.changed);
	check_colour(r.value, 1.0f, 0.0f, 0.0f);

	// The next frame says so: "not #RRGGBB" adds its letters.
	picker_frame(ui, frame, red, up_at(OUTSIDE), NO_KEYS);
	before = voe_ui_element_count(ui);
	r = type_hex(ui, frame, red, "#FF0000");
	VOE_TEST_CHECK(!r.refused);
	picker_frame(ui, frame, red, up_at(OUTSIDE), NO_KEYS);
	VOE_TEST_CHECK(voe_ui_element_count(ui) < before);
}

static void a_press_outside_says_outside(voe_ui_context *ui,
					 voe_base_arena *frame)
{
	voe_math_float3 red = { 1.0f, 0.0f, 0.0f };
	voe_ui_colour_result r;

	r = picker_frame(ui, frame, red, press_at(OUTSIDE), NO_KEYS);
	VOE_TEST_CHECK(r.outside);
	VOE_TEST_CHECK(!r.changed);
	// Only on the frame it went down.
	r = picker_frame(ui, frame, red, press_at(OUTSIDE), NO_KEYS);
	VOE_TEST_CHECK(!r.outside);
	picker_frame(ui, frame, red, up_at(OUTSIDE), NO_KEYS);
}

// Green, dragged to the square's left edge, is a grey; handed back as that
// grey the next frame, a press at the top-right is green again and not red.
static void a_grey_keeps_its_hue_into_the_next_frame(voe_ui_context *ui,
						     voe_base_arena *frame)
{
	voe_math_float3 green = { 0.0f, 1.0f, 0.0f };
	voe_ui_colour_result r;

	picker_frame(ui, frame, green, up_at(OUTSIDE), NO_KEYS);
	r = picker_frame(ui, frame, green, press_at(SQUARE_LEFT_MIDDLE),
			 NO_KEYS);
	VOE_TEST_CHECK(r.changed);
	VOE_TEST_CHECK_FLOAT(r.value.x, r.value.y, 1e-6f);
	VOE_TEST_CHECK_FLOAT(r.value.y, r.value.z, 1e-6f);

	r = picker_frame(ui, frame, r.value, up_at(OUTSIDE), NO_KEYS);
	r = picker_frame(ui, frame, r.value, press_at(SQUARE_TOP_RIGHT),
			 NO_KEYS);
	check_colour(r.value, 0.0f, 1.0f, 0.0f);
	picker_frame(ui, frame, r.value, up_at(OUTSIDE), NO_KEYS);
}

static int the_picker(voe_base_arena *arena)
{
	voe_platform_size size = { 64, 64 };
	// The smallest numbers a device opens with: it exists so a font can
	// upload its atlas, and nothing here draws.
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
	voe_base_arena *frame;
	voe_base_error error = VOE_BASE_OK;

	device = voe_render_device_new_headless(arena, size, capacities,
						&error);
	if (device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED) {
			printf("skip: no graphics driver — the colour picker "
			       "checks did not run\n");
			return 0;
		}
		VOE_TEST_CHECK(device != NULL);
		return 0;
	}
	font = voe_text_font_new(VOE_TEXT_TYPEFACE_OXANIUM, device, arena,
				 &error);
	if (font == NULL) {
		VOE_TEST_CHECK(font != NULL);
		voe_render_device_destroy(device);
		return 0;
	}

	frame = voe_base_arena_new(SCRATCH);
	ui = voe_ui_context_new(arena, (voe_ui_capacities){ .nodes = 16,
							    .elements = 600 });
	voe_ui_font_set(ui, font);
	voe_ui_theme_set(ui, &TEST_THEME);

	a_press_at_the_top_right_with_hue_nought_is_red(ui, frame);
	a_typed_hex_is_taken_in_either_form(ui, frame);
	a_short_hex_is_refused_and_the_colour_stays(ui, frame);
	a_press_outside_says_outside(ui, frame);
	a_grey_keeps_its_hue_into_the_next_frame(ui, frame);

	voe_base_arena_destroy(frame);
	voe_text_font_destroy(font);
	voe_render_device_destroy(device);
	return 0;
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_ui_theme_inputs inputs = voe_ui_theme_default_inputs();

	TEST_THEME = voe_ui_theme_derive(&inputs, NULL);

	a_swatch_emits_one_element_of_its_colour(arena);
	(void)the_picker(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
