// The folding section: that its header row sits above its body inside its
// box, that a folded one is its header row and its pad, that the fold button
// fires and a title choice beside it fires only itself, that sections nest,
// and that one name under two indexes is two sections.
//
// EVERY CASE NEEDS A FONT, because the fold button holds a label, and a font
// needs a device: they take a headless one, as `ui/tests/widgets.c`'s label
// cases do, and where there is no driver they skip, saying which checks did
// not run (ADR-0106). The pointer is a value handed in (ADR-0093), so nothing
// here opens a window.
//
// THE PAD IS WRITTEN AS A NUMBER, 2 millimetres at the default theme's
// spacing of one, so a box that came out a pad short says so.
#include <ui/layout.h>
#include <ui/widgets.h>

#include <base/arena.h>
#include <math/float2.h>
#include <render/device.h>
#include <text/font.h>

#include <testing/test.h>

#include <stdio.h>

#define SCRATCH 262144
#define PAD 2.0f
#define ROOT_WIDE 100.0f
#define EPS 0.001f

// The theme every context in this file draws with, set once in main().
static voe_ui_theme TEST_THEME;

// Whether `inner` lies wholly inside `outer`.
static bool rect_within(voe_ui_rect outer, voe_ui_rect inner)
{
	return inner.min.x >= outer.min.x - EPS &&
	       inner.min.y >= outer.min.y - EPS &&
	       inner.min.x + inner.size.x <=
		       outer.min.x + outer.size.x + EPS &&
	       inner.min.y + inner.size.y <= outer.min.y + outer.size.y + EPS;
}

static float bottom(voe_ui_rect r)
{
	return r.min.y + r.size.y;
}

static voe_math_float2 centre(voe_ui_rect r)
{
	return (voe_math_float2){ r.min.x + r.size.x * 0.5f,
				  r.min.y + r.size.y * 0.5f };
}

// The root every case builds in: a column ROOT_WIDE across, filling its
// children to that width.
static void root_begin(voe_ui_context *ui)
{
	voe_ui_column_begin(ui, (voe_ui_container){
					.size = { .across = { VOE_UI_SIZE_FIXED,
							      ROOT_WIDE } },
					.across = VOE_UI_ACROSS_FILL });
}

// An open section: the header row — the fold button and the title — above two
// body rows, and all of it inside the box.
static void an_open_section_has_its_header_above_its_body(
	voe_ui_context *ui, voe_base_arena *arena)
{
	voe_ui_section s;
	voe_ui_node title;
	voe_ui_node first;
	voe_ui_node second;
	voe_ui_rect box;

	voe_ui_frame_begin(ui, arena);
	root_begin(ui);
	s = voe_ui_section_begin(ui, "open", 0, true);
	title = voe_ui_label_role(ui, "Open", VOE_UI_TEXT_ROLE_HEADING);
	voe_ui_end(ui);
	first = voe_ui_box(ui, (voe_math_float2){ 20.0f, 6.0f },
			   (voe_ui_sizing){ 0 });
	second = voe_ui_box(ui, (voe_math_float2){ 20.0f, 6.0f },
			    (voe_ui_sizing){ 0 });
	voe_ui_end(ui);
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));

	box = voe_ui_node_rect(ui, s.box);
	VOE_TEST_CHECK_FLOAT(box.size.x, ROOT_WIDE, EPS);

	VOE_TEST_CHECK(bottom(voe_ui_node_rect(ui, s.fold)) <=
		       voe_ui_node_rect(ui, first).min.y);
	VOE_TEST_CHECK(bottom(voe_ui_node_rect(ui, title)) <=
		       voe_ui_node_rect(ui, first).min.y);
	VOE_TEST_CHECK(bottom(voe_ui_node_rect(ui, first)) <=
		       voe_ui_node_rect(ui, second).min.y);

	VOE_TEST_CHECK(rect_within(box, voe_ui_node_rect(ui, s.fold)));
	VOE_TEST_CHECK(rect_within(box, voe_ui_node_rect(ui, title)));
	VOE_TEST_CHECK(rect_within(box, voe_ui_node_rect(ui, first)));
	VOE_TEST_CHECK(rect_within(box, voe_ui_node_rect(ui, second)));
}

// A folded section with no body is as tall as its header row — the taller of
// the fold button and the title — plus its pad above and below.
static void a_folded_section_is_its_header_and_its_pad(voe_ui_context *ui,
						       voe_base_arena *arena)
{
	voe_ui_section s;
	voe_ui_node title;
	float fold_high;
	float title_high;
	float header_high;

	voe_ui_frame_begin(ui, arena);
	root_begin(ui);
	s = voe_ui_section_begin(ui, "folded", 0, false);
	title = voe_ui_label_role(ui, "Folded", VOE_UI_TEXT_ROLE_HEADING);
	voe_ui_end(ui);
	voe_ui_end(ui);
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));

	fold_high = voe_ui_node_rect(ui, s.fold).size.y;
	title_high = voe_ui_node_rect(ui, title).size.y;
	header_high = fold_high > title_high ? fold_high : title_high;

	VOE_TEST_CHECK(header_high > 0.0f);
	VOE_TEST_CHECK_FLOAT(voe_ui_node_rect(ui, s.box).size.y,
			     header_high + 2.0f * PAD, EPS);
}

struct titled {
	voe_ui_section section;
	voe_ui_node title;
};

// A section whose title is a choice holding a heading, with the pointer
// wherever the caller says.
static struct titled build_titled(voe_ui_context *ui, voe_base_arena *arena,
				  voe_math_float2 at, bool down)
{
	struct titled t;

	voe_ui_frame_begin(ui, arena);
	voe_ui_pointer_set(ui, (voe_ui_pointer){ .at = at,
						 .over = true,
						 .down = down });
	root_begin(ui);
	t.section = voe_ui_section_begin(ui, "kind", 0, true);
	t.title = voe_ui_choice_begin(ui, "title", 0, false);
	voe_ui_label_role(ui, "Kind", VOE_UI_TEXT_ROLE_HEADING);
	voe_ui_end(ui);
	voe_ui_end(ui);
	voe_ui_end(ui);
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));
	return t;
}

// A press and a release on the fold button fire it and not the title; a
// press and a release on the title choice fire it and not the fold button.
static void the_fold_and_the_title_fire_apart(voe_ui_context *ui,
					      voe_base_arena *arena)
{
	struct titled t = build_titled(ui, arena, (voe_math_float2){ 0 },
				       false);
	voe_math_float2 on_fold = centre(voe_ui_node_rect(ui, t.section.fold));
	voe_math_float2 on_title = centre(voe_ui_node_rect(ui, t.title));

	(void)build_titled(ui, arena, on_fold, true);
	t = build_titled(ui, arena, on_fold, false);
	VOE_TEST_CHECK(voe_ui_button_action(ui, t.section.fold).fired);
	VOE_TEST_CHECK(!voe_ui_button_action(ui, t.title).fired);

	(void)build_titled(ui, arena, on_title, true);
	t = build_titled(ui, arena, on_title, false);
	VOE_TEST_CHECK(voe_ui_button_action(ui, t.title).fired);
	VOE_TEST_CHECK(!voe_ui_button_action(ui, t.section.fold).fired);
}

// A section in another's body lies inside the outer box, and takes its width
// less the outer pad on both sides.
static void a_nested_section_lies_inside_the_outer(voe_ui_context *ui,
						   voe_base_arena *arena)
{
	voe_ui_section outer;
	voe_ui_section inner;
	voe_ui_rect outer_box;
	voe_ui_rect inner_box;

	voe_ui_frame_begin(ui, arena);
	root_begin(ui);
	outer = voe_ui_section_begin(ui, "outer", 0, true);
	voe_ui_label_role(ui, "Outer", VOE_UI_TEXT_ROLE_HEADING);
	voe_ui_end(ui);
	inner = voe_ui_section_begin(ui, "inner", 0, true);
	voe_ui_label_role(ui, "Inner", VOE_UI_TEXT_ROLE_HEADING);
	voe_ui_end(ui);
	voe_ui_box(ui, (voe_math_float2){ 20.0f, 6.0f }, (voe_ui_sizing){ 0 });
	voe_ui_end(ui);
	voe_ui_end(ui);
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));

	outer_box = voe_ui_node_rect(ui, outer.box);
	inner_box = voe_ui_node_rect(ui, inner.box);

	VOE_TEST_CHECK(rect_within(outer_box, inner_box));
	VOE_TEST_CHECK(inner_box.min.y >=
		       bottom(voe_ui_node_rect(ui, outer.fold)));
	VOE_TEST_CHECK_FLOAT(inner_box.size.x, outer_box.size.x - 2.0f * PAD,
			     EPS);
}

// One name under two indexes is two sections, each with its own fold button
// keyed inside it, and the frame is not refused.
static void one_name_two_indexes_is_two_sections(voe_ui_context *ui,
						 voe_base_arena *arena)
{
	voe_ui_section first;
	voe_ui_section second;

	voe_ui_frame_begin(ui, arena);
	root_begin(ui);
	for (uint32_t i = 0; i < 2; i++) {
		voe_ui_section s = voe_ui_section_begin(ui, "kind", i, false);

		voe_ui_label_role(ui, "Kind", VOE_UI_TEXT_ROLE_HEADING);
		voe_ui_end(ui);
		voe_ui_end(ui);
		if (i == 0)
			first = s;
		else
			second = s;
	}
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));

	VOE_TEST_CHECK(voe_ui_node_rect(ui, second.box).min.y >=
		       bottom(voe_ui_node_rect(ui, first.box)));
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_ui_theme_inputs inputs = voe_ui_theme_default_inputs();
	voe_platform_size size = { 64, 64 };
	// The smallest numbers a device opens with: it exists only so that a
	// font can upload its atlas.
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

	TEST_THEME = voe_ui_theme_derive(&inputs, NULL);

	device = voe_render_device_new_headless(arena, size, capacities,
						&error);
	if (device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED) {
			printf("skip: no graphics driver — every section "
			       "check did not run\n");
			voe_base_arena_destroy(arena);
			return 0;
		}
		VOE_TEST_CHECK(device != NULL);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}

	// Oxanium: nothing here cares which face, so the engine's default.
	font = voe_text_font_new(VOE_TEXT_TYPEFACE_OXANIUM, device, arena,
				 &error);
	if (font == NULL) {
		VOE_TEST_CHECK(font != NULL);
		voe_render_device_destroy(device);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}

	ui = voe_ui_context_new(arena, (voe_ui_capacities){ .nodes = 64,
							    .elements = 256 });
	voe_ui_font_set(ui, font);
	voe_ui_theme_set(ui, &TEST_THEME);

	an_open_section_has_its_header_above_its_body(ui, arena);
	a_folded_section_is_its_header_and_its_pad(ui, arena);
	the_fold_and_the_title_fire_apart(ui, arena);
	a_nested_section_lies_inside_the_outer(ui, arena);
	one_name_two_indexes_is_two_sections(ui, arena);

	voe_text_font_destroy(font);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
