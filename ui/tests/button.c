// The button and the number box, and what is worth pinning down about them:
// what a press and a release do in every awkward order a hand can produce, what
// a sideways drag on a number box comes to, and that a clip decides both what
// is drawn and what can be hit. What every widget shares — keys, themes,
// panels, labels and the known list — is `ui/tests/widgets.c`.
//
// EVERY CLICK AND DRAG CASE NEEDS NO GRAPHICS CARD AND NO WINDOW SYSTEM, which
// is the property the whole design is arranged around: the pointer is a value
// handed in (ADR-0093), so a drag is three calls in a row and not a mouse. A
// button is composed rather than given a string and a number box draws its
// value only once it is open for typing, so none of them touches a font, opens
// a window, asks a compositor for anything or names `platform`. Typing into a
// number box does need one and is `ui/tests/field.c`.
//
// STATE IS DRAWN INVERTED (ADR-0196), and the cases at the end of this file are
// where that is pinned down: a held button, a number box being dragged and a
// selected choice each draw a fill of `inverse` with the label on it in
// `inverse_ink`, while merely hovered stays the control_hovered step it was.
// Those cases put a label on a control and so measure a string, which needs a
// real font and so a device — a headless one, with no window and no compositor,
// and a skip naming what did not run where there is no driver at all
// (ADR-0106).
//
// ---- WHY THE CLICK CASES ARE THE ONES THAT MATTER ----
//
// A button that fires on press is one line shorter and passes a test that only
// ever presses and releases in the same place. The four orders below are the
// ones a real hand produces — press and release, press and drag off, drag off
// and back, and press something that then stops existing — and each of the four
// has a different right answer. Getting the third wrong is the common bug and it
// looks like an unreliable button rather than like a rule.
//
// A CLIP IS WHAT CAN BE SEEN AND SO WHAT CAN BE HIT, since spec 001. A button
// half clipped emits a record clipped to its visible half and answers the pointer
// only there; one wholly clipped emits nothing; and a number box scrolled out of
// sight mid-drag goes on reporting its drag. A label under a clip is the same
// rule and needs a font, so it is in `ui/tests/widgets.c`.
//
// THE GEOMETRY IS WORKED OUT BY HAND AND WRITTEN AS NUMBERS. A button's
// rectangle comes out of layout, so a test that asked layout where the button
// was and then clicked there would pass with the arithmetic inverted. The
// numbers below say where the buttons are meant to be.
#include <ui/layout.h>
#include <ui/widgets.h>

#include <base/arena.h>
#include <math/float2.h>
#include <math/float4.h>
#include <render/device.h>
#include <text/font.h>

#include <testing/test.h>

#include <stdio.h>

#define SCRATCH 65536

// Two buttons in a column, each holding one box of a known size, so that every
// rectangle below is arithmetic a reader can do:
//
//   panel   pad 4, gap 3, at the origin
//     button "a"   pad 2.5 round a 20 x 10 box  ->  25 x 15 at (4, 4)
//     button "b"   the same                     ->  25 x 15 at (4, 22)
//
// The panel is then 33 x 41. The two buttons do not touch, which is what makes
// "the pointer is between them" a case the tests can express.
#define BOX_WIDE 20.0f
#define BOX_HIGH 10.0f
#define BUTTON_WIDE 25.0f
#define BUTTON_HIGH 15.0f
#define A_Y 4.0f
#define B_Y 22.0f

// A point inside button "a", one inside "b", and one in the gap between them.
#define IN_A ((voe_math_float2){ 16.0f, 10.0f })
#define IN_B ((voe_math_float2){ 16.0f, 28.0f })
#define BETWEEN ((voe_math_float2){ 16.0f, 19.0f })

struct frame {
	voe_ui_node a;
	voe_ui_node b;
	bool ok;
};

// The theme every context in this file draws with, set once in main() — a
// panel, a button and a number box each need one in force to be built at all,
// and the fill and the border a state draws with come out of its roles.
static voe_ui_theme TEST_THEME;

// The same padding on all four sides, which is what the panel above wants and
// what one number used to say.
static voe_ui_pad pad_all(float mm)
{
	return (voe_ui_pad){ mm, mm, mm, mm };
}

// One frame of the interface above, with the pointer wherever the caller says.
// `buttons` is how many of the two are built, so that "the widget that was
// pressed is not called this frame" is one argument rather than a second tree.
static struct frame build(voe_ui_context *ui, voe_base_arena *arena,
			  voe_math_float2 at, bool over, bool down,
			  uint32_t buttons)
{
	struct frame f = { VOE_UI_NODE_NONE, VOE_UI_NODE_NONE, false };

	voe_ui_frame_begin(ui, arena);
	voe_ui_pointer_set(ui, (voe_ui_pointer){ .at = at,
						 .over = over,
						 .down = down });

	voe_ui_panel_begin(ui, "panel", 0, VOE_UI_SURFACE_SURFACE,
			   (voe_ui_container){ .gap = 3.0f,
					       .pad = pad_all(4.0f) });
	if (buttons > 0) {
		f.a = voe_ui_button_begin(ui, "a", 0);
		voe_ui_box(ui, (voe_math_float2){ BOX_WIDE, BOX_HIGH },
			   (voe_ui_sizing){ 0 });
		voe_ui_end(ui);
	}
	if (buttons > 1) {
		f.b = voe_ui_button_begin(ui, "b", 0);
		voe_ui_box(ui, (voe_math_float2){ BOX_WIDE, BOX_HIGH },
			   (voe_ui_sizing){ 0 });
		voe_ui_end(ui);
	}
	voe_ui_end(ui);

	f.ok = voe_ui_frame_end(ui);
	return f;
}

// The rectangles are the premise of every click case below, so they are checked
// once, here, rather than assumed six times.
static void the_tree_is_where_the_tests_think_it_is(voe_ui_context *ui,
						    voe_base_arena *arena)
{
	struct frame f = build(ui, arena, IN_A, false, false, 2);
	voe_ui_rect a;
	voe_ui_rect b;

	VOE_TEST_CHECK(f.ok);
	a = voe_ui_node_rect(ui, f.a);
	b = voe_ui_node_rect(ui, f.b);

	VOE_TEST_CHECK_FLOAT(a.min.x, 4.0f, 0.001f);
	VOE_TEST_CHECK_FLOAT(a.min.y, A_Y, 0.001f);
	VOE_TEST_CHECK_FLOAT(a.size.x, BUTTON_WIDE, 0.001f);
	VOE_TEST_CHECK_FLOAT(a.size.y, BUTTON_HIGH, 0.001f);
	VOE_TEST_CHECK_FLOAT(b.min.y, B_Y, 0.001f);
	VOE_TEST_CHECK_FLOAT(b.size.y, BUTTON_HIGH, 0.001f);
}

static void a_pointer_over_a_button_hovers_it(voe_ui_context *ui,
					      voe_base_arena *arena)
{
	struct frame f = build(ui, arena, IN_A, true, false, 2);
	voe_ui_action a = voe_ui_button_action(ui, f.a);
	voe_ui_action b = voe_ui_button_action(ui, f.b);

	VOE_TEST_CHECK(a.hovered);
	VOE_TEST_CHECK(!a.held);
	VOE_TEST_CHECK(!a.fired);
	VOE_TEST_CHECK(!b.hovered);
}

// The pointer is gone rather than merely elsewhere, which is a different thing
// and the reason `over` exists: a hover has to end when the mouse leaves the
// window, not only when it moves onto something else.
static void no_pointer_hovers_nothing(voe_ui_context *ui,
				      voe_base_arena *arena)
{
	struct frame f = build(ui, arena, IN_A, false, false, 2);

	VOE_TEST_CHECK(!voe_ui_button_action(ui, f.a).hovered);
}

static void press_and_release_inside_fires_once(voe_ui_context *ui,
						voe_base_arena *arena)
{
	struct frame f = build(ui, arena, IN_A, true, false, 2);

	VOE_TEST_CHECK(!voe_ui_button_action(ui, f.a).fired);

	f = build(ui, arena, IN_A, true, true, 2);
	VOE_TEST_CHECK(voe_ui_button_action(ui, f.a).held);
	VOE_TEST_CHECK(!voe_ui_button_action(ui, f.a).fired);

	f = build(ui, arena, IN_A, true, false, 2);
	VOE_TEST_CHECK(voe_ui_button_action(ui, f.a).fired);
	VOE_TEST_CHECK(!voe_ui_button_action(ui, f.a).held);

	// EXACTLY ONCE. A fired flag that stayed set is a button that runs its
	// action every frame the mouse rests on it afterwards, which is the
	// worst version of this bug because the first click looks right.
	f = build(ui, arena, IN_A, true, false, 2);
	VOE_TEST_CHECK(!voe_ui_button_action(ui, f.a).fired);
}

static void dragging_off_and_releasing_cancels(voe_ui_context *ui,
					       voe_base_arena *arena)
{
	// A frame with the button up first, so that the press below is an
	// edge and not whatever the frame before this test left behind.
	struct frame f = build(ui, arena, IN_A, true, false, 2);

	VOE_TEST_CHECK(!voe_ui_button_action(ui, f.a).held);

	f = build(ui, arena, IN_A, true, true, 2);
	VOE_TEST_CHECK(voe_ui_button_action(ui, f.a).held);

	// Off it, still down: no longer hovered, still held, still cancellable.
	f = build(ui, arena, BETWEEN, true, true, 2);
	VOE_TEST_CHECK(!voe_ui_button_action(ui, f.a).hovered);
	VOE_TEST_CHECK(voe_ui_button_action(ui, f.a).held);

	f = build(ui, arena, BETWEEN, true, false, 2);
	VOE_TEST_CHECK(!voe_ui_button_action(ui, f.a).fired);
	VOE_TEST_CHECK(!voe_ui_button_action(ui, f.a).held);
}

// The case people get wrong. Leaving cancels; coming back undoes the cancel.
static void dragging_off_and_back_still_fires(voe_ui_context *ui,
					      voe_base_arena *arena)
{
	struct frame f = build(ui, arena, IN_A, true, false, 2);

	VOE_TEST_CHECK(!voe_ui_button_action(ui, f.a).held);

	f = build(ui, arena, IN_A, true, true, 2);
	VOE_TEST_CHECK(voe_ui_button_action(ui, f.a).held);

	// Off it: cancelled, as far as a release is concerned.
	f = build(ui, arena, BETWEEN, true, true, 2);
	VOE_TEST_CHECK(!voe_ui_button_action(ui, f.a).hovered);

	// And back on it: uncancelled.
	f = build(ui, arena, IN_A, true, true, 2);
	VOE_TEST_CHECK(voe_ui_button_action(ui, f.a).hovered);
	VOE_TEST_CHECK(voe_ui_button_action(ui, f.a).held);

	f = build(ui, arena, IN_A, true, false, 2);
	VOE_TEST_CHECK(voe_ui_button_action(ui, f.a).fired);
}

// Press one and release over the other. Neither fires: the one released over
// was never armed, and the one armed was not released over.
static void releasing_over_another_button_fires_nothing(voe_ui_context *ui,
						       voe_base_arena *arena)
{
	struct frame f = build(ui, arena, IN_A, true, false, 2);

	VOE_TEST_CHECK(!voe_ui_button_action(ui, f.a).held);

	f = build(ui, arena, IN_A, true, true, 2);
	VOE_TEST_CHECK(voe_ui_button_action(ui, f.a).held);
	VOE_TEST_CHECK(!voe_ui_button_action(ui, f.b).held);

	f = build(ui, arena, IN_B, true, false, 2);
	VOE_TEST_CHECK(!voe_ui_button_action(ui, f.a).fired);
	VOE_TEST_CHECK(!voe_ui_button_action(ui, f.b).fired);
}

// Arriving over a button with the mouse already down arms nothing, so letting go
// there does not fire it. Windows 10 again: a drag that began somewhere else is
// not a press on what it happens to end over.
static void arriving_with_the_button_already_down_arms_nothing(
	voe_ui_context *ui, voe_base_arena *arena)
{
	struct frame f = build(ui, arena, BETWEEN, true, false, 2);

	VOE_TEST_CHECK(!voe_ui_button_action(ui, f.a).hovered);

	// Down over nothing: nothing is armed.
	f = build(ui, arena, BETWEEN, true, true, 2);
	VOE_TEST_CHECK(!voe_ui_button_action(ui, f.a).held);

	f = build(ui, arena, IN_A, true, true, 2);
	VOE_TEST_CHECK(voe_ui_button_action(ui, f.a).hovered);
	VOE_TEST_CHECK(!voe_ui_button_action(ui, f.a).held);

	f = build(ui, arena, IN_A, true, false, 2);
	VOE_TEST_CHECK(!voe_ui_button_action(ui, f.a).fired);
}

// A button pressed and then not built again — a panel closed with the mouse
// down. The press must not survive to be completed later on whatever comes back
// with that key.
static void a_button_that_stops_being_called_is_let_go(voe_ui_context *ui,
						       voe_base_arena *arena)
{
	struct frame f = build(ui, arena, IN_A, true, false, 2);

	VOE_TEST_CHECK(!voe_ui_button_action(ui, f.a).held);

	f = build(ui, arena, IN_A, true, true, 2);
	VOE_TEST_CHECK(voe_ui_button_action(ui, f.a).held);

	// Gone, with the mouse still down. There is no handle to ask about
	// any more, which is the situation: the frame did not build it.
	f = build(ui, arena, IN_A, true, true, 0);
	VOE_TEST_CHECK_INT((int)f.a, (int)VOE_UI_NODE_NONE);

	// Back, still down. It must not be held, and releasing on it must not
	// fire: the gesture ended when the button did.
	f = build(ui, arena, IN_A, true, true, 2);
	VOE_TEST_CHECK(!voe_ui_button_action(ui, f.a).held);

	f = build(ui, arena, IN_A, true, false, 2);
	VOE_TEST_CHECK(!voe_ui_button_action(ui, f.a).fired);
}

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

// A row 12.5 wide clipping X, holding two buttons 25 by 15: "half" at the origin,
// of which the left 12.5 is seen, and "gone" at 25, of which nothing is. The row
// is no panel and emits nothing, so every record here is a button's.
#define HALF_WIDE 12.5f

static struct frame build_clipped(voe_ui_context *ui, voe_base_arena *arena,
				  voe_math_float2 at, bool down)
{
	struct frame f = { VOE_UI_NODE_NONE, VOE_UI_NODE_NONE, false };

	voe_ui_frame_begin(ui, arena);
	voe_ui_pointer_set(ui, (voe_ui_pointer){ .at = at,
						 .over = true,
						 .down = down });

	voe_ui_row_begin(ui, (voe_ui_container){
				     .size = { { VOE_UI_SIZE_FIXED, HALF_WIDE },
					       { VOE_UI_SIZE_FIXED, 20.0f } },
				     .overflow = { VOE_UI_OVERFLOW_CLIP,
						   VOE_UI_OVERFLOW_VISIBLE } });
	f.a = voe_ui_button_begin(ui, "half", 0);
	voe_ui_box(ui, (voe_math_float2){ BOX_WIDE, BOX_HIGH },
		   (voe_ui_sizing){ 0 });
	voe_ui_end(ui);
	f.b = voe_ui_button_begin(ui, "gone", 0);
	voe_ui_box(ui, (voe_math_float2){ BOX_WIDE, BOX_HIGH },
		   (voe_ui_sizing){ 0 });
	voe_ui_end(ui);
	voe_ui_end(ui);

	f.ok = voe_ui_frame_end(ui);
	return f;
}

// ONE RECORD, FOR THE HALF THAT IS SEEN: its bounds the whole button, its clip
// the left 12.5 of it. The wholly clipped button emits nothing and takes no
// capacity, which the count says. The seen half is TWO records now — its
// border, then its fill inset from every edge — and both are narrowed to the
// same clip (ADR-0171).
#define BORDER_WIDE 0.3f

static void a_half_clipped_button_emits_its_visible_half(voe_ui_context *ui,
							 voe_base_arena *arena)
{
	struct frame f = build_clipped(ui, arena, (voe_math_float2){ 0 }, false);
	voe_render_element border;
	voe_render_element fill;

	VOE_TEST_CHECK(f.ok);
	VOE_TEST_CHECK_INT((int)voe_ui_element_count(ui), 2);
	if (voe_ui_element_count(ui) != 2)
		return;

	border = voe_ui_element(ui, 0);
	fill = voe_ui_element(ui, 1);

	VOE_TEST_CHECK_FLOAT(border.bounds.x, 0.0f, 0.001f);
	VOE_TEST_CHECK_FLOAT(border.bounds.z, BUTTON_WIDE, 0.001f);
	VOE_TEST_CHECK_FLOAT(border.clip.x, 0.0f, 0.001f);
	VOE_TEST_CHECK_FLOAT(border.clip.y, 0.0f, 0.001f);
	VOE_TEST_CHECK_FLOAT(border.clip.z, HALF_WIDE, 0.001f);
	VOE_TEST_CHECK_FLOAT(border.clip.w, BUTTON_HIGH, 0.001f);

	VOE_TEST_CHECK_FLOAT(fill.bounds.x, BORDER_WIDE, 0.001f);
	VOE_TEST_CHECK_FLOAT(fill.bounds.z, BUTTON_WIDE - 2.0f * BORDER_WIDE,
			     0.001f);
	VOE_TEST_CHECK_FLOAT(fill.clip.x, BORDER_WIDE, 0.001f);
	VOE_TEST_CHECK_FLOAT(fill.clip.z, HALF_WIDE - BORDER_WIDE, 0.001f);
}

// THE POINTER HITS WHAT IS SEEN. Over the visible half the button is hovered; over
// the half cut away, inside its rectangle, it is not — and neither is the button
// wholly out of sight, pointed at where it would be.
static void a_pointer_over_the_clipped_half_hovers_nothing(
	voe_ui_context *ui, voe_base_arena *arena)
{
	struct frame f = build_clipped(ui, arena,
				       (voe_math_float2){ 6.0f, 7.0f }, false);

	VOE_TEST_CHECK(voe_ui_button_action(ui, f.a).hovered);

	f = build_clipped(ui, arena, (voe_math_float2){ 18.0f, 7.0f }, false);
	VOE_TEST_CHECK(!voe_ui_button_action(ui, f.a).hovered);

	f = build_clipped(ui, arena, (voe_math_float2){ 30.0f, 7.0f }, false);
	VOE_TEST_CHECK(!voe_ui_button_action(ui, f.b).hovered);

	// And a press on the half cut away arms nothing.
	f = build_clipped(ui, arena, (voe_math_float2){ 18.0f, 7.0f }, true);
	VOE_TEST_CHECK(!voe_ui_button_action(ui, f.a).held);

	// Let go, so the next case starts with the button up.
	(void)build_clipped(ui, arena, (voe_math_float2){ 18.0f, 7.0f }, false);
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

// ---- STATE DRAWN INVERTED, WHICH NEEDS A FONT ----

// The three controls the cases below are built from, each round the same
// one-letter label.
enum control { CONTROL_BUTTON, CONTROL_NUMBER, CONTROL_CHOICE };

// One of the three at the origin of a bare column, which emits nothing itself,
// so every record of the frame is the control's: its border where it has one,
// its fill, and then the label's letters. All three are 2.5 of padding round a
// label taller and wider than nothing, so the pointer a millimetre in from the
// origin is inside whichever was built.
static voe_ui_node build_labelled(voe_ui_context *ui, voe_base_arena *arena,
				  enum control kind, bool selected, float x,
				  bool over, bool down)
{
	voe_ui_node n;

	voe_ui_frame_begin(ui, arena);
	voe_ui_pointer_set(ui, (voe_ui_pointer){ .at = { x, 1.0f },
						 .over = over,
						 .down = down });

	voe_ui_column_begin(ui, (voe_ui_container){ 0 });
	if (kind == CONTROL_NUMBER)
		n = voe_ui_number_begin(ui, "n", 0, START, PER_MM);
	else if (kind == CONTROL_CHOICE)
		n = voe_ui_choice_begin(ui, "c", 0, selected);
	else
		n = voe_ui_button_begin(ui, "a", 0);
	voe_ui_label(ui, "A");
	voe_ui_end(ui);
	voe_ui_end(ui);

	VOE_TEST_CHECK(voe_ui_frame_end(ui));
	return n;
}

// Alpha is not compared: every record here is opaque.
static void check_colour(voe_math_float4 got, voe_math_float4 want)
{
	VOE_TEST_CHECK_FLOAT(got.x, want.x, 0.001f);
	VOE_TEST_CHECK_FLOAT(got.y, want.y, 0.001f);
	VOE_TEST_CHECK_FLOAT(got.z, want.z, 0.001f);
}

// The control's fill and the ink of every letter on it. A button and a choice
// are a border and then the fill inside it; a number box is one borderless
// fill; the label's letters follow either.
static void check_control(voe_ui_context *ui, enum control kind,
			  voe_math_float4 fill, voe_math_float4 ink)
{
	uint32_t at = kind == CONTROL_NUMBER ? 0u : 1u;
	uint32_t glyphs = 0;

	VOE_TEST_CHECK(voe_ui_element_count(ui) > at);
	if (voe_ui_element_count(ui) <= at)
		return;

	check_colour(voe_ui_element(ui, at).colour, fill);

	for (uint32_t i = at + 1; i < voe_ui_element_count(ui); i++) {
		voe_render_element e = voe_ui_element(ui, i);

		if (e.kind != VOE_RENDER_ELEMENT_GLYPH)
			continue;
		glyphs++;
		check_colour(e.colour, ink);
	}

	// The letters are real: a label that measured to nothing would leave
	// every ink check above unrun.
	VOE_TEST_CHECK(glyphs > 0);
}

// A HELD BUTTON IS DRAWN INVERTED, LABEL AND ALL (ADR-0196): the fill is
// `inverse` and the letters on it `inverse_ink`, which the caller's plain
// label never asked for and the button decided.
static void a_held_button_is_inverted(voe_ui_context *ui,
				      voe_base_arena *arena)
{
	voe_ui_node b = build_labelled(ui, arena, CONTROL_BUTTON, false, 1.0f,
				       true, false);

	// Up first, so the press below is an edge.
	VOE_TEST_CHECK(!voe_ui_button_action(ui, b).held);

	b = build_labelled(ui, arena, CONTROL_BUTTON, false, 1.0f, true, true);
	VOE_TEST_CHECK(voe_ui_button_action(ui, b).held);
	check_control(ui, CONTROL_BUTTON, TEST_THEME.inverse,
		      TEST_THEME.inverse_ink);

	// Let go, so the next case starts with the pointer up.
	(void)build_labelled(ui, arena, CONTROL_BUTTON, false, 1.0f, true,
			     false);
}

// A number box being dragged is the same state: pressed at 1 and dragged to
// 11, nine millimetres past the dead zone, so it is moving the value and drawn
// inverted while it does.
static void a_dragged_number_box_is_inverted(voe_ui_context *ui,
					     voe_base_arena *arena)
{
	voe_ui_node n = build_labelled(ui, arena, CONTROL_NUMBER, false, 1.0f,
				       true, false);

	VOE_TEST_CHECK(!voe_ui_number_action(ui, n).held);

	(void)build_labelled(ui, arena, CONTROL_NUMBER, false, 1.0f, true,
			     true);
	n = build_labelled(ui, arena, CONTROL_NUMBER, false, 11.0f, true, true);
	VOE_TEST_CHECK(voe_ui_number_action(ui, n).changed);
	check_control(ui, CONTROL_NUMBER, TEST_THEME.inverse,
		      TEST_THEME.inverse_ink);

	// Let go far outside the dead zone, which ends the drag without
	// opening the box for typing.
	(void)build_labelled(ui, arena, CONTROL_NUMBER, false, 11.0f, true,
			     false);
}

// A SELECTED CHOICE IS DRAWN AS A HELD BUTTON IS, with no pointer on it at
// all, and being neither selected nor held it is the ordinary control — and
// control_hovered under the pointer, which inversion does not swallow.
static void a_selected_choice_is_inverted(voe_ui_context *ui,
					  voe_base_arena *arena)
{
	(void)build_labelled(ui, arena, CONTROL_CHOICE, true, 1.0f, false,
			     false);
	check_control(ui, CONTROL_CHOICE, TEST_THEME.inverse,
		      TEST_THEME.inverse_ink);

	(void)build_labelled(ui, arena, CONTROL_CHOICE, false, 1.0f, false,
			     false);
	check_control(ui, CONTROL_CHOICE, TEST_THEME.control,
		      TEST_THEME.text_primary);

	(void)build_labelled(ui, arena, CONTROL_CHOICE, false, 1.0f, true,
			     false);
	check_control(ui, CONTROL_CHOICE, TEST_THEME.control_hovered,
		      TEST_THEME.text_primary);
}

// The device and the font the three cases above need, and the skip that stands
// in for them where there is no driver.
static void the_inverted_states(voe_base_arena *arena)
{
	voe_platform_size size = { 64, 64 };
	// Nothing here draws anything. The device exists so that a font can
	// upload its atlas, which is a texture and not a pool — so these are
	// the smallest numbers a device will open with and not an estimate of
	// anything.
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

	device = voe_render_device_new_headless(arena, size, capacities,
						&error);
	if (device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED) {
			// ADR-0106: a skip names what went unchecked, not only
			// why it did.
			printf("skip: no graphics driver — the held-button, "
			       "dragged-number-box and selected-choice "
			       "inversion checks did not run\n");
			return;
		}
		VOE_TEST_CHECK(device != NULL);
		return;
	}

	// Oxanium: nothing here cares which face, so the engine's default.
	font = voe_text_font_new(VOE_TEXT_TYPEFACE_OXANIUM, device, arena,
				 &error);
	if (font == NULL) {
		VOE_TEST_CHECK(font != NULL);
		voe_render_device_destroy(device);
		return;
	}

	ui = voe_ui_context_new(arena, (voe_ui_capacities){ .nodes = 16,
							    .elements = 64 });
	voe_ui_font_set(ui, font);
	voe_ui_theme_set(ui, &TEST_THEME);

	a_held_button_is_inverted(ui, arena);
	a_dragged_number_box_is_inverted(ui, arena);
	a_selected_choice_is_inverted(ui, arena);

	voe_text_font_destroy(font);
	voe_render_device_destroy(device);
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

	the_tree_is_where_the_tests_think_it_is(ui, arena);
	a_pointer_over_a_button_hovers_it(ui, arena);
	no_pointer_hovers_nothing(ui, arena);
	press_and_release_inside_fires_once(ui, arena);
	dragging_off_and_releasing_cancels(ui, arena);
	dragging_off_and_back_still_fires(ui, arena);
	releasing_over_another_button_fires_nothing(ui, arena);
	arriving_with_the_button_already_down_arms_nothing(ui, arena);
	a_button_that_stops_being_called_is_let_go(ui, arena);
	the_number_box_is_where_the_tests_think_it_is(ui, arena);
	dragging_sideways_moves_the_value(ui, arena);
	a_fine_drag_moves_a_tenth_as_far(ui, arena);
	a_drag_past_the_edge_keeps_working(ui, arena);
	arriving_with_the_button_already_down_arms_no_number(ui, arena);
	a_number_box_that_stops_being_called_is_let_go(ui, arena);
	a_half_clipped_button_emits_its_visible_half(ui, arena);
	a_pointer_over_the_clipped_half_hovers_nothing(ui, arena);
	a_number_box_scrolled_away_mid_drag_keeps_dragging(ui, arena);

	the_inverted_states(arena);

	// A context of its own for the dead zone, thrown away with the box it
	// leaves open still open — see the case itself.
	ui = voe_ui_context_new(
		arena, (voe_ui_capacities){ .nodes = 16, .elements = 64 });
	voe_ui_theme_set(ui, &TEST_THEME);
	movement_inside_the_dead_zone_changes_nothing(ui, arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
