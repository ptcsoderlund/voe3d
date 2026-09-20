// The scroll area: what it remembers, what it passes on, and the bar it draws.
// The button under that bar is a button and its own gestures are
// `ui/tests/button.c`; what every widget shares is `ui/tests/widgets.c`.
//
// EVERY CASE HERE NEEDS NO GRAPHICS CARD AND NO WINDOW SYSTEM, which is the
// property the whole design is arranged around: the pointer and its wheel are
// values handed in (ADR-0093), so a scroll is two calls in a row and not a
// mouse. Nothing in this file measures a string, so nothing here touches a
// font, opens a window, asks a compositor for anything or names `platform`.
//
// A SCROLL AREA REMEMBERS, PASSES ON AND DRAWS A BAR, since spec 001. A scroll
// lands in the next frame and is clamped; an area skipped for a frame forgets; an
// inner area takes what it can and hands the rest to the one around it; the bar
// is absent when content fits — including when it fits only because it wrapped,
// which is the case that catches an ancestor's stale measure — has a thumb of
// known length at known offsets,
// drags and pages by known amounts and stands in front of a button; and one area
// too many refuses the frame. Every one of these makes its own context, so what
// one remembers cannot leak into the next.
//
// THE GEOMETRY IS WORKED OUT BY HAND AND WRITTEN AS NUMBERS. The bar and its
// thumb come out of layout, so a test that asked layout where the thumb was and
// then dragged it would pass with the arithmetic inverted. The numbers below
// say where the area, the bar and the thumb are meant to be.
#include <ui/layout.h>
#include <ui/widgets.h>

#include <base/arena.h>
#include <math/float2.h>
#include <math/float4.h>

#include <testing/test.h>

#include <stdio.h>

#define SCRATCH 65536

// The height of the box the area's button holds — the 10 of the 35 x 10 below.
#define BOX_HIGH 10.0f

// A scroll area at the origin, 40 wide and 30 tall, scrolling Y, with no padding
// and no gap, over content 100 tall — so its range is 70:
//
//   area   column, 30 along (Y) and 40 across (X)
//     either  a box 20 x `content_high`
//     or      button "under" round a 35 x 10 box  ->  40 x 15 at (0, 0)
//             and a spacer 10 x 85 under it
//
// Its one bar is along the right edge: a track 1.5 wide at x 38.5 and 30 tall,
// and a thumb 30 x 30 / 100 = 9 tall at (30 - 9) x offset / 70.
#define AREA_WIDE 40.0f
#define AREA_HIGH 30.0f
#define BAR_LEFT 38.5f
#define BAR_WIDE 1.5f
#define THUMB_HIGH 9.0f
// A point across the bar, and one across the area clear of it.
#define ON_BAR_X 39.0f
#define OFF_BAR_X 20.0f

struct area_frame {
	voe_ui_node area;
	voe_ui_node first;
	bool ok;
};

// The theme every context in this file draws with, set once in main() — a
// scroll area's bar and the button beneath it each need one in force, and the
// bar's track and thumb draw from its roles.
static voe_ui_theme TEST_THEME;

static voe_ui_context *scroll_context(voe_base_arena *arena, uint32_t scrolls)
{
	voe_ui_context *ui =
		voe_ui_context_new(arena, (voe_ui_capacities){
						   .nodes = 64,
						   .elements = 64,
						   .scrolls = scrolls });

	voe_ui_theme_set(ui, &TEST_THEME);
	return ui;
}

static voe_ui_pointer pointer_at(float x, float y, bool down, float scroll_y)
{
	return (voe_ui_pointer){ .at = { x, y },
				 .over = true,
				 .down = down,
				 .scroll = { 0.0f, scroll_y } };
}

static struct area_frame build_area(voe_ui_context *ui, voe_base_arena *arena,
				    voe_ui_pointer pointer, float content_high,
				    bool with_button)
{
	struct area_frame f = { VOE_UI_NODE_NONE, VOE_UI_NODE_NONE, false };

	voe_ui_frame_begin(ui, arena);
	voe_ui_pointer_set(ui, pointer);

	f.area = voe_ui_scroll_begin(
		ui, "area", 0,
		(voe_ui_container){ .size = { { VOE_UI_SIZE_FIXED, AREA_HIGH },
					      { VOE_UI_SIZE_FIXED, AREA_WIDE } } },
		(voe_ui_scroll_axes){ .y = true });
	if (with_button) {
		f.first = voe_ui_button_begin(ui, "under", 0);
		voe_ui_box(ui, (voe_math_float2){ 35.0f, BOX_HIGH },
			   (voe_ui_sizing){ 0 });
		voe_ui_end(ui);
		voe_ui_box(ui, (voe_math_float2){ 10.0f, 85.0f },
			   (voe_ui_sizing){ 0 });
	} else {
		f.first = voe_ui_box(ui, (voe_math_float2){ 20.0f, content_high },
				     (voe_ui_sizing){ 0 });
	}
	voe_ui_end(ui);

	f.ok = voe_ui_frame_end(ui);
	return f;
}

// The same frame with no scroll area in it at all.
static bool build_no_area(voe_ui_context *ui, voe_base_arena *arena)
{
	voe_ui_frame_begin(ui, arena);
	voe_ui_column_begin(ui, (voe_ui_container){ 0 });
	voe_ui_box(ui, (voe_math_float2){ 20.0f, 100.0f }, (voe_ui_sizing){ 0 });
	voe_ui_end(ui);
	return voe_ui_frame_end(ui);
}

// A SCROLL LANDS IN THE NEXT FRAME, CLAMPED. Ten millimetres: the frame that was
// handed them is where it was, and the next one has its content 10 higher. Five
// hundred: the frame after is at the range, 70, and not past it.
static void a_scroll_moves_the_next_frames_content(voe_base_arena *arena)
{
	voe_ui_context *ui = scroll_context(arena, 4);
	struct area_frame f =
		build_area(ui, arena, pointer_at(10.0f, 10.0f, false, 10.0f),
			   100.0f, false);

	VOE_TEST_CHECK(f.ok);
	VOE_TEST_CHECK_FLOAT(voe_ui_node_rect(ui, f.first).min.y, 0.0f, 0.001f);

	f = build_area(ui, arena, pointer_at(10.0f, 10.0f, false, 500.0f),
		       100.0f, false);
	VOE_TEST_CHECK_FLOAT(voe_ui_node_rect(ui, f.first).min.y, -10.0f,
			     0.001f);
	VOE_TEST_CHECK_FLOAT(voe_ui_node_scroll(ui, f.area).y, 10.0f, 0.001f);

	f = build_area(ui, arena, pointer_at(10.0f, 10.0f, false, 0.0f), 100.0f,
		       false);
	VOE_TEST_CHECK_FLOAT(voe_ui_node_rect(ui, f.first).min.y, -70.0f,
			     0.001f);
	VOE_TEST_CHECK_FLOAT(voe_ui_node_scroll(ui, f.area).y, 70.0f, 0.001f);
}

// A frame that does not call the area drops what it remembered, so the area
// comes back at the top.
static void an_area_skipped_for_a_frame_forgets(voe_base_arena *arena)
{
	voe_ui_context *ui = scroll_context(arena, 4);
	struct area_frame f;

	(void)build_area(ui, arena, pointer_at(10.0f, 10.0f, false, 10.0f),
			 100.0f, false);
	f = build_area(ui, arena, pointer_at(10.0f, 10.0f, false, 0.0f), 100.0f,
		       false);
	VOE_TEST_CHECK_FLOAT(voe_ui_node_scroll(ui, f.area).y, 10.0f, 0.001f);

	VOE_TEST_CHECK(build_no_area(ui, arena));

	f = build_area(ui, arena, pointer_at(10.0f, 10.0f, false, 0.0f), 100.0f,
		       false);
	VOE_TEST_CHECK(f.ok);
	VOE_TEST_CHECK_FLOAT(voe_ui_node_scroll(ui, f.area).y, 0.0f, 0.001f);
}

// An area inside an area, the pointer over the inner one:
//
//   outer  column 30 along (Y), 40 across (X), scrolling both
//     inner  column 20 along, 40 across, over a 20 x 30 box  ->  Y range 10
//     box    100 x 100                                        ->  outer's Y range
//                                                                 120 - 30 = 90,
//                                                                 X range 60
struct nested_frame {
	voe_ui_node outer;
	voe_ui_node inner;
};

static struct nested_frame build_nested(voe_ui_context *ui,
					voe_base_arena *arena,
					voe_math_float2 scroll,
					voe_ui_scroll_axes inner_axes)
{
	struct nested_frame f;

	voe_ui_frame_begin(ui, arena);
	voe_ui_pointer_set(ui, (voe_ui_pointer){ .at = { 5.0f, 5.0f },
						 .over = true,
						 .scroll = scroll });

	f.outer = voe_ui_scroll_begin(
		ui, "outer", 0,
		(voe_ui_container){ .size = { { VOE_UI_SIZE_FIXED, AREA_HIGH },
					      { VOE_UI_SIZE_FIXED, AREA_WIDE } } },
		(voe_ui_scroll_axes){ .x = true, .y = true });
	f.inner = voe_ui_scroll_begin(
		ui, "inner", 0,
		(voe_ui_container){ .size = { { VOE_UI_SIZE_FIXED, 20.0f },
					      { VOE_UI_SIZE_FIXED, AREA_WIDE } } },
		inner_axes);
	voe_ui_box(ui, (voe_math_float2){ 20.0f, 30.0f }, (voe_ui_sizing){ 0 });
	voe_ui_end(ui);
	voe_ui_box(ui, (voe_math_float2){ 100.0f, 100.0f }, (voe_ui_sizing){ 0 });
	voe_ui_end(ui);

	VOE_TEST_CHECK(voe_ui_frame_end(ui));
	return f;
}

static void a_nested_area_passes_on_what_it_cannot_take(voe_base_arena *arena)
{
	voe_ui_scroll_axes both = { .x = true, .y = true };
	voe_ui_scroll_axes only_y = { .y = true };
	voe_math_float2 none = { 0.0f, 0.0f };
	voe_ui_context *ui = scroll_context(arena, 4);
	struct nested_frame f;

	// WITH RANGE LEFT THE INNER TAKES IT ALL and the outer is unmoved.
	(void)build_nested(ui, arena, (voe_math_float2){ 0.0f, 5.0f }, both);
	f = build_nested(ui, arena, none, both);
	VOE_TEST_CHECK_FLOAT(voe_ui_node_scroll(ui, f.inner).y, 5.0f, 0.001f);
	VOE_TEST_CHECK_FLOAT(voe_ui_node_scroll(ui, f.outer).y, 0.0f, 0.001f);

	// AT ITS END IT HANDS THE REST OUTWARD: five more brings the inner to
	// its end at 10, and twenty-five after that all goes to the outer.
	(void)build_nested(ui, arena, (voe_math_float2){ 0.0f, 5.0f }, both);
	(void)build_nested(ui, arena, (voe_math_float2){ 0.0f, 25.0f }, both);
	f = build_nested(ui, arena, none, both);
	VOE_TEST_CHECK_FLOAT(voe_ui_node_scroll(ui, f.inner).y, 10.0f, 0.001f);
	VOE_TEST_CHECK_FLOAT(voe_ui_node_scroll(ui, f.outer).y, 25.0f, 0.001f);

	// AN AXIS THE INNER DOES NOT SCROLL PASSES THROUGH IT WHOLE.
	ui = scroll_context(arena, 4);
	(void)build_nested(ui, arena, (voe_math_float2){ 7.0f, 0.0f }, only_y);
	f = build_nested(ui, arena, none, only_y);
	VOE_TEST_CHECK_FLOAT(voe_ui_node_scroll(ui, f.inner).x, 0.0f, 0.001f);
	VOE_TEST_CHECK_FLOAT(voe_ui_node_scroll(ui, f.outer).x, 7.0f, 0.001f);
}

// No bar when there is nothing to scroll: content 20 tall in an area 30 tall,
// and neither the area nor its box draws anything else.
//
// AND NO BAR WHEN THE CONTENT FITS BECAUSE IT WRAPPED, which is the same
// statement about a number that is only right after every wrap is decided. An
// area 40 by 30 filling its content across, holding a wrapping row of three
// boxes 15 by 8: the row is stretched to 40, breaks into a line of two and a
// line of one, and so measures 30 by 16 — both inside the area. Before the
// corrective sweep in layout the area still held the one line the X pass
// measured, 45, and drew a horizontal track and thumb over 5 mm of slack that
// was not there.
static void content_that_fits_has_no_bar(voe_base_arena *arena)
{
	voe_ui_context *ui = scroll_context(arena, 4);
	struct area_frame f =
		build_area(ui, arena, pointer_at(10.0f, 10.0f, false, 0.0f),
			   20.0f, false);
	voe_ui_node area;

	VOE_TEST_CHECK(f.ok);
	VOE_TEST_CHECK_INT((int)voe_ui_element_count(ui), 0);

	ui = scroll_context(arena, 4);
	voe_ui_frame_begin(ui, arena);
	voe_ui_pointer_set(ui, pointer_at(10.0f, 10.0f, false, 0.0f));
	area = voe_ui_scroll_begin(
		ui, "wrapping", 0,
		(voe_ui_container){ .size = { { VOE_UI_SIZE_FIXED, AREA_HIGH },
					      { VOE_UI_SIZE_FIXED, AREA_WIDE } },
				    .across = VOE_UI_ACROSS_FILL },
		(voe_ui_scroll_axes){ .x = true, .y = true });
	voe_ui_row_begin(ui, (voe_ui_container){ .wrap = true });
	for (uint32_t i = 0; i < 3; i++)
		voe_ui_box(ui, (voe_math_float2){ 15.0f, 8.0f },
			   (voe_ui_sizing){ 0 });
	voe_ui_end(ui);
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));

	VOE_TEST_CHECK_FLOAT(voe_ui_node_measured(ui, area).x, 30.0f, 0.001f);
	VOE_TEST_CHECK_FLOAT(voe_ui_node_measured(ui, area).y, 16.0f, 0.001f);
	VOE_TEST_CHECK_INT((int)voe_ui_element_count(ui), 0);
}

// The thumb record at one offset: the track then the thumb, and nothing else.
static void check_thumb(voe_ui_context *ui, float y)
{
	voe_render_element track;
	voe_render_element thumb;

	VOE_TEST_CHECK_INT((int)voe_ui_element_count(ui), 2);
	if (voe_ui_element_count(ui) != 2)
		return;

	track = voe_ui_element(ui, 0);
	thumb = voe_ui_element(ui, 1);
	VOE_TEST_CHECK_FLOAT(track.bounds.x, BAR_LEFT, 0.001f);
	VOE_TEST_CHECK_FLOAT(track.bounds.y, 0.0f, 0.001f);
	VOE_TEST_CHECK_FLOAT(track.bounds.z, BAR_WIDE, 0.001f);
	VOE_TEST_CHECK_FLOAT(track.bounds.w, AREA_HIGH, 0.001f);
	VOE_TEST_CHECK_FLOAT(thumb.bounds.x, BAR_LEFT, 0.001f);
	VOE_TEST_CHECK_FLOAT(thumb.bounds.y, y, 0.001f);
	VOE_TEST_CHECK_FLOAT(thumb.bounds.z, BAR_WIDE, 0.001f);
	VOE_TEST_CHECK_FLOAT(thumb.bounds.w, THUMB_HIGH, 0.001f);
	// Clipped to the area, which holds all of it.
	VOE_TEST_CHECK_FLOAT(thumb.clip.w, THUMB_HIGH, 0.001f);
}

// Nine tall wherever it is, and at 0, (30 - 9) x 35 / 70 = 10.5 and 21.
static void the_thumb_is_as_long_and_as_far_as_the_offset_says(
	voe_base_arena *arena)
{
	voe_ui_context *ui = scroll_context(arena, 4);

	(void)build_area(ui, arena, pointer_at(10.0f, 10.0f, false, 35.0f),
			 100.0f, false);
	check_thumb(ui, 0.0f);

	(void)build_area(ui, arena, pointer_at(10.0f, 10.0f, false, 35.0f),
			 100.0f, false);
	check_thumb(ui, 10.5f);

	(void)build_area(ui, arena, pointer_at(10.0f, 10.0f, false, 0.0f),
			 100.0f, false);
	check_thumb(ui, 21.0f);
}

// The colour of the thumb record — the second of the bar's two, the track
// being the first.
static void check_thumb_colour(voe_ui_context *ui, voe_math_float4 want)
{
	voe_render_element thumb;

	VOE_TEST_CHECK_INT((int)voe_ui_element_count(ui), 2);
	if (voe_ui_element_count(ui) != 2)
		return;

	thumb = voe_ui_element(ui, 1);
	VOE_TEST_CHECK_FLOAT(thumb.colour.x, want.x, 0.001f);
	VOE_TEST_CHECK_FLOAT(thumb.colour.y, want.y, 0.001f);
	VOE_TEST_CHECK_FLOAT(thumb.colour.z, want.z, 0.001f);
}

// A HELD THUMB IS DRAWN INVERTED (ADR-0196), as every other held control is.
// Pressed on the thumb, which stands at the top of the track, and kept down:
// `inverse`. Let go with the pointer clear of the bar: the control it is at
// rest, which is what makes the first check a state and not the only colour a
// thumb has.
static void a_held_thumb_is_inverted(voe_base_arena *arena)
{
	voe_ui_context *ui = scroll_context(arena, 4);

	(void)build_area(ui, arena, pointer_at(ON_BAR_X, 4.0f, false, 0.0f),
			 100.0f, false);
	(void)build_area(ui, arena, pointer_at(ON_BAR_X, 4.0f, true, 0.0f),
			 100.0f, false);
	check_thumb_colour(ui, TEST_THEME.inverse);

	(void)build_area(ui, arena, pointer_at(OFF_BAR_X, 4.0f, false, 0.0f),
			 100.0f, false);
	check_thumb_colour(ui, TEST_THEME.control);
}

// Pressed on the thumb and moved 3 mm down: 3 x 100 / 30 is 10 of offset.
static void dragging_the_thumb_moves_the_offset(voe_base_arena *arena)
{
	voe_ui_context *ui = scroll_context(arena, 4);
	struct area_frame f;

	(void)build_area(ui, arena, pointer_at(ON_BAR_X, 4.0f, false, 0.0f),
			 100.0f, false);
	(void)build_area(ui, arena, pointer_at(ON_BAR_X, 4.0f, true, 0.0f),
			 100.0f, false);
	(void)build_area(ui, arena, pointer_at(ON_BAR_X, 7.0f, true, 0.0f),
			 100.0f, false);
	f = build_area(ui, arena, pointer_at(ON_BAR_X, 7.0f, true, 0.0f),
		       100.0f, false);
	VOE_TEST_CHECK_FLOAT(voe_ui_node_scroll(ui, f.area).y, 10.0f, 0.001f);
	VOE_TEST_CHECK_FLOAT(voe_ui_node_rect(ui, f.first).min.y, -10.0f,
			     0.001f);

	f = build_area(ui, arena, pointer_at(ON_BAR_X, 7.0f, false, 0.0f),
		       100.0f, false);
	VOE_TEST_CHECK_FLOAT(voe_ui_node_scroll(ui, f.area).y, 10.0f, 0.001f);
}

// A press on the track below the thumb pages one arranged length, 30, and holding
// the button down on the track does not page again.
static void pressing_the_track_pages_once(voe_base_arena *arena)
{
	voe_ui_context *ui = scroll_context(arena, 4);
	struct area_frame f;

	(void)build_area(ui, arena, pointer_at(ON_BAR_X, 20.0f, false, 0.0f),
			 100.0f, false);
	(void)build_area(ui, arena, pointer_at(ON_BAR_X, 20.0f, true, 0.0f),
			 100.0f, false);
	f = build_area(ui, arena, pointer_at(ON_BAR_X, 20.0f, true, 0.0f),
		       100.0f, false);
	VOE_TEST_CHECK_FLOAT(voe_ui_node_scroll(ui, f.area).y, 30.0f, 0.001f);

	f = build_area(ui, arena, pointer_at(ON_BAR_X, 20.0f, true, 0.0f),
		       100.0f, false);
	VOE_TEST_CHECK_FLOAT(voe_ui_node_scroll(ui, f.area).y, 30.0f, 0.001f);

	f = build_area(ui, arena, pointer_at(ON_BAR_X, 20.0f, false, 0.0f),
		       100.0f, false);
	VOE_TEST_CHECK_FLOAT(voe_ui_node_scroll(ui, f.area).y, 30.0f, 0.001f);
}

// THE BAR IS IN FRONT FOR THE POINTER. The button runs under the bar; clear of the
// bar the button is hovered, and on the thumb it is neither hovered nor armed.
static void a_thumb_hides_the_button_beneath_it(voe_base_arena *arena)
{
	voe_ui_context *ui = scroll_context(arena, 4);
	struct area_frame f =
		build_area(ui, arena, pointer_at(OFF_BAR_X, 4.0f, false, 0.0f),
			   0.0f, true);

	VOE_TEST_CHECK(voe_ui_button_action(ui, f.first).hovered);

	f = build_area(ui, arena, pointer_at(ON_BAR_X, 4.0f, false, 0.0f), 0.0f,
		       true);
	VOE_TEST_CHECK(!voe_ui_button_action(ui, f.first).hovered);

	f = build_area(ui, arena, pointer_at(ON_BAR_X, 4.0f, true, 0.0f), 0.0f,
		       true);
	VOE_TEST_CHECK(!voe_ui_button_action(ui, f.first).held);

	(void)build_area(ui, arena, pointer_at(ON_BAR_X, 4.0f, false, 0.0f),
			 0.0f, true);
}

// Room for one scroll area and two called: refused, and the next frame with one
// is fine.
static void too_many_scroll_areas_refuses_the_frame(voe_base_arena *arena)
{
	voe_ui_context *ui = scroll_context(arena, 1);
	voe_ui_container area = { .size = { { VOE_UI_SIZE_FIXED, 10.0f },
					    { VOE_UI_SIZE_FIXED, 10.0f } } };

	fprintf(stderr, "-- the next voe_ui line is this test's own --\n");

	voe_ui_frame_begin(ui, arena);
	voe_ui_column_begin(ui, (voe_ui_container){ 0 });
	voe_ui_scroll_begin(ui, "one", 0, area, (voe_ui_scroll_axes){ .y = true });
	voe_ui_end(ui);
	voe_ui_scroll_begin(ui, "two", 0, area, (voe_ui_scroll_axes){ .y = true });
	voe_ui_end(ui);
	voe_ui_end(ui);
	VOE_TEST_CHECK(!voe_ui_frame_end(ui));

	voe_ui_frame_begin(ui, arena);
	voe_ui_column_begin(ui, (voe_ui_container){ 0 });
	voe_ui_scroll_begin(ui, "one", 0, area, (voe_ui_scroll_axes){ .y = true });
	voe_ui_end(ui);
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_ui_theme_inputs inputs = voe_ui_theme_default_inputs();

	TEST_THEME = voe_ui_theme_derive(&inputs, NULL);

	a_scroll_moves_the_next_frames_content(arena);
	an_area_skipped_for_a_frame_forgets(arena);
	a_nested_area_passes_on_what_it_cannot_take(arena);
	content_that_fits_has_no_bar(arena);
	the_thumb_is_as_long_and_as_far_as_the_offset_says(arena);
	a_held_thumb_is_inverted(arena);
	dragging_the_thumb_moves_the_offset(arena);
	pressing_the_track_pages_once(arena);
	a_thumb_hides_the_button_beneath_it(arena);
	too_many_scroll_areas_refuses_the_frame(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
