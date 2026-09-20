// The slider: where the thumb sits for a value, and what a drag across the
// track comes to. A slider is a number box with a thumb in it (ADR-0196), so
// the press, the release, the dead zone and the drag are all
// `ui/tests/button.c`'s already; what is new here and what this file pins down
// is the two ends of the range — the thumb's place in the track, and the clamp
// that keeps a hand from dragging the value out of it.
//
// NO GRAPHICS CARD AND NO WINDOW SYSTEM. Nothing here composes a label: a
// slider is a fill, a thumb and no letters, and a number box only draws its
// value once it is open for typing — which needs a font and is
// `ui/tests/field.c`. So every case below is the pointer handed in as a value
// (ADR-0093) and arithmetic checked against numbers worked out by hand.
//
// THE THUMB IS FOUND THROUGH THE ELEMENT RECORDS AND NOT THROUGH A HANDLE,
// because the call hands back the number box's node and the thumb has none of
// its own. That is not a workaround: the records are what a caller submits, so
// checking them is checking what is actually drawn. The slider costs three of
// them — the box's fill, then the thumb's border and its surface, a parent
// before its children — and a count that is not three would mean the tree is
// not the one the header promises.
#include <ui/layout.h>
#include <ui/slider.h>
#include <ui/widgets.h>

#include <base/arena.h>
#include <math/float2.h>
#include <render/device.h>

#include <testing/test.h>

#define SCRATCH 65536

// One slider in a bare column at the origin. A number box pads itself by 2.5
// on every side (it is built like a button), so a 40 x 4 track makes a 45 x 9
// slider with the track at (2.5, 2.5) inside it, and the thumb's left edge is
// 2.5 plus however far along the track it has got.
#define WIDE 40.0f
#define TRACK_X 2.5f
#define TRACK_Y 2.5f
#define SLIDER_WIDE 45.0f
#define SLIDER_HIGH 9.0f

// The range. A hundred over forty millimetres is two and a half units of value
// per millimetre of drag, which is not one — so a distance and a value cannot
// be confused for each other by coming out the same number.
#define MIN 0.0
#define MAX 100.0
#define PER_MM ((MAX - MIN) / (double)WIDE)

// How far the thumb's left edge can travel: the track less the thumb.
#define TRAVEL (WIDE - VOE_UI_SLIDER_THUMB)

// A y inside the slider, which never moves: a number box is dragged sideways
// and the vertical part of a gesture is not meant to reach the value.
#define ON_Y 4.5f

static voe_ui_theme TEST_THEME;

struct frame {
	voe_ui_node s;
	bool ok;
};

static struct frame build(voe_ui_context *ui, voe_base_arena *arena,
			  double value, float x, bool over, bool down)
{
	struct frame f = { VOE_UI_NODE_NONE, false };

	voe_ui_frame_begin(ui, arena);
	voe_ui_pointer_set(ui, (voe_ui_pointer){ .at = { x, ON_Y },
						 .over = over,
						 .down = down });

	voe_ui_column_begin(ui, (voe_ui_container){ 0 });
	f.s = voe_ui_slider(ui, "s", 0, value, MIN, MAX, WIDE);
	voe_ui_end(ui);

	f.ok = voe_ui_frame_end(ui);
	return f;
}

// The thumb's rectangle: the second record, the raised panel's border, which
// is at the panel's own bounds before its surface is inset from them.
static voe_math_float4 thumb(voe_ui_context *ui)
{
	VOE_TEST_CHECK(voe_ui_element_count(ui) == 3);
	if (voe_ui_element_count(ui) < 2)
		return (voe_math_float4){ 0, 0, 0, 0 };

	return voe_ui_element(ui, 1).bounds;
}

// The premise of everything below, checked once rather than assumed: the
// slider is where the arithmetic says, and it is three records.
static void the_slider_is_where_the_test_thinks_it_is(voe_ui_context *ui,
						      voe_base_arena *arena)
{
	struct frame f = build(ui, arena, MIN, 0.0f, false, false);
	voe_ui_rect r;

	VOE_TEST_CHECK(f.ok);
	r = voe_ui_node_rect(ui, f.s);

	VOE_TEST_CHECK_FLOAT(r.min.x, 0.0f, 0.001f);
	VOE_TEST_CHECK_FLOAT(r.min.y, 0.0f, 0.001f);
	VOE_TEST_CHECK_FLOAT(r.size.x, SLIDER_WIDE, 0.001f);
	VOE_TEST_CHECK_FLOAT(r.size.y, SLIDER_HIGH, 0.001f);
	VOE_TEST_CHECK(voe_ui_element_count(ui) == 3);
}

// THE CASE THE WIDGET EXISTS FOR: the value is legible as a position. At `min`
// the thumb is against the track's left edge, at `max` its far edge is against
// the right one — which is the travel being the track less the thumb and not
// the track — and halfway is halfway between those two.
static void the_thumb_sits_where_the_value_is(voe_ui_context *ui,
					      voe_base_arena *arena)
{
	voe_math_float4 t;

	(void)build(ui, arena, MIN, 0.0f, false, false);
	t = thumb(ui);
	VOE_TEST_CHECK_FLOAT(t.x, TRACK_X, 0.001f);
	VOE_TEST_CHECK_FLOAT(t.y, TRACK_Y, 0.001f);
	VOE_TEST_CHECK_FLOAT(t.z, VOE_UI_SLIDER_THUMB, 0.001f);
	VOE_TEST_CHECK_FLOAT(t.w, VOE_UI_SLIDER_HEIGHT, 0.001f);

	(void)build(ui, arena, MAX, 0.0f, false, false);
	t = thumb(ui);
	VOE_TEST_CHECK_FLOAT(t.x, TRACK_X + TRAVEL, 0.001f);
	VOE_TEST_CHECK_FLOAT(t.x + t.z, TRACK_X + WIDE, 0.001f);

	(void)build(ui, arena, (MIN + MAX) / 2.0, 0.0f, false, false);
	t = thumb(ui);
	VOE_TEST_CHECK_FLOAT(t.x, TRACK_X + TRAVEL / 2.0f, 0.001f);
}

// A drag across the whole track is the whole range, which is what makes the
// thumb keep up with the pointer. Pressed at 10 and dragged to 21: eleven
// millimetres, of which the first is the dead zone, so ten of travel and
// twenty-five of value. Then two more, measured from last frame and not from
// the press.
static void a_sideways_drag_moves_the_value(voe_ui_context *ui,
					    voe_base_arena *arena)
{
	struct frame f = build(ui, arena, 50.0, 10.0f, true, false);
	voe_ui_slider_result r;

	// Up first, so the press below is an edge and not what a case before
	// this one left behind.
	VOE_TEST_CHECK(!voe_ui_slider_action(ui, f.s, MIN, MAX).held);

	f = build(ui, arena, 50.0, 10.0f, true, true);
	r = voe_ui_slider_action(ui, f.s, MIN, MAX);
	VOE_TEST_CHECK(r.held);
	VOE_TEST_CHECK(!r.changed);
	VOE_TEST_CHECK_FLOAT((float)r.value, 50.0f, 0.001f);

	f = build(ui, arena, 50.0, 21.0f, true, true);
	r = voe_ui_slider_action(ui, f.s, MIN, MAX);
	VOE_TEST_CHECK(r.changed);
	VOE_TEST_CHECK_FLOAT((float)r.value, (float)(50.0 + 10.0 * PER_MM),
			     0.001f);

	f = build(ui, arena, 50.0, 23.0f, true, true);
	r = voe_ui_slider_action(ui, f.s, MIN, MAX);
	VOE_TEST_CHECK(r.changed);
	VOE_TEST_CHECK_FLOAT((float)r.value, (float)(50.0 + 2.0 * PER_MM),
			     0.001f);

	// Released far from the press, so nothing opens for typing.
	f = build(ui, arena, 50.0, 23.0f, true, false);
	VOE_TEST_CHECK(!voe_ui_slider_action(ui, f.s, MIN, MAX).held);
}

// THE CLAMP IS THE WHOLE REASON THE ACTION TAKES THE RANGE. A number box knows
// no limits, so a hand that keeps going past the end would run the value away
// and the slider would take a drag back before the thumb moved at all. It
// answers exactly the bound — not near it — at either end.
static void a_drag_past_an_end_stops_at_it(voe_ui_context *ui,
					   voe_base_arena *arena)
{
	struct frame f = build(ui, arena, MAX, 10.0f, true, false);
	voe_ui_slider_result r;

	VOE_TEST_CHECK(!voe_ui_slider_action(ui, f.s, MIN, MAX).held);

	f = build(ui, arena, MAX, 10.0f, true, true);
	VOE_TEST_CHECK(voe_ui_slider_action(ui, f.s, MIN, MAX).held);

	// Thirty millimetres to the right of a slider already at its top.
	f = build(ui, arena, MAX, 40.0f, true, true);
	r = voe_ui_slider_action(ui, f.s, MIN, MAX);
	VOE_TEST_CHECK(r.value == MAX);
	// And the thumb has not walked off the end with it.
	VOE_TEST_CHECK_FLOAT(thumb(ui).x, TRACK_X + TRAVEL, 0.001f);

	f = build(ui, arena, MAX, 40.0f, true, false);
	VOE_TEST_CHECK(!voe_ui_slider_action(ui, f.s, MIN, MAX).held);

	// The same at the other end, dragged leftwards from a slider at its
	// bottom.
	f = build(ui, arena, MIN, 30.0f, true, true);
	VOE_TEST_CHECK(voe_ui_slider_action(ui, f.s, MIN, MAX).held);

	f = build(ui, arena, MIN, 0.0f, true, true);
	r = voe_ui_slider_action(ui, f.s, MIN, MAX);
	VOE_TEST_CHECK(r.value == MIN);
	VOE_TEST_CHECK_FLOAT(thumb(ui).x, TRACK_X, 0.001f);

	f = build(ui, arena, MIN, 0.0f, true, false);
	VOE_TEST_CHECK(!voe_ui_slider_action(ui, f.s, MIN, MAX).held);
}

// A frame nobody touched hands the value straight back, so a caller may write
// it back every frame and see nothing move. A value from outside the range is
// the one exception and it is a change: it comes back at the bound and says
// so, or a caller that writes back only on `changed` would keep an impossible
// value for ever.
static void an_untouched_slider_answers_what_it_was_given(
	voe_ui_context *ui, voe_base_arena *arena)
{
	struct frame f = build(ui, arena, 42.0, 0.0f, false, false);
	voe_ui_slider_result r = voe_ui_slider_action(ui, f.s, MIN, MAX);

	VOE_TEST_CHECK(f.ok);
	VOE_TEST_CHECK(!r.held);
	VOE_TEST_CHECK(!r.changed);
	VOE_TEST_CHECK_FLOAT((float)r.value, 42.0f, 0.001f);

	f = build(ui, arena, MAX + 10.0, 0.0f, false, false);
	r = voe_ui_slider_action(ui, f.s, MIN, MAX);
	VOE_TEST_CHECK(r.changed);
	VOE_TEST_CHECK(r.value == MAX);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_ui_theme_inputs inputs = voe_ui_theme_default_inputs();
	voe_ui_context *ui;

	TEST_THEME = voe_ui_theme_derive(&inputs, NULL);

	ui = voe_ui_context_new(
		arena, (voe_ui_capacities){ .nodes = 16, .elements = 64 });
	voe_ui_theme_set(ui, &TEST_THEME);

	the_slider_is_where_the_test_thinks_it_is(ui, arena);
	the_thumb_sits_where_the_value_is(ui, arena);
	a_sideways_drag_moves_the_value(ui, arena);
	a_drag_past_an_end_stops_at_it(ui, arena);
	an_untouched_slider_answers_what_it_was_given(ui, arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
