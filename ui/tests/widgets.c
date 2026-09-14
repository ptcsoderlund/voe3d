// The widgets, and the two things about them that are worth pinning down: what
// a press and a release do in every awkward order a hand can produce, and that
// a known tree comes out as a known list of records — two images in a row among
// them, each one IMAGE record carrying the texture index and sheet it was given.
//
// EVERY CASE HERE EXCEPT THE LAST NEEDS NO GRAPHICS CARD AND NO WINDOW SYSTEM,
// which is the property the whole design is arranged around: the pointer is a
// value handed in (ADR-0093), so a drag is three calls in a row and not a mouse.
// A button is composed rather than given a string, so nothing in the click cases
// touches a font. Nothing here opens a window, asks a compositor for anything or
// names `platform`.
//
// THE LAST CASE IS THE EXCEPTION AND IT IS DELIBERATE (ADR-0106). The text scale
// multiplies a MEASUREMENT, and a label emits one record per letter, so proving
// either needs a real font, and a font uploads
// an atlas and so needs a device. It takes a headless one — no window, no
// surface, no compositor — and where there is no driver at all it skips, saying
// which check did not run rather than only why.
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
// THE COLLISION CASE IS THE MOST VALUABLE ONE IN THIS FILE. Two widgets given
// the same key share state silently: the second lights up when the first is
// hovered and nothing says why. Here it is a refused frame and a line on stderr,
// and this case is what keeps it one.
//
// A CLIP IS WHAT CAN BE SEEN AND SO WHAT CAN BE HIT, since spec 001. A button
// half clipped emits a record clipped to its visible half and answers the pointer
// only there; one wholly clipped emits nothing; and a number box scrolled out of
// sight mid-drag goes on reporting its drag. A label wholly clipped is the one of
// these that needs a font, so it is among the device cases.
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

static const voe_math_float4 PANEL = { 0.05f, 0.05f, 0.06f, 0.8f };

// The same padding on all four sides, which is what every case in this file
// wants and what one number used to say.
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

	voe_ui_panel_begin(ui, "panel", 0, PANEL,
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

// Two buttons under the same parent with the same name and the same index. The
// frame is refused rather than the two of them quietly behaving as one.
static void a_duplicate_key_refuses_the_frame(voe_ui_context *ui,
					      voe_base_arena *arena)
{
	bool ok;

	fprintf(stderr, "-- the next voe_ui line is this test's own --\n");

	voe_ui_frame_begin(ui, arena);
	voe_ui_panel_begin(ui, "panel", 0, PANEL, (voe_ui_container){ 0 });
	voe_ui_button_begin(ui, "same", 0);
	voe_ui_end(ui);
	voe_ui_button_begin(ui, "same", 0);
	voe_ui_end(ui);
	voe_ui_end(ui);
	ok = voe_ui_frame_end(ui);

	VOE_TEST_CHECK(!ok);

	// And the next frame is fine, which is what "a refusal and not a fatal
	// error" has to mean.
	voe_ui_frame_begin(ui, arena);
	voe_ui_panel_begin(ui, "panel", 0, PANEL, (voe_ui_container){ 0 });
	voe_ui_button_begin(ui, "same", 0);
	voe_ui_end(ui);
	voe_ui_button_begin(ui, "same", 1);
	voe_ui_end(ui);
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));
}

// The same name under two different panels is two different widgets, which is
// the whole reason a key is a path and not a name. Two panels stacked in a
// column, a button called "go" in each: the frame is not refused, and hovering
// the second does not light up the first.
static void the_same_name_under_two_panels_is_two_widgets(voe_ui_context *ui,
							  voe_base_arena *arena)
{
	voe_ui_node first;
	voe_ui_node second;

	voe_ui_frame_begin(ui, arena);
	// Over the second button: the outer column has no padding and no gap,
	// so the first panel is 25 x 15 at the origin and the second is the
	// same directly under it.
	voe_ui_pointer_set(ui, (voe_ui_pointer){ .at = { 10.0f, 20.0f },
						 .over = true });
	voe_ui_column_begin(ui, (voe_ui_container){ 0 });

	voe_ui_panel_begin(ui, "left", 0, PANEL, (voe_ui_container){ 0 });
	first = voe_ui_button_begin(ui, "go", 0);
	voe_ui_box(ui, (voe_math_float2){ BOX_WIDE, BOX_HIGH },
		   (voe_ui_sizing){ 0 });
	voe_ui_end(ui);
	voe_ui_end(ui);

	voe_ui_panel_begin(ui, "right", 0, PANEL, (voe_ui_container){ 0 });
	second = voe_ui_button_begin(ui, "go", 0);
	voe_ui_box(ui, (voe_math_float2){ BOX_WIDE, BOX_HIGH },
		   (voe_ui_sizing){ 0 });
	voe_ui_end(ui);
	voe_ui_end(ui);

	voe_ui_end(ui);

	// NOT REFUSED, which is the first half: two "go"s under two different
	// parents are two keys and not a duplicate.
	VOE_TEST_CHECK(voe_ui_frame_end(ui));

	// And the second half: a shared key would report BOTH as hovered,
	// which is exactly the silent sharing this scheme exists to prevent.
	VOE_TEST_CHECK(voe_ui_button_action(ui, second).hovered);
	VOE_TEST_CHECK(!voe_ui_button_action(ui, first).hovered);
}

// ---------------------------------------------------------- the number box

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

	voe_ui_panel_begin(ui, "panel", 0, PANEL, (voe_ui_container){ 0 });
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

// Half a millimetre is not a drag, and this is what keeps a click a click. Two
// frames inside the zone, so that a slow hand crossing it in small steps is
// covered as well as one that never leaves it.
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

// A CLICK DOES NOTHING, AND IT IS RESERVED RATHER THAN MERELY UNUSED. Press and
// release without moving: no frame reports a change and no frame reports a
// different value. There is nothing to bind to here because typing is going to
// want it.
static void a_press_and_release_without_movement_does_nothing(
	voe_ui_context *ui, voe_base_arena *arena)
{
	struct number_frame f = build_number(ui, arena, 10.0f, true, false,
					     false, 1);
	voe_ui_number_result r = voe_ui_number_action(ui, f.n);

	VOE_TEST_CHECK(!r.changed);

	f = build_number(ui, arena, 10.0f, true, true, false, 1);
	r = voe_ui_number_action(ui, f.n);
	VOE_TEST_CHECK(r.held);
	VOE_TEST_CHECK(!r.changed);
	VOE_TEST_CHECK_FLOAT((float)r.value, (float)START, 0.001f);

	f = build_number(ui, arena, 10.0f, true, false, false, 1);
	r = voe_ui_number_action(ui, f.n);
	VOE_TEST_CHECK(!r.held);
	VOE_TEST_CHECK(!r.changed);
	VOE_TEST_CHECK_FLOAT((float)r.value, (float)START, 0.001f);

	// And the frame after the release, which is where a fired flag that
	// stayed set would show up if this widget had one.
	f = build_number(ui, arena, 10.0f, true, false, false, 1);
	VOE_TEST_CHECK(!voe_ui_number_action(ui, f.n).changed);
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

// A number box and a button are not two kinds of thing as far as identity is
// concerned: one key each, and two calls that make the same one is the same
// refusal a pair of buttons gets.
static void a_number_and_a_button_sharing_a_key_refuse_the_frame(
	voe_ui_context *ui, voe_base_arena *arena)
{
	fprintf(stderr, "-- the next voe_ui line is this test's own --\n");

	voe_ui_frame_begin(ui, arena);
	voe_ui_panel_begin(ui, "panel", 0, PANEL, (voe_ui_container){ 0 });
	voe_ui_button_begin(ui, "same", 0);
	voe_ui_end(ui);
	voe_ui_number_begin(ui, "same", 0, START, PER_MM);
	voe_ui_end(ui);
	voe_ui_end(ui);

	VOE_TEST_CHECK(!voe_ui_frame_end(ui));

	// And the next frame is fine, which is what a refusal has to mean.
	voe_ui_frame_begin(ui, arena);
	voe_ui_panel_begin(ui, "panel", 0, PANEL, (voe_ui_container){ 0 });
	voe_ui_button_begin(ui, "same", 0);
	voe_ui_end(ui);
	voe_ui_number_begin(ui, "same", 1, START, PER_MM);
	voe_ui_end(ui);
	voe_ui_end(ui);

	VOE_TEST_CHECK(voe_ui_frame_end(ui));
}

// A known tree, a known list. Three records: the panel's background, then each
// button's, in the order they were called. The boxes inside the buttons draw
// nothing, and nothing is emitted for them.
static void a_known_tree_emits_a_known_list(voe_ui_context *ui,
					    voe_base_arena *arena)
{
	struct frame f = build(ui, arena, IN_A, true, false, 2);
	voe_render_element panel;
	voe_render_element a;
	voe_render_element b;

	VOE_TEST_CHECK(f.ok);
	VOE_TEST_CHECK_INT((int)voe_ui_element_count(ui), 3);
	if (voe_ui_element_count(ui) != 3)
		return;

	panel = voe_ui_element(ui, 0);
	a = voe_ui_element(ui, 1);
	b = voe_ui_element(ui, 2);

	// THE PANEL'S BACKGROUND COMES FIRST, which is the claim: paint order
	// is submission order and a parent is submitted before its children.
	// Reversed, the panel would cover both buttons and the interface would
	// look like a plain rectangle.
	VOE_TEST_CHECK_FLOAT(panel.bounds.z, 33.0f, 0.001f);
	VOE_TEST_CHECK_FLOAT(panel.bounds.w, 41.0f, 0.001f);
	VOE_TEST_CHECK_FLOAT(panel.colour.w, PANEL.w, 0.001f);

	// A COPY AND NOT A CONVERSION: min is the xy and size is the zw, and
	// there is no arithmetic between voe_ui_node_rect and this.
	VOE_TEST_CHECK_FLOAT(a.bounds.x, 4.0f, 0.001f);
	VOE_TEST_CHECK_FLOAT(a.bounds.y, A_Y, 0.001f);
	VOE_TEST_CHECK_FLOAT(a.bounds.z, BUTTON_WIDE, 0.001f);
	VOE_TEST_CHECK_FLOAT(a.bounds.w, BUTTON_HIGH, 0.001f);
	VOE_TEST_CHECK_FLOAT(b.bounds.y, B_Y, 0.001f);

	// NOT A ZEROED CLIP, WHICH WOULD CLIP EVERY ONE OF THEM AWAY. Its own
	// bounds, so nothing is clipped and card 035 has something to narrow.
	VOE_TEST_CHECK_FLOAT(a.clip.x, a.bounds.x, 0.001f);
	VOE_TEST_CHECK_FLOAT(a.clip.z, a.bounds.z, 0.001f);

	VOE_TEST_CHECK_INT((int)a.kind, (int)VOE_RENDER_ELEMENT_SOLID);

	// The hovered button is not the same colour as the other one, which is
	// the whole of what a visual state is.
	VOE_TEST_CHECK(a.colour.x != b.colour.x || a.colour.y != b.colour.y ||
		       a.colour.z != b.colour.z);
}

// An alpha of nought is not a transparent rectangle. It is no record at all.
static void a_transparent_panel_emits_nothing(voe_ui_context *ui,
					      voe_base_arena *arena)
{
	voe_ui_frame_begin(ui, arena);
	voe_ui_panel_begin(ui, "invisible", 0,
			   (voe_math_float4){ 0.5f, 0.5f, 0.5f, 0.0f },
			   (voe_ui_container){ .pad = pad_all(6.0f) });
	voe_ui_box(ui, (voe_math_float2){ 10.0f, 10.0f }, (voe_ui_sizing){ 0 });
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));

	VOE_TEST_CHECK_INT((int)voe_ui_element_count(ui), 0);
}

// A frame that emits more than it was given is refused, the same way one that
// wants more nodes is.
static void too_many_elements_refuses_the_frame(voe_base_arena *arena)
{
	voe_ui_context *ui = voe_ui_context_new(
		arena, (voe_ui_capacities){ .nodes = 8, .elements = 1 });

	fprintf(stderr, "-- the next voe_ui line is this test's own --\n");

	voe_ui_frame_begin(ui, arena);
	voe_ui_panel_begin(ui, "panel", 0, PANEL, (voe_ui_container){ 0 });
	voe_ui_button_begin(ui, "a", 0);
	voe_ui_end(ui);
	voe_ui_end(ui);

	VOE_TEST_CHECK(!voe_ui_frame_end(ui));
}

// ------------------------------------------------------------------ clipping

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
// capacity, which the count says.
static void a_half_clipped_button_emits_its_visible_half(voe_ui_context *ui,
							 voe_base_arena *arena)
{
	struct frame f = build_clipped(ui, arena, (voe_math_float2){ 0 }, false);
	voe_render_element e;

	VOE_TEST_CHECK(f.ok);
	VOE_TEST_CHECK_INT((int)voe_ui_element_count(ui), 1);
	if (voe_ui_element_count(ui) != 1)
		return;

	e = voe_ui_element(ui, 0);
	VOE_TEST_CHECK_FLOAT(e.bounds.x, 0.0f, 0.001f);
	VOE_TEST_CHECK_FLOAT(e.bounds.z, BUTTON_WIDE, 0.001f);
	VOE_TEST_CHECK_FLOAT(e.clip.x, 0.0f, 0.001f);
	VOE_TEST_CHECK_FLOAT(e.clip.y, 0.0f, 0.001f);
	VOE_TEST_CHECK_FLOAT(e.clip.z, HALF_WIDE, 0.001f);
	VOE_TEST_CHECK_FLOAT(e.clip.w, BUTTON_HIGH, 0.001f);
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

// --------------------------------------------------------------- the image

// Two pictures in a row of a known width, so that both rectangles are arithmetic
// a reader can do and neither is what the test asked layout for:
//
//   row   100 x 30 fixed, pad 5, gap 4, at the origin, no background
//     image 1   20 x 12 fixed                    ->  20 x 12 at (5, 5)
//     image 2   grow along, 12 across            ->  100 - 5 - 20 - 4 - 5 = 66
//                                                    wide, at (29, 5)
//
// The texture ids are made up. An image record carries an index and nothing here
// reads a texture, which is why no device is needed.
#define IMAGE_ROW_WIDE 100.0f
#define IMAGE_HIGH 12.0f
#define IMAGE_ONE_WIDE 20.0f
#define IMAGE_TWO_X 29.0f
#define IMAGE_TWO_WIDE 66.0f

static const voe_render_texture IMAGE_ONE_TEXTURE = { .index = 7,
						      .generation = 3 };
static const voe_render_texture IMAGE_TWO_TEXTURE = { .index = 12,
						      .generation = 1 };
static const voe_math_float4 IMAGE_ONE_SHEET = { 0.0f, 0.0f, 1.0f, 1.0f };
static const voe_math_float4 IMAGE_TWO_SHEET = { 0.25f, 0.5f, 0.5f, 0.25f };

static void check_image_record(voe_render_element e, float x, float wide,
			       voe_render_texture texture,
			       voe_math_float4 sheet)
{
	VOE_TEST_CHECK_INT((int)e.kind, (int)VOE_RENDER_ELEMENT_IMAGE);

	// The node's laid-out rectangle, copied, and clipped to itself.
	VOE_TEST_CHECK_FLOAT(e.bounds.x, x, 0.001f);
	VOE_TEST_CHECK_FLOAT(e.bounds.y, 5.0f, 0.001f);
	VOE_TEST_CHECK_FLOAT(e.bounds.z, wide, 0.001f);
	VOE_TEST_CHECK_FLOAT(e.bounds.w, IMAGE_HIGH, 0.001f);
	VOE_TEST_CHECK_FLOAT(e.clip.x, e.bounds.x, 0.001f);
	VOE_TEST_CHECK_FLOAT(e.clip.y, e.bounds.y, 0.001f);
	VOE_TEST_CHECK_FLOAT(e.clip.z, e.bounds.z, 0.001f);
	VOE_TEST_CHECK_FLOAT(e.clip.w, e.bounds.w, 0.001f);

	// THE INDEX HALF AND NOT THE GENERATION. The two ids above differ in
	// both, so a record carrying the wrong half fails here.
	VOE_TEST_CHECK_INT((int)e.sheet_texture, (int)texture.index);
	VOE_TEST_CHECK_FLOAT(e.sheet.x, sheet.x, 0.001f);
	VOE_TEST_CHECK_FLOAT(e.sheet.y, sheet.y, 0.001f);
	VOE_TEST_CHECK_FLOAT(e.sheet.z, sheet.z, 0.001f);
	VOE_TEST_CHECK_FLOAT(e.sheet.w, sheet.w, 0.001f);

	// Opaque white, which shows the picture as it is.
	VOE_TEST_CHECK_FLOAT(e.colour.x, 1.0f, 0.001f);
	VOE_TEST_CHECK_FLOAT(e.colour.y, 1.0f, 0.001f);
	VOE_TEST_CHECK_FLOAT(e.colour.z, 1.0f, 0.001f);
	VOE_TEST_CHECK_FLOAT(e.colour.w, 1.0f, 0.001f);
}

static void two_images_are_two_records_in_call_order(voe_ui_context *ui,
						     voe_base_arena *arena)
{
	voe_ui_node one;
	voe_ui_node two;
	voe_ui_rect rect_one;
	voe_ui_rect rect_two;

	voe_ui_frame_begin(ui, arena);
	voe_ui_row_begin(ui, (voe_ui_container){
				     .size = { { VOE_UI_SIZE_FIXED,
						 IMAGE_ROW_WIDE },
					       { VOE_UI_SIZE_FIXED, 30.0f } },
				     .gap = 4.0f,
				     .pad = pad_all(5.0f) });
	one = voe_ui_image(ui, IMAGE_ONE_TEXTURE, IMAGE_ONE_SHEET,
			   (voe_math_float2){ 0 },
			   (voe_ui_sizing){
				   { VOE_UI_SIZE_FIXED, IMAGE_ONE_WIDE },
				   { VOE_UI_SIZE_FIXED, IMAGE_HIGH } });
	two = voe_ui_image(ui, IMAGE_TWO_TEXTURE, IMAGE_TWO_SHEET,
			   (voe_math_float2){ 0 },
			   (voe_ui_sizing){ { VOE_UI_SIZE_GROW, 1.0f },
					    { VOE_UI_SIZE_FIXED, IMAGE_HIGH } });
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));

	// The premise: layout put the two nodes where the numbers above say.
	rect_one = voe_ui_node_rect(ui, one);
	rect_two = voe_ui_node_rect(ui, two);
	VOE_TEST_CHECK_FLOAT(rect_one.min.x, 5.0f, 0.001f);
	VOE_TEST_CHECK_FLOAT(rect_one.size.x, IMAGE_ONE_WIDE, 0.001f);
	VOE_TEST_CHECK_FLOAT(rect_two.min.x, IMAGE_TWO_X, 0.001f);
	VOE_TEST_CHECK_FLOAT(rect_two.size.x, IMAGE_TWO_WIDE, 0.001f);

	// ONE RECORD PER IMAGE AND NOTHING FOR THE ROW, the first call's first.
	VOE_TEST_CHECK_INT((int)voe_ui_element_count(ui), 2);
	if (voe_ui_element_count(ui) != 2)
		return;

	check_image_record(voe_ui_element(ui, 0), 5.0f, IMAGE_ONE_WIDE,
			   IMAGE_ONE_TEXTURE, IMAGE_ONE_SHEET);
	check_image_record(voe_ui_element(ui, 1), IMAGE_TWO_X, IMAGE_TWO_WIDE,
			   IMAGE_TWO_TEXTURE, IMAGE_TWO_SHEET);
}

// ---------------------------------------------------------- the text scale

// The one case that needs a font, and so a device. See this file's header.
//
// A COLUMN OF TWO LABELS, MEASURED AT 1.0 AND AT 1.5. What must grow is the
// labels; what must not is the gap between them or the padding round them, and
// the way to say both at once is that the column's height grows by exactly the
// labels' growth and by nothing else.
#define SCALE_GAP 3.0f
#define SCALE_PAD 4.0f

static float labelled_column_height(voe_ui_context *ui, voe_base_arena *arena,
				    float scale)
{
	voe_ui_node column;

	voe_ui_text_scale_set(ui, scale);

	voe_ui_frame_begin(ui, arena);
	column = voe_ui_column_begin(ui, (voe_ui_container){
						.gap = SCALE_GAP,
						.pad = pad_all(SCALE_PAD) });
	voe_ui_label(ui, "Measure me");
	voe_ui_label(ui, "Measure me");
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));

	return voe_ui_node_rect(ui, column).size.y;
}

static void a_bigger_text_scale_grows_the_row_and_not_the_gaps(
	voe_ui_context *ui, voe_base_arena *arena)
{
	float one = labelled_column_height(ui, arena, 1.0f);
	float half_again = labelled_column_height(ui, arena, 1.5f);
	float labels_at_one = one - SCALE_GAP - 2.0f * SCALE_PAD;

	// The labels are real: a measured string has a height.
	VOE_TEST_CHECK(labels_at_one > 0.0f);

	// Half again as much room for the labels, and the gap and the padding
	// exactly where they were. If the scale had been applied to the whole
	// column instead, this would come out at 1.5 * one.
	VOE_TEST_CHECK_FLOAT(half_again,
			     labels_at_one * 1.5f + SCALE_GAP +
				     2.0f * SCALE_PAD,
			     0.01f);
	VOE_TEST_CHECK(half_again < one * 1.5f);

	voe_ui_text_scale_set(ui, 1.0f);
}

// A label is one record per character that draws and none for a space, and its
// letters come after the panel behind them.
static void a_label_emits_its_letters_after_the_panel(voe_ui_context *ui,
						      voe_base_arena *arena)
{
	voe_ui_frame_begin(ui, arena);
	voe_ui_panel_begin(ui, "panel", 0, PANEL,
			   (voe_ui_container){ .pad = pad_all(2.0f) });
	voe_ui_label(ui, "A B");
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));

	// The panel, then A, then B. The space costs nothing.
	VOE_TEST_CHECK_INT((int)voe_ui_element_count(ui), 3);
	if (voe_ui_element_count(ui) != 3)
		return;

	VOE_TEST_CHECK_INT((int)voe_ui_element(ui, 0).kind,
			   (int)VOE_RENDER_ELEMENT_SOLID);
	VOE_TEST_CHECK_INT((int)voe_ui_element(ui, 1).kind,
			   (int)VOE_RENDER_ELEMENT_GLYPH);
	VOE_TEST_CHECK_INT((int)voe_ui_element(ui, 2).kind,
			   (int)VOE_RENDER_ELEMENT_GLYPH);

	// Reading order, left to right.
	VOE_TEST_CHECK(voe_ui_element(ui, 2).bounds.x >
		       voe_ui_element(ui, 1).bounds.x);

	// Inside the panel and below its top edge, not above it: the baseline
	// is found by ADDING the ascender to the top of the label's rectangle,
	// and getting that sign wrong puts every letter above the panel.
	VOE_TEST_CHECK(voe_ui_element(ui, 1).bounds.y >=
		       voe_ui_element(ui, 0).bounds.y);
}

// A LABEL WHOLLY CLIPPED EMITS NOTHING. A panel 40 by 10 clipping both holds a
// spacer 30 tall and then a label, which begins 20 below the panel's bottom edge.
// Unclipped the same tree emits the panel and the label's letters; clipped it
// emits the panel alone, and the count says so.
static uint32_t clipped_label_elements(voe_ui_context *ui,
				       voe_base_arena *arena,
				       voe_ui_overflow_kind kind)
{
	voe_ui_frame_begin(ui, arena);
	voe_ui_panel_begin(ui, "clipper", 0, PANEL,
			   (voe_ui_container){
				   .size = { { VOE_UI_SIZE_FIXED, 10.0f },
					     { VOE_UI_SIZE_FIXED, 40.0f } },
				   .overflow = { kind, kind } });
	voe_ui_box(ui, (voe_math_float2){ 10.0f, 30.0f }, (voe_ui_sizing){ 0 });
	voe_ui_label(ui, "Hidden");
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));

	return voe_ui_element_count(ui);
}

static void a_wholly_clipped_label_emits_nothing(voe_ui_context *ui,
						 voe_base_arena *arena)
{
	VOE_TEST_CHECK(clipped_label_elements(ui, arena,
					      VOE_UI_OVERFLOW_VISIBLE) > 1);
	VOE_TEST_CHECK_INT((int)clipped_label_elements(ui, arena,
						       VOE_UI_OVERFLOW_CLIP),
			   1);
}

// The device and the font this one case needs, and the skip that stands in for
// them where there is no driver.
static int the_text_scale(voe_base_arena *arena)
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
			printf("skip: no graphics driver — the text-size, "
			       "label-emission and clipped-label checks did "
			       "not run\n");
			return 0;
		}
		VOE_TEST_CHECK(device != NULL);
		return 0;
	}

	font = voe_text_font_new(device, arena, &error);
	if (font == NULL) {
		VOE_TEST_CHECK(font != NULL);
		voe_render_device_destroy(device);
		return 0;
	}

	ui = voe_ui_context_new(arena, (voe_ui_capacities){ .nodes = 64,
							    .elements = 64 });
	voe_ui_font_set(ui, font);

	a_bigger_text_scale_grows_the_row_and_not_the_gaps(ui, arena);
	a_label_emits_its_letters_after_the_panel(ui, arena);
	a_wholly_clipped_label_emits_nothing(ui, arena);

	voe_text_font_destroy(font);
	voe_render_device_destroy(device);
	return 0;
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_ui_context *ui = voe_ui_context_new(
		arena, (voe_ui_capacities){ .nodes = 64, .elements = 64 });

	the_tree_is_where_the_tests_think_it_is(ui, arena);
	a_pointer_over_a_button_hovers_it(ui, arena);
	no_pointer_hovers_nothing(ui, arena);
	press_and_release_inside_fires_once(ui, arena);
	dragging_off_and_releasing_cancels(ui, arena);
	dragging_off_and_back_still_fires(ui, arena);
	releasing_over_another_button_fires_nothing(ui, arena);
	arriving_with_the_button_already_down_arms_nothing(ui, arena);
	a_button_that_stops_being_called_is_let_go(ui, arena);
	a_duplicate_key_refuses_the_frame(ui, arena);
	the_same_name_under_two_panels_is_two_widgets(ui, arena);
	the_number_box_is_where_the_tests_think_it_is(ui, arena);
	dragging_sideways_moves_the_value(ui, arena);
	movement_inside_the_dead_zone_changes_nothing(ui, arena);
	a_fine_drag_moves_a_tenth_as_far(ui, arena);
	a_press_and_release_without_movement_does_nothing(ui, arena);
	a_drag_past_the_edge_keeps_working(ui, arena);
	arriving_with_the_button_already_down_arms_no_number(ui, arena);
	a_number_box_that_stops_being_called_is_let_go(ui, arena);
	a_number_and_a_button_sharing_a_key_refuse_the_frame(ui, arena);
	a_known_tree_emits_a_known_list(ui, arena);
	a_transparent_panel_emits_nothing(ui, arena);
	two_images_are_two_records_in_call_order(ui, arena);
	a_half_clipped_button_emits_its_visible_half(ui, arena);
	a_pointer_over_the_clipped_half_hovers_nothing(ui, arena);
	a_number_box_scrolled_away_mid_drag_keeps_dragging(ui, arena);
	too_many_elements_refuses_the_frame(arena);

	(void)the_text_scale(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
