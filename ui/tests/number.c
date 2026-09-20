// The number box's drag, and what is worth pinning down about it: what a
// sideways drag comes to, what a fine one comes to, that a drag goes on past
// the edge of what it started on, and that half a millimetre inside the dead
// zone is no drag at all. The press and the release a button answers — and what
// an inverted control draws, the dragged number box among them — are
// `ui/tests/button.c`; what every widget shares is `ui/tests/widgets.c`.
//
// EVERY DRAG CASE NEEDS NO GRAPHICS CARD AND NO WINDOW SYSTEM, which is the
// property the whole design is arranged around: the pointer is a value handed
// in (ADR-0093), so a drag below is three calls in a row and not a mouse. A
// number box draws its value only once it is open for typing, so nothing here
// touches a font, opens a window, asks a compositor for anything or names
// `platform`. Typing into a number box does need one and is `ui/tests/field.c`.
//
// THE VALUE IS HANDED IN EVERY FRAME AND NEVER KEPT. The widget is given
// `START` each frame and answers that plus THIS frame's drag, so a case that
// measured from the press rather than from last frame would make the value race
// away as the square of the gesture. `PER_MM` is two and not one so that a
// distance in millimetres and a value can never be confused for one another by
// coming out the same number.
//
// A DRAG SURVIVES ITS WIDGET LEAVING THE SCREEN, since spec 001: a clip is what
// can be seen and so what can be hit, but a number box scrolled out of sight
// mid-drag goes on reporting its drag, exactly as one dragged past the edge of
// the panel does.
//
// THE GEOMETRY IS WORKED OUT BY HAND AND WRITTEN AS NUMBERS. A number box's
// rectangle comes out of layout, so a test that asked layout where the box was
// and then pressed there would pass with the arithmetic inverted. The numbers
// below say where the box is meant to be.
#include <ui/layout.h>
#include <ui/widgets.h>

#include <base/arena.h>
#include <math/float2.h>

#include <testing/test.h>

#define SCRATCH 65536

// The box each number box below is built round: 20 by 10, which with the
// control's own 2.5 of padding on every side makes the box 25 x 15.
#define BOX_WIDE 20.0f
#define BOX_HIGH 10.0f

// One number box in a panel with no padding, so it is 25 x 15 at the origin and
// every x below is a millimetre the reader can place on it. The pointer's y
// never moves: a number box is dragged sideways and a vertical component of the
// gesture is not meant to reach the value at all.
#define NUMBER_WIDE 25.0f
#define NUMBER_HIGH 15.0f
#define ON_NUMBER_Y 7.0f

// Two units of value per millimetre of drag, so that a distance and a value
// cannot be confused for one another by coming out the same number — which they
// would at one.
#define PER_MM 2.0
// What the caller hands in every frame. The widget never keeps it, so every
// result below is this plus THIS frame's drag and never a running total.
#define START 100.0

// The theme every context in this file draws with, set once in main() — a panel
// and a number box each need one in force to be built at all.
static voe_ui_theme TEST_THEME;

struct number_frame {
	voe_ui_node n;
	bool ok;
};

static struct number_frame build_number(voe_ui_context *ui,
					voe_base_arena *arena, float x,
					bool over, bool down, bool fine,
					uint32_t boxes)
{
	struct number_frame f = { VOE_UI_NODE_NONE, false };

	voe_ui_frame_begin(ui, arena);
	voe_ui_pointer_set(ui, (voe_ui_pointer){ .at = { x, ON_NUMBER_Y },
						 .over = over,
						 .down = down,
						 .fine = fine });

	voe_ui_panel_begin(ui, "panel", 0, VOE_UI_SURFACE_SURFACE,
			   (voe_ui_container){ 0 });
	if (boxes > 0) {
		f.n = voe_ui_number_begin(ui, "n", 0, START, PER_MM);
		voe_ui_box(ui, (voe_math_float2){ BOX_WIDE, BOX_HIGH },
			   (voe_ui_sizing){ 0 });
		voe_ui_end(ui);
	}
	voe_ui_end(ui);

	f.ok = voe_ui_frame_end(ui);
	return f;
}

// The premise of every drag case below, checked once rather than assumed.
static void the_number_box_is_where_the_tests_think_it_is(
	voe_ui_context *ui, voe_base_arena *arena)
{
	struct number_frame f = build_number(ui, arena, 0.0f, false, false,
					     false, 1);
	voe_ui_rect r;

	VOE_TEST_CHECK(f.ok);
	r = voe_ui_node_rect(ui, f.n);

	VOE_TEST_CHECK_FLOAT(r.min.x, 0.0f, 0.001f);
	VOE_TEST_CHECK_FLOAT(r.min.y, 0.0f, 0.001f);
	VOE_TEST_CHECK_FLOAT(r.size.x, NUMBER_WIDE, 0.001f);
	VOE_TEST_CHECK_FLOAT(r.size.y, NUMBER_HIGH, 0.001f);
}

// THE CASE THE WHOLE WIDGET EXISTS FOR, AND THE TWO ORIGINS ARE THE POINT OF IT.
// Pressed at 0 and dragged to 10: the first change is what lies BEYOND the dead
// zone, so nine millimetres and not ten. Dragged on to 12: two more, measured
// from last frame and not from the press — measuring that one from the press
// would give twelve and the value would race away as the square of the gesture.
static void dragging_sideways_moves_the_value(voe_ui_context *ui,
					      voe_base_arena *arena)
{
	struct number_frame f = build_number(ui, arena, 0.0f, true, false,
					     false, 1);
	voe_ui_number_result r;

	// Nothing is held before the press, so the press below is an edge
	// and not whatever the case before this left behind.
	VOE_TEST_CHECK(!voe_ui_number_action(ui, f.n).held);

	// The frame that arms it has moved nothing yet.
	f = build_number(ui, arena, 0.0f, true, true, false, 1);
	r = voe_ui_number_action(ui, f.n);
	VOE_TEST_CHECK(r.held);
	VOE_TEST_CHECK(!r.changed);
	VOE_TEST_CHECK_FLOAT((float)r.value, (float)START, 0.001f);

	f = build_number(ui, arena, 10.0f, true, true, false, 1);
	r = voe_ui_number_action(ui, f.n);
	VOE_TEST_CHECK(r.changed);
	VOE_TEST_CHECK_FLOAT((float)r.value, (float)(START + 9.0 * PER_MM),
			     0.001f);

	f = build_number(ui, arena, 12.0f, true, true, false, 1);
	r = voe_ui_number_action(ui, f.n);
	VOE_TEST_CHECK(r.changed);
	// TWO more and not twelve: the value handed in plus THIS frame's drag.
	VOE_TEST_CHECK_FLOAT((float)r.value, (float)(START + 2.0 * PER_MM),
			     0.001f);

	// A frame that does not move reports no change, rather than repeating
	// the last one.
	f = build_number(ui, arena, 12.0f, true, true, false, 1);
	r = voe_ui_number_action(ui, f.n);
	VOE_TEST_CHECK(!r.changed);
	VOE_TEST_CHECK_FLOAT((float)r.value, (float)START, 0.001f);

	f = build_number(ui, arena, 12.0f, true, false, false, 1);
	VOE_TEST_CHECK(!voe_ui_number_action(ui, f.n).held);
}

// The fine modifier slows the value and NOT the dead zone: the same nine
// millimetres of travel, a tenth of the change. A fine drag that also had a
// tenth of the dead zone would begin sooner than an ordinary one, which is the
// opposite of what a person asking for precision wants.
static void a_fine_drag_moves_a_tenth_as_far(voe_ui_context *ui,
					     voe_base_arena *arena)
{
	struct number_frame f = build_number(ui, arena, 0.0f, true, false,
					     true, 1);
	voe_ui_number_result r;

	// Nothing is held before the press, so the press below is an edge
	// and not whatever the case before this left behind.
	VOE_TEST_CHECK(!voe_ui_number_action(ui, f.n).held);

	f = build_number(ui, arena, 0.0f, true, true, true, 1);
	VOE_TEST_CHECK(voe_ui_number_action(ui, f.n).held);

	f = build_number(ui, arena, 10.0f, true, true, true, 1);
	r = voe_ui_number_action(ui, f.n);
	VOE_TEST_CHECK(r.changed);
	VOE_TEST_CHECK_FLOAT((float)r.value,
			     (float)(START + 9.0 * PER_MM * VOE_UI_NUMBER_FINE),
			     0.001f);

	f = build_number(ui, arena, 10.0f, true, false, true, 1);
	VOE_TEST_CHECK(!voe_ui_number_action(ui, f.n).held);
}

// A DRAG DOES NOT STOP AT THE EDGE OF WHAT IT STARTED ON. The pointer is far to
// the right of the number box and well off the panel; `platform` goes on
// reporting it because a button is down, and the value goes on moving. A widget
// that only answered while the pointer was inside it would make a long drag stop
// dead, which reads as the interface having lost the mouse.
static void a_drag_past_the_edge_keeps_working(voe_ui_context *ui,
					       voe_base_arena *arena)
{
	struct number_frame f = build_number(ui, arena, 10.0f, true, false,
					     false, 1);
	voe_ui_number_result r;

	// Nothing is held before the press, so the press below is an edge
	// and not whatever the case before this left behind.
	VOE_TEST_CHECK(!voe_ui_number_action(ui, f.n).held);

	f = build_number(ui, arena, 10.0f, true, true, false, 1);
	VOE_TEST_CHECK(voe_ui_number_action(ui, f.n).held);

	f = build_number(ui, arena, 100.0f, true, true, false, 1);
	r = voe_ui_number_action(ui, f.n);

	// Not hovered — the pointer is nowhere near it — and still held and
	// still moving, which is the whole claim.
	VOE_TEST_CHECK(!r.hovered);
	VOE_TEST_CHECK(r.held);
	VOE_TEST_CHECK(r.changed);
	VOE_TEST_CHECK_FLOAT((float)r.value, (float)(START + 89.0 * PER_MM),
			     0.001f);

	f = build_number(ui, arena, 100.0f, true, false, false, 1);
	VOE_TEST_CHECK(!voe_ui_number_action(ui, f.n).held);
}

// The same rule a button follows: a gesture that began somewhere else is not a
// press on whatever it is dragged over.
static void arriving_with_the_button_already_down_arms_no_number(
	voe_ui_context *ui, voe_base_arena *arena)
{
	struct number_frame f = build_number(ui, arena, 100.0f, true, false,
					     false, 1);
	voe_ui_number_result r;

	// Nothing is held before the press, so the press below is an edge
	// and not whatever the case before this left behind.
	VOE_TEST_CHECK(!voe_ui_number_action(ui, f.n).held);

	// Down over nothing.
	f = build_number(ui, arena, 100.0f, true, true, false, 1);
	VOE_TEST_CHECK(!voe_ui_number_action(ui, f.n).held);

	// Dragged onto it and across it, still down. Nothing is armed, so
	// nothing changes however far it travels.
	f = build_number(ui, arena, 10.0f, true, true, false, 1);
	r = voe_ui_number_action(ui, f.n);
	VOE_TEST_CHECK(r.hovered);
	VOE_TEST_CHECK(!r.held);
	VOE_TEST_CHECK(!r.changed);

	f = build_number(ui, arena, 20.0f, true, true, false, 1);
	r = voe_ui_number_action(ui, f.n);
	VOE_TEST_CHECK(!r.changed);
	VOE_TEST_CHECK_FLOAT((float)r.value, (float)START, 0.001f);

	f = build_number(ui, arena, 20.0f, true, false, false, 1);
	VOE_TEST_CHECK(!voe_ui_number_action(ui, f.n).held);
}

// Dragged, then not built — a panel closed mid-gesture. The drag must end with
// it rather than wait to be finished by whatever next takes that key.
static void a_number_box_that_stops_being_called_is_let_go(
	voe_ui_context *ui, voe_base_arena *arena)
{
	struct number_frame f = build_number(ui, arena, 0.0f, true, false,
					     false, 1);
	voe_ui_number_result r;

	// Nothing is held before the press, so the press below is an edge
	// and not whatever the case before this left behind.
	VOE_TEST_CHECK(!voe_ui_number_action(ui, f.n).held);

	f = build_number(ui, arena, 0.0f, true, true, false, 1);
	VOE_TEST_CHECK(voe_ui_number_action(ui, f.n).held);

	f = build_number(ui, arena, 10.0f, true, true, false, 1);
	VOE_TEST_CHECK(voe_ui_number_action(ui, f.n).changed);

	// Gone, with the pointer still down.
	f = build_number(ui, arena, 12.0f, true, true, false, 0);
	VOE_TEST_CHECK_INT((int)f.n, (int)VOE_UI_NODE_NONE);

	// Back, still down and still moving. It must not be held and the
	// movement must not reach the value: the gesture ended when the widget
	// did.
	f = build_number(ui, arena, 20.0f, true, true, false, 1);
	r = voe_ui_number_action(ui, f.n);
	VOE_TEST_CHECK(!r.held);
	VOE_TEST_CHECK(!r.changed);
	VOE_TEST_CHECK_FLOAT((float)r.value, (float)START, 0.001f);
}

// A column 40 wide and 20 tall clipping Y and scrolled by `scroll`, holding the
// number box at the top and 100 of spacer under it — so it may scroll to 95, and
// at 50 the number box is wholly above the column's top.
static struct number_frame build_scrolled_number(voe_ui_context *ui,
						 voe_base_arena *arena, float x,
						 bool down, float scroll)
{
	struct number_frame f = { VOE_UI_NODE_NONE, false };

	voe_ui_frame_begin(ui, arena);
	voe_ui_pointer_set(ui, (voe_ui_pointer){ .at = { x, ON_NUMBER_Y },
						 .over = true,
						 .down = down });

	voe_ui_column_begin(ui, (voe_ui_container){
					.size = { { VOE_UI_SIZE_FIXED, 20.0f },
						  { VOE_UI_SIZE_FIXED, 40.0f } },
					.overflow = { VOE_UI_OVERFLOW_VISIBLE,
						      VOE_UI_OVERFLOW_CLIP },
					.scroll = { 0.0f, scroll } });
	f.n = voe_ui_number_begin(ui, "n", 0, START, PER_MM);
	voe_ui_box(ui, (voe_math_float2){ BOX_WIDE, BOX_HIGH },
		   (voe_ui_sizing){ 0 });
	voe_ui_end(ui);
	voe_ui_box(ui, (voe_math_float2){ 10.0f, 100.0f }, (voe_ui_sizing){ 0 });
	voe_ui_end(ui);

	f.ok = voe_ui_frame_end(ui);
	return f;
}

// A DRAG SURVIVES ITS WIDGET SCROLLING OUT OF SIGHT, as it survives the pointer
// leaving the surface. Pressed at 5, dragged to 15 — nine past the dead zone —
// then scrolled 50 so that nothing of the box is seen, and dragged on to 20: not
// hovered, still held, and five more millimetres of change.
static void a_number_box_scrolled_away_mid_drag_keeps_dragging(
	voe_ui_context *ui, voe_base_arena *arena)
{
	struct number_frame f =
		build_scrolled_number(ui, arena, 5.0f, false, 0.0f);
	voe_ui_number_result r;

	VOE_TEST_CHECK(!voe_ui_number_action(ui, f.n).held);

	f = build_scrolled_number(ui, arena, 5.0f, true, 0.0f);
	VOE_TEST_CHECK(voe_ui_number_action(ui, f.n).held);

	f = build_scrolled_number(ui, arena, 15.0f, true, 0.0f);
	r = voe_ui_number_action(ui, f.n);
	VOE_TEST_CHECK(r.changed);
	VOE_TEST_CHECK_FLOAT((float)r.value, (float)(START + 9.0 * PER_MM),
			     0.001f);

	f = build_scrolled_number(ui, arena, 20.0f, true, 50.0f);
	VOE_TEST_CHECK(f.ok);
	VOE_TEST_CHECK_FLOAT(voe_ui_node_visible(ui, f.n).size.y, 0.0f, 0.0);
	r = voe_ui_number_action(ui, f.n);
	VOE_TEST_CHECK(!r.hovered);
	VOE_TEST_CHECK(r.held);
	VOE_TEST_CHECK(r.changed);
	VOE_TEST_CHECK_FLOAT((float)r.value, (float)(START + 5.0 * PER_MM),
			     0.001f);

	f = build_scrolled_number(ui, arena, 20.0f, false, 50.0f);
	VOE_TEST_CHECK(!voe_ui_number_action(ui, f.n).held);
}

// Half a millimetre is not a drag, and this is what keeps a click a click. Two
// frames inside the zone, so that a slow hand crossing it in small steps is
// covered as well as one that never leaves it. Its release, inside the zone,
// opens the box for typing, and the next frame built on that context would
// compose a label and so want a font — which is why this case is the last one
// here, runs on a context of its own and has nothing run after it. What an
// open box does with the keyboard is `ui/tests/field.c`.
static void movement_inside_the_dead_zone_changes_nothing(
	voe_ui_context *ui, voe_base_arena *arena)
{
	struct number_frame f = build_number(ui, arena, 10.0f, true, false,
					     false, 1);
	voe_ui_number_result r;

	// Nothing is held before the press, so the press below is an edge
	// and not whatever the case before this left behind.
	VOE_TEST_CHECK(!voe_ui_number_action(ui, f.n).held);

	f = build_number(ui, arena, 10.0f, true, true, false, 1);
	VOE_TEST_CHECK(voe_ui_number_action(ui, f.n).held);

	f = build_number(ui, arena, 10.4f, true, true, false, 1);
	r = voe_ui_number_action(ui, f.n);
	VOE_TEST_CHECK(!r.changed);
	VOE_TEST_CHECK_FLOAT((float)r.value, (float)START, 0.001f);

	f = build_number(ui, arena, 10.8f, true, true, false, 1);
	r = voe_ui_number_action(ui, f.n);
	VOE_TEST_CHECK(!r.changed);
	VOE_TEST_CHECK_FLOAT((float)r.value, (float)START, 0.001f);

	f = build_number(ui, arena, 10.8f, true, false, false, 1);
	VOE_TEST_CHECK(!voe_ui_number_action(ui, f.n).held);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_ui_context *ui;
	voe_ui_theme_inputs inputs = voe_ui_theme_default_inputs();

	TEST_THEME = voe_ui_theme_derive(&inputs, NULL);

	ui = voe_ui_context_new(
		arena, (voe_ui_capacities){ .nodes = 64, .elements = 64 });
	voe_ui_theme_set(ui, &TEST_THEME);

	the_number_box_is_where_the_tests_think_it_is(ui, arena);
	dragging_sideways_moves_the_value(ui, arena);
	a_fine_drag_moves_a_tenth_as_far(ui, arena);
	a_drag_past_the_edge_keeps_working(ui, arena);
	arriving_with_the_button_already_down_arms_no_number(ui, arena);
	a_number_box_that_stops_being_called_is_let_go(ui, arena);
	a_number_box_scrolled_away_mid_drag_keeps_dragging(ui, arena);

	// A context of its own for the dead zone, thrown away with the box it
	// leaves open still open — see the case itself.
	ui = voe_ui_context_new(
		arena, (voe_ui_capacities){ .nodes = 16, .elements = 64 });
	voe_ui_theme_set(ui, &TEST_THEME);
	movement_inside_the_dead_zone_changes_nothing(ui, arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
