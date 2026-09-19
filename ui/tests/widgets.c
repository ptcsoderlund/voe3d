// The widgets, and the two things about them that are worth pinning down: what
// a press and a release do in every awkward order a hand can produce, and that
// a known tree comes out as a known list of records — two images in a row among
// them, each one IMAGE record carrying the texture index and sheet it was given.
//
// EVERY CLICK, DRAG AND SCROLL CASE NEEDS NO GRAPHICS CARD AND NO WINDOW
// SYSTEM, which is the property the whole design is arranged around: the
// pointer is a value handed in (ADR-0093), so a drag is three calls in a row
// and not a mouse. A button is composed rather than given a string, so nothing
// in the click cases touches a font. Nothing here opens a window, asks a
// compositor for anything or names `platform`.
//
// A CASE THAT MEASURES A STRING IS THE EXCEPTION AND IT IS DELIBERATE
// (ADR-0106). The text scale multiplies a MEASUREMENT, a label emits one
// record per letter, and A FIELD COMPOSES A LABEL OF ITS OWN, so proving any
// of the three needs a real font, and a font uploads an atlas and so needs a
// device. Those cases take a headless one — no window, no surface, no
// compositor — and where there is no driver at all they skip, saying which
// check did not run rather than only why.
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
// A FIELD FOCUSES ON THE PRESS ITSELF, NOT ON A RELEASE, and the cases below
// pin that down before anything about editing: a press elsewhere — nothing, a
// button, wherever — clears it on the very same edge a press on the field
// sets it, and a field not called this frame loses it exactly as a scroll
// area not called forgets its offset. TWO FIELDS IN ONE FRAME, ONLY ONE
// FOCUSED, is the case that would catch a shared buffer: typing must reach
// the focused one and leave the other's own text untouched. BACKSPACE ON AN
// EMPTY FIELD AND A CODE POINT TAKEN WHOLE both come from the same walk
// backward over continuation bytes, so both are pinned down rather than
// trusted to follow from one another. A TEXT AT CAPACITY REFUSING THE NEXT
// CODE POINT WHOLE is the one that would show a cut multi-byte character if
// the append ever stopped counting in bytes instead of code points. AND
// `changed` FALSE HANDS BACK THE CALLER'S OWN POINTER, not a copy of the same
// bytes, which is what lets a caller write the answer back every frame with no
// cost on the frames that changed nothing. Every field case needs the device,
// a field always composing a label of its own — see above.
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
#include <string.h>

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

// The theme every context in this file draws with, set once in main() —
// every case that opens a panel, a button, a number box or a field needs one
// in force, and the roles it derives to are what a known list's colours are
// checked against.
static voe_ui_theme TEST_THEME;

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

// Two buttons under the same parent with the same name and the same index. The
// frame is refused rather than the two of them quietly behaving as one.
static void a_duplicate_key_refuses_the_frame(voe_ui_context *ui,
					      voe_base_arena *arena)
{
	bool ok;

	fprintf(stderr, "-- the next voe_ui line is this test's own --\n");

	voe_ui_frame_begin(ui, arena);
	voe_ui_panel_begin(ui, "panel", 0, VOE_UI_SURFACE_SURFACE,
			   (voe_ui_container){ 0 });
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
	voe_ui_panel_begin(ui, "panel", 0, VOE_UI_SURFACE_SURFACE,
			   (voe_ui_container){ 0 });
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

	voe_ui_panel_begin(ui, "left", 0, VOE_UI_SURFACE_SURFACE,
			   (voe_ui_container){ 0 });
	first = voe_ui_button_begin(ui, "go", 0);
	voe_ui_box(ui, (voe_math_float2){ BOX_WIDE, BOX_HIGH },
		   (voe_ui_sizing){ 0 });
	voe_ui_end(ui);
	voe_ui_end(ui);

	voe_ui_panel_begin(ui, "right", 0, VOE_UI_SURFACE_SURFACE,
			   (voe_ui_container){ 0 });
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

// Half a millimetre is not a drag, and this is what keeps a click a click. Two
// frames inside the zone, so that a slow hand crossing it in small steps is
// covered as well as one that never leaves it. Its release, inside the zone,
// opens the box for typing, whose next frame composes a label — so it runs
// among the device cases and not beside the drags around it.
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
	voe_ui_panel_begin(ui, "panel", 0, VOE_UI_SURFACE_SURFACE,
			   (voe_ui_container){ 0 });
	voe_ui_button_begin(ui, "same", 0);
	voe_ui_end(ui);
	voe_ui_number_begin(ui, "same", 0, START, PER_MM);
	voe_ui_end(ui);
	voe_ui_end(ui);

	VOE_TEST_CHECK(!voe_ui_frame_end(ui));

	// And the next frame is fine, which is what a refusal has to mean.
	voe_ui_frame_begin(ui, arena);
	voe_ui_panel_begin(ui, "panel", 0, VOE_UI_SURFACE_SURFACE,
			   (voe_ui_container){ 0 });
	voe_ui_button_begin(ui, "same", 0);
	voe_ui_end(ui);
	voe_ui_number_begin(ui, "same", 1, START, PER_MM);
	voe_ui_end(ui);
	voe_ui_end(ui);

	VOE_TEST_CHECK(voe_ui_frame_end(ui));
}

// A known tree, a known list. Six records: the panel's border and its fill,
// then each button's border and its fill, in the order they were called
// (ADR-0171: a bordered widget is two records and not one). The boxes inside
// the buttons draw nothing, and nothing is emitted for them.
static void a_known_tree_emits_a_known_list(voe_ui_context *ui,
					    voe_base_arena *arena)
{
	struct frame f = build(ui, arena, IN_A, true, false, 2);
	voe_render_element panel_border;
	voe_render_element panel_fill;
	voe_render_element a_border;
	voe_render_element a_fill;
	voe_render_element b_border;
	voe_render_element b_fill;

	VOE_TEST_CHECK(f.ok);
	VOE_TEST_CHECK_INT((int)voe_ui_element_count(ui), 6);
	if (voe_ui_element_count(ui) != 6)
		return;

	panel_border = voe_ui_element(ui, 0);
	panel_fill = voe_ui_element(ui, 1);
	a_border = voe_ui_element(ui, 2);
	a_fill = voe_ui_element(ui, 3);
	b_border = voe_ui_element(ui, 4);
	b_fill = voe_ui_element(ui, 5);

	// THE PANEL'S BORDER AND FILL COME FIRST, IN THAT ORDER, which is the
	// claim: paint order is submission order, a parent is submitted before
	// its children, and a border is submitted before the fill drawn over
	// its middle. Reversed at either level, something would cover what
	// should be in front of it.
	VOE_TEST_CHECK_FLOAT(panel_border.bounds.z, 33.0f, 0.001f);
	VOE_TEST_CHECK_FLOAT(panel_border.bounds.w, 41.0f, 0.001f);
	VOE_TEST_CHECK_FLOAT(panel_fill.bounds.z, 33.0f - 0.6f, 0.001f);
	VOE_TEST_CHECK_FLOAT(panel_fill.bounds.w, 41.0f - 0.6f, 0.001f);
	VOE_TEST_CHECK_FLOAT(panel_border.colour.x, TEST_THEME.border.x, 0.001f);
	VOE_TEST_CHECK_FLOAT(panel_fill.colour.x, TEST_THEME.surface.x, 0.001f);

	// A COPY AND NOT A CONVERSION: min is the xy and size is the zw, and
	// there is no arithmetic between voe_ui_node_rect and a border's own
	// bounds.
	VOE_TEST_CHECK_FLOAT(a_border.bounds.x, 4.0f, 0.001f);
	VOE_TEST_CHECK_FLOAT(a_border.bounds.y, A_Y, 0.001f);
	VOE_TEST_CHECK_FLOAT(a_border.bounds.z, BUTTON_WIDE, 0.001f);
	VOE_TEST_CHECK_FLOAT(a_border.bounds.w, BUTTON_HIGH, 0.001f);
	VOE_TEST_CHECK_FLOAT(b_border.bounds.y, B_Y, 0.001f);

	// NOT A ZEROED CLIP, WHICH WOULD CLIP EVERY ONE OF THEM AWAY. Its own
	// bounds, so nothing is clipped and card 035 has something to narrow.
	VOE_TEST_CHECK_FLOAT(a_border.clip.x, a_border.bounds.x, 0.001f);
	VOE_TEST_CHECK_FLOAT(a_border.clip.z, a_border.bounds.z, 0.001f);

	VOE_TEST_CHECK_INT((int)a_border.kind, (int)VOE_RENDER_ELEMENT_SOLID);

	// The hovered button's fill is not the same colour as the other one's,
	// which is the whole of what a visual state is; their borders, being
	// the border role whatever their state, are the same.
	VOE_TEST_CHECK(a_fill.colour.x != b_fill.colour.x ||
		       a_fill.colour.y != b_fill.colour.y ||
		       a_fill.colour.z != b_fill.colour.z);
	VOE_TEST_CHECK_FLOAT(a_border.colour.x, b_border.colour.x, 0.001f);
}

// NONE is not a transparent rectangle. It is no record at all.
static void a_transparent_panel_emits_nothing(voe_ui_context *ui,
					      voe_base_arena *arena)
{
	voe_ui_frame_begin(ui, arena);
	voe_ui_panel_begin(ui, "invisible", 0, VOE_UI_SURFACE_NONE,
			   (voe_ui_container){ .pad = pad_all(6.0f) });
	voe_ui_box(ui, (voe_math_float2){ 10.0f, 10.0f }, (voe_ui_sizing){ 0 });
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));

	VOE_TEST_CHECK_INT((int)voe_ui_element_count(ui), 0);
}

// ------------------------------------------------------------- the theme

// A theme with a different surface_separation than TEST_THEME's, so its
// `control` role is a different lightness — the signal these cases read to
// tell which theme a widget actually drew with.
static voe_ui_theme derive_variant(float surface_separation)
{
	voe_ui_theme_inputs inputs = voe_ui_theme_default_inputs();

	inputs.surface_separation = surface_separation;
	return voe_ui_theme_derive(&inputs, NULL);
}

// THE NEAREST THEME WINS (ADR-0170), AND A POP RESTORES THE ONE ABOVE IT.
// Three buttons in a row: the first before any push, reading TEST_THEME; the
// second between a push and its pop, reading the pushed theme; the third
// after the pop, back to TEST_THEME. A button's fill is the second of its two
// records (its border comes first), and it is `control` at rest.
static void the_nearest_theme_wins_and_a_pop_restores_it(
	voe_ui_context *ui, voe_base_arena *arena)
{
	voe_ui_theme inner = derive_variant(2.0f);

	VOE_TEST_CHECK(inner.control.x != TEST_THEME.control.x);

	voe_ui_frame_begin(ui, arena);
	voe_ui_row_begin(ui, (voe_ui_container){ .gap = 1.0f });
	voe_ui_button_begin(ui, "outer", 0);
	voe_ui_end(ui);
	voe_ui_theme_push(ui, &inner);
	voe_ui_button_begin(ui, "inner", 0);
	voe_ui_end(ui);
	voe_ui_theme_pop(ui);
	voe_ui_button_begin(ui, "after", 0);
	voe_ui_end(ui);
	voe_ui_end(ui);

	VOE_TEST_CHECK(voe_ui_frame_end(ui));
	VOE_TEST_CHECK_INT((int)voe_ui_element_count(ui), 6);
	if (voe_ui_element_count(ui) != 6)
		return;

	VOE_TEST_CHECK_FLOAT(voe_ui_element(ui, 1).colour.x,
			     TEST_THEME.control.x, 0.001f);
	VOE_TEST_CHECK_FLOAT(voe_ui_element(ui, 3).colour.x, inner.control.x,
			     0.001f);
	VOE_TEST_CHECK_FLOAT(voe_ui_element(ui, 5).colour.x,
			     TEST_THEME.control.x, 0.001f);
}

// AN UNMATCHED PUSH REFUSES THE FRAME the way a duplicate key does: reported
// once on stderr, every rectangle in the frame nought, and the next frame
// laying out normally.
static void an_unbalanced_push_refuses_the_frame(voe_ui_context *ui,
						 voe_base_arena *arena)
{
	fprintf(stderr, "-- the next voe_ui line is this test's own --\n");

	voe_ui_frame_begin(ui, arena);
	voe_ui_row_begin(ui, (voe_ui_container){ 0 });
	voe_ui_theme_push(ui, &TEST_THEME);
	voe_ui_button_begin(ui, "unpopped", 0);
	voe_ui_end(ui);
	voe_ui_end(ui);

	VOE_TEST_CHECK(!voe_ui_frame_end(ui));

	// And the next frame is fine, which is what a refusal has to mean.
	voe_ui_frame_begin(ui, arena);
	voe_ui_row_begin(ui, (voe_ui_container){ 0 });
	voe_ui_button_begin(ui, "unpopped", 0);
	voe_ui_end(ui);
	voe_ui_end(ui);

	VOE_TEST_CHECK(voe_ui_frame_end(ui));
}

// A frame that emits more than it was given is refused, the same way one that
// wants more nodes is.
static void too_many_elements_refuses_the_frame(voe_base_arena *arena)
{
	voe_ui_context *ui = voe_ui_context_new(
		arena, (voe_ui_capacities){ .nodes = 8, .elements = 1 });

	voe_ui_theme_set(ui, &TEST_THEME);
	fprintf(stderr, "-- the next voe_ui line is this test's own --\n");

	voe_ui_frame_begin(ui, arena);
	voe_ui_panel_begin(ui, "panel", 0, VOE_UI_SURFACE_SURFACE,
			   (voe_ui_container){ 0 });
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

// --------------------------------------------------------- the scroll area

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

// ---------------------------------------------------------- the theme's size

// The one case that needs a font, and so a device. See this file's header.
//
// A COLUMN OF TWO LABELS, MEASURED UNDER A PUSHED THEME OF A GIVEN text_size.
// What must grow is the labels; what must not is the gap between them or the
// padding round them, and the way to say both at once is that the column's
// height grows by exactly the labels' growth and by nothing else.
#define SCALE_GAP 3.0f
#define SCALE_PAD 4.0f

static float labelled_column_height(voe_ui_context *ui, voe_base_arena *arena,
				    const voe_ui_theme *theme)
{
	voe_ui_node column;

	voe_ui_frame_begin(ui, arena);
	voe_ui_theme_push(ui, theme);
	column = voe_ui_column_begin(ui, (voe_ui_container){
						.gap = SCALE_GAP,
						.pad = pad_all(SCALE_PAD) });
	voe_ui_label(ui, "Measure me");
	voe_ui_label(ui, "Measure me");
	voe_ui_end(ui);
	voe_ui_theme_pop(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));

	return voe_ui_node_rect(ui, column).size.y;
}

static void a_bigger_theme_grows_the_row_and_not_the_gaps(
	voe_ui_context *ui, voe_base_arena *arena)
{
	voe_ui_theme_inputs inputs = voe_ui_theme_default_inputs();
	voe_ui_theme normal = voe_ui_theme_derive(&inputs, NULL);
	voe_ui_theme bigger;
	float one;
	float half_again;
	float labels_at_one;

	inputs.text_size *= 1.5f;
	bigger = voe_ui_theme_derive(&inputs, NULL);

	one = labelled_column_height(ui, arena, &normal);
	half_again = labelled_column_height(ui, arena, &bigger);
	labels_at_one = one - SCALE_GAP - 2.0f * SCALE_PAD;

	// The labels are real: a measured string has a height.
	VOE_TEST_CHECK(labels_at_one > 0.0f);

	// Half again as much room for the labels, and the gap and the padding
	// exactly where they were. If the bigger text_size had been applied to
	// the whole column instead, this would come out at 1.5 * one.
	VOE_TEST_CHECK_FLOAT(half_again,
			     labels_at_one * 1.5f + SCALE_GAP +
				     2.0f * SCALE_PAD,
			     0.01f);
	VOE_TEST_CHECK(half_again < one * 1.5f);
}

// A label is one record per character that draws and none for a space, and its
// letters come after the panel's border and fill behind them.
static void a_label_emits_its_letters_after_the_panel(voe_ui_context *ui,
						      voe_base_arena *arena)
{
	voe_ui_frame_begin(ui, arena);
	voe_ui_panel_begin(ui, "panel", 0, VOE_UI_SURFACE_SURFACE,
			   (voe_ui_container){ .pad = pad_all(2.0f) });
	voe_ui_label(ui, "A B");
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));

	// The panel's border, its fill, then A, then B. The space costs
	// nothing.
	VOE_TEST_CHECK_INT((int)voe_ui_element_count(ui), 4);
	if (voe_ui_element_count(ui) != 4)
		return;

	VOE_TEST_CHECK_INT((int)voe_ui_element(ui, 0).kind,
			   (int)VOE_RENDER_ELEMENT_SOLID);
	VOE_TEST_CHECK_INT((int)voe_ui_element(ui, 1).kind,
			   (int)VOE_RENDER_ELEMENT_SOLID);
	VOE_TEST_CHECK_INT((int)voe_ui_element(ui, 2).kind,
			   (int)VOE_RENDER_ELEMENT_GLYPH);
	VOE_TEST_CHECK_INT((int)voe_ui_element(ui, 3).kind,
			   (int)VOE_RENDER_ELEMENT_GLYPH);

	// Reading order, left to right.
	VOE_TEST_CHECK(voe_ui_element(ui, 3).bounds.x >
		       voe_ui_element(ui, 2).bounds.x);

	// Inside the panel and below its top edge, not above it: the baseline
	// is found by ADDING the ascender to the top of the label's rectangle,
	// and getting that sign wrong puts every letter above the panel.
	VOE_TEST_CHECK(voe_ui_element(ui, 2).bounds.y >=
		       voe_ui_element(ui, 0).bounds.y);
}

// A LABEL WHOLLY CLIPPED EMITS NOTHING. A panel 40 by 10 clipping both holds a
// spacer 30 tall and then a label, which begins 20 below the panel's bottom edge.
// Unclipped the same tree emits the panel's border and fill and the label's
// letters; clipped it emits the panel's border and fill alone, and the count
// says so.
static uint32_t clipped_label_elements(voe_ui_context *ui,
				       voe_base_arena *arena,
				       voe_ui_overflow_kind kind)
{
	voe_ui_frame_begin(ui, arena);
	voe_ui_panel_begin(ui, "clipper", 0, VOE_UI_SURFACE_SURFACE,
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
					      VOE_UI_OVERFLOW_VISIBLE) > 2);
	VOE_TEST_CHECK_INT((int)clipped_label_elements(ui, arena,
						       VOE_UI_OVERFLOW_CLIP),
			   2);
}

// A LABEL IN ACCENT DRAWS IN THE ACCENT (ADR-0171's role list, task 4's
// voe_ui_text_role), rather than the ordinary text_primary voe_ui_label
// itself draws in.
static void a_label_in_accent_draws_the_accent(voe_ui_context *ui,
					       voe_base_arena *arena)
{
	voe_ui_frame_begin(ui, arena);
	voe_ui_column_begin(ui, (voe_ui_container){ 0 });
	voe_ui_label_role(ui, "A", VOE_UI_TEXT_ROLE_ACCENT);
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));

	VOE_TEST_CHECK_INT((int)voe_ui_element_count(ui), 1);
	if (voe_ui_element_count(ui) != 1)
		return;

	VOE_TEST_CHECK_FLOAT(voe_ui_element(ui, 0).colour.x, TEST_THEME.accent.x,
			     0.001f);
	VOE_TEST_CHECK_FLOAT(voe_ui_element(ui, 0).colour.y, TEST_THEME.accent.y,
			     0.001f);
	VOE_TEST_CHECK_FLOAT(voe_ui_element(ui, 0).colour.z, TEST_THEME.accent.z,
			     0.001f);
}

// ---------------------------------------------------------------- the field

// A field 25 x 15 at the origin of a bare panel, so a point inside it and one
// well outside it are known without asking layout — the same premise the
// button cases rest on. A field always composes a label, which is why every
// case in this section runs inside the device group below rather than beside
// the click cases above.
#define FIELD_WIDE 25.0f
#define FIELD_HIGH 15.0f
#define IN_FIELD ((voe_math_float2){ 12.0f, 7.0f })
#define OUTSIDE_FIELD ((voe_math_float2){ 100.0f, 100.0f })

static const voe_ui_keyboard NO_KEYS = { 0 };

static voe_ui_sizing field_sizing(void)
{
	return (voe_ui_sizing){ { VOE_UI_SIZE_FIXED, FIELD_WIDE },
				{ VOE_UI_SIZE_FIXED, FIELD_HIGH } };
}

struct field_frame {
	voe_ui_node f;
	bool ok;
};

static struct field_frame build_field(voe_ui_context *ui,
				      voe_base_arena *arena,
				      voe_math_float2 at, bool over, bool down,
				      const char *text, voe_ui_keyboard keyboard)
{
	struct field_frame f = { VOE_UI_NODE_NONE, false };

	voe_ui_frame_begin(ui, arena);
	voe_ui_pointer_set(ui, (voe_ui_pointer){ .at = at,
						 .over = over,
						 .down = down });
	voe_ui_keyboard_set(ui, keyboard);

	voe_ui_panel_begin(ui, "panel", 0, VOE_UI_SURFACE_SURFACE,
			   (voe_ui_container){ 0 });
	f.f = voe_ui_field(ui, "name", 0, text, field_sizing());
	voe_ui_end(ui);

	f.ok = voe_ui_frame_end(ui);
	return f;
}

// A press inside it focuses it, on the press and not on the release that
// follows; a press outside it — nothing there, just empty panel — clears it
// on that same edge.
static void a_press_focuses_and_a_press_elsewhere_unfocuses(
	voe_ui_context *ui, voe_base_arena *arena)
{
	struct field_frame f =
		build_field(ui, arena, IN_FIELD, true, false, "hi", NO_KEYS);

	VOE_TEST_CHECK(!voe_ui_field_action(ui, f.f).focused);

	f = build_field(ui, arena, IN_FIELD, true, true, "hi", NO_KEYS);
	VOE_TEST_CHECK(voe_ui_field_action(ui, f.f).focused);

	// Let go: a release does not clear it, unlike a button's press.
	f = build_field(ui, arena, IN_FIELD, true, false, "hi", NO_KEYS);
	VOE_TEST_CHECK(voe_ui_field_action(ui, f.f).focused);

	// A press outside it, on empty panel, clears it.
	f = build_field(ui, arena, OUTSIDE_FIELD, true, true, "hi", NO_KEYS);
	VOE_TEST_CHECK(!voe_ui_field_action(ui, f.f).focused);
}

static void typed_bytes_are_appended_in_order(voe_ui_context *ui,
					      voe_base_arena *arena)
{
	voe_ui_keyboard type_ab = { .text = "ab", .size = 2 };
	voe_ui_field_result r;
	struct field_frame f;

	// Nothing is focused before the press, so the press below is an edge
	// and not whatever the case before this left behind.
	f = build_field(ui, arena, IN_FIELD, true, false, "", NO_KEYS);
	VOE_TEST_CHECK(!voe_ui_field_action(ui, f.f).focused);

	f = build_field(ui, arena, IN_FIELD, true, true, "", NO_KEYS);
	VOE_TEST_CHECK(voe_ui_field_action(ui, f.f).focused);

	f = build_field(ui, arena, IN_FIELD, false, false, "", type_ab);
	r = voe_ui_field_action(ui, f.f);
	VOE_TEST_CHECK(r.changed);
	VOE_TEST_CHECK(strcmp(r.text, "ab") == 0);
}

// Two fields, one focused: typing must reach the one focused — replacing
// its whole text, the focus having just arrived — and leave the other's own
// text exactly as it was handed in.
static void two_fields_only_the_focused_one_changes(voe_ui_context *ui,
						    voe_base_arena *arena)
{
	voe_ui_keyboard type_x = { .text = "x", .size = 1 };
	voe_ui_node a;
	voe_ui_node b;
	voe_ui_field_result ra;
	voe_ui_field_result rb;

	voe_ui_frame_begin(ui, arena);
	voe_ui_pointer_set(ui, (voe_ui_pointer){ .at = IN_FIELD,
						 .over = true,
						 .down = true });
	voe_ui_column_begin(ui, (voe_ui_container){ 0 });
	a = voe_ui_field(ui, "a", 0, "one", field_sizing());
	(void)voe_ui_field(ui, "b", 0, "two", field_sizing());
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));
	VOE_TEST_CHECK(voe_ui_field_action(ui, a).focused);

	voe_ui_frame_begin(ui, arena);
	voe_ui_pointer_set(ui, (voe_ui_pointer){ 0 });
	voe_ui_keyboard_set(ui, type_x);
	voe_ui_column_begin(ui, (voe_ui_container){ 0 });
	a = voe_ui_field(ui, "a", 0, "one", field_sizing());
	b = voe_ui_field(ui, "b", 0, "two", field_sizing());
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));

	ra = voe_ui_field_action(ui, a);
	rb = voe_ui_field_action(ui, b);
	VOE_TEST_CHECK(ra.focused);
	VOE_TEST_CHECK(ra.changed);
	VOE_TEST_CHECK(strcmp(ra.text, "x") == 0);
	VOE_TEST_CHECK(!rb.focused);
	VOE_TEST_CHECK(!rb.changed);
	VOE_TEST_CHECK(strcmp(rb.text, "two") == 0);
}

// Focuses the one field of build_field, text `text`, and lets go. A press
// outside first, so the focus arrives rather than staying where the case
// before left it.
static void focus_field(voe_ui_context *ui, voe_base_arena *arena,
			const char *text)
{
	(void)build_field(ui, arena, OUTSIDE_FIELD, true, true, text, NO_KEYS);
	(void)build_field(ui, arena, OUTSIDE_FIELD, true, false, text, NO_KEYS);
	(void)build_field(ui, arena, IN_FIELD, true, true, text, NO_KEYS);
	(void)build_field(ui, arena, IN_FIELD, true, false, text, NO_KEYS);
}

// The trailing continuation byte and the byte before it both go: "aö" loses
// the whole of "ö" (0xC3 0xB6) and not just its last byte.
static void backspace_takes_a_two_byte_code_point_whole(voe_ui_context *ui,
							voe_base_arena *arena)
{
	voe_ui_keyboard erase = { .backspace = true };
	voe_ui_keyboard type_it = { .text = "a\xc3" "\xb6", .size = 3 };
	struct field_frame f;
	voe_ui_field_result r;

	// Typed rather than handed in, so no selection is left for the
	// Backspace to empty.
	focus_field(ui, arena, "");
	f = build_field(ui, arena, IN_FIELD, true, false, "", type_it);
	VOE_TEST_CHECK(voe_ui_field_action(ui, f.f).focused);

	f = build_field(ui, arena, IN_FIELD, false, false, "", erase);
	r = voe_ui_field_action(ui, f.f);
	VOE_TEST_CHECK(r.changed);
	VOE_TEST_CHECK(strcmp(r.text, "a") == 0);
}

static void backspace_on_an_empty_text_does_nothing(voe_ui_context *ui,
						    voe_base_arena *arena)
{
	voe_ui_keyboard erase = { .backspace = true };
	struct field_frame f;

	focus_field(ui, arena, "");
	f = build_field(ui, arena, IN_FIELD, true, false, "", NO_KEYS);
	VOE_TEST_CHECK(voe_ui_field_action(ui, f.f).focused);

	f = build_field(ui, arena, IN_FIELD, false, false, "", erase);
	VOE_TEST_CHECK(!voe_ui_field_action(ui, f.f).changed);
}

// A text already at capacity refuses the next code point whole rather than
// cutting it at a byte the capacity happens to allow.
static void a_text_at_capacity_refuses_the_next_code_point_whole(
	voe_ui_context *ui, voe_base_arena *arena)
{
	static char full[VOE_UI_FIELD_CAPACITY + 1];
	voe_ui_keyboard type_one = { .text = "x", .size = 1 };
	struct field_frame f;
	voe_ui_field_result r;

	memset(full, 'a', VOE_UI_FIELD_CAPACITY);
	full[VOE_UI_FIELD_CAPACITY] = '\0';

	voe_ui_keyboard type_full = { .text = full,
				      .size = VOE_UI_FIELD_CAPACITY };

	// Typed to capacity rather than handed in, so no selection is left
	// for the next letter to replace.
	focus_field(ui, arena, "");
	f = build_field(ui, arena, IN_FIELD, true, false, "", type_full);
	VOE_TEST_CHECK(voe_ui_field_action(ui, f.f).focused);

	f = build_field(ui, arena, IN_FIELD, false, false, "", type_one);
	r = voe_ui_field_action(ui, f.f);
	VOE_TEST_CHECK(!r.changed);
	VOE_TEST_CHECK(strlen(r.text) == VOE_UI_FIELD_CAPACITY);
}

// Enter is true for exactly the frame it arrived on, and only while focused;
// it changes no text, commits, and drops the focus.
static void entered_is_true_only_on_the_frame_enter_arrived(
	voe_ui_context *ui, voe_base_arena *arena)
{
	voe_ui_keyboard press_enter = { .enter = true };
	struct field_frame f;
	voe_ui_field_result r;

	focus_field(ui, arena, "hi");
	f = build_field(ui, arena, IN_FIELD, true, false, "hi", NO_KEYS);
	VOE_TEST_CHECK(!voe_ui_field_action(ui, f.f).entered);

	f = build_field(ui, arena, IN_FIELD, false, false, "hi", press_enter);
	r = voe_ui_field_action(ui, f.f);
	VOE_TEST_CHECK(r.entered);
	VOE_TEST_CHECK(r.committed);
	VOE_TEST_CHECK(!r.changed);
	VOE_TEST_CHECK(!r.focused);

	f = build_field(ui, arena, IN_FIELD, false, false, "hi", NO_KEYS);
	VOE_TEST_CHECK(!voe_ui_field_action(ui, f.f).entered);
}

// A field not called this frame is no longer the focus, exactly as a scroll
// area not called forgets its offset.
static void a_field_not_called_loses_focus(voe_ui_context *ui,
					   voe_base_arena *arena)
{
	struct field_frame f;
	bool ok;

	(void)build_field(ui, arena, IN_FIELD, true, true, "hi", NO_KEYS);
	f = build_field(ui, arena, IN_FIELD, true, false, "hi", NO_KEYS);
	VOE_TEST_CHECK(voe_ui_field_action(ui, f.f).focused);

	// A frame with no field in it at all.
	voe_ui_frame_begin(ui, arena);
	voe_ui_panel_begin(ui, "panel", 0, VOE_UI_SURFACE_SURFACE,
			   (voe_ui_container){ 0 });
	voe_ui_end(ui);
	ok = voe_ui_frame_end(ui);
	VOE_TEST_CHECK(ok);

	f = build_field(ui, arena, OUTSIDE_FIELD, false, false, "hi", NO_KEYS);
	VOE_TEST_CHECK(!voe_ui_field_action(ui, f.f).focused);
}

// voe_ui_field_focus takes the keyboard to a field with no press at all —
// the frame a panel holding the one field first opens.
static void voe_ui_field_focus_takes_it(voe_ui_context *ui,
					voe_base_arena *arena)
{
	voe_ui_node f;

	voe_ui_frame_begin(ui, arena);
	voe_ui_pointer_set(ui, (voe_ui_pointer){ 0 });
	voe_ui_panel_begin(ui, "panel", 0, VOE_UI_SURFACE_SURFACE,
			   (voe_ui_container){ 0 });
	f = voe_ui_field(ui, "name", 0, "hi", field_sizing());
	voe_ui_field_focus(ui, f);
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));

	VOE_TEST_CHECK(voe_ui_field_action(ui, f).focused);
}

// A field that is not focused hands back the exact pointer the call was
// given, not a copy of the same bytes — so a caller may write the answer back
// every frame at no cost.
static void unfocused_hands_back_the_callers_own_pointer(
	voe_ui_context *ui, voe_base_arena *arena)
{
	static const char hello[] = "hello";
	struct field_frame f;
	voe_ui_field_result r;

	build_field(ui, arena, OUTSIDE_FIELD, true, true, hello, NO_KEYS);
	f = build_field(ui, arena, OUTSIDE_FIELD, false, false, hello, NO_KEYS);
	r = voe_ui_field_action(ui, f.f);
	VOE_TEST_CHECK(!r.focused);
	VOE_TEST_CHECK(!r.changed);
	VOE_TEST_CHECK(r.text == hello);
}

// The focus arrives with the whole text selected: the first typed text
// replaces it, and the next is appended as before.
static void typing_into_a_newly_focused_field_replaces_its_text(
	voe_ui_context *ui, voe_base_arena *arena)
{
	voe_ui_keyboard type_x = { .text = "x", .size = 1 };
	voe_ui_keyboard type_y = { .text = "y", .size = 1 };
	struct field_frame f;
	voe_ui_field_result r;

	focus_field(ui, arena, "hello");
	f = build_field(ui, arena, IN_FIELD, true, false, "hello", type_x);
	r = voe_ui_field_action(ui, f.f);
	VOE_TEST_CHECK(r.changed);
	VOE_TEST_CHECK(strcmp(r.text, "x") == 0);

	// The caller's "hello" is not read while focused.
	f = build_field(ui, arena, IN_FIELD, true, false, "hello", type_y);
	r = voe_ui_field_action(ui, f.f);
	VOE_TEST_CHECK(strcmp(r.text, "xy") == 0);
}

static void backspace_on_a_newly_focused_field_empties_it(
	voe_ui_context *ui, voe_base_arena *arena)
{
	voe_ui_keyboard erase = { .backspace = true };
	struct field_frame f;
	voe_ui_field_result r;

	focus_field(ui, arena, "hello");
	f = build_field(ui, arena, IN_FIELD, true, false, "hello", erase);
	r = voe_ui_field_action(ui, f.f);
	VOE_TEST_CHECK(r.changed);
	VOE_TEST_CHECK(strcmp(r.text, "") == 0);
}

// Escape cancels: the focus drops, `text` is the caller's own that frame,
// and the next frame shows the caller's text again.
static void escape_cancels_and_the_callers_text_stands(voe_ui_context *ui,
						       voe_base_arena *arena)
{
	static const char hello[] = "hello";
	voe_ui_keyboard type_x = { .text = "x", .size = 1 };
	voe_ui_keyboard escape = { .escape = true };
	struct field_frame f;
	voe_ui_field_result r;

	focus_field(ui, arena, hello);
	(void)build_field(ui, arena, IN_FIELD, true, false, hello, type_x);
	f = build_field(ui, arena, IN_FIELD, true, false, hello, escape);
	r = voe_ui_field_action(ui, f.f);
	VOE_TEST_CHECK(r.cancelled);
	VOE_TEST_CHECK(!r.committed);
	VOE_TEST_CHECK(!r.focused);
	VOE_TEST_CHECK(r.text == hello);

	f = build_field(ui, arena, IN_FIELD, true, false, hello, NO_KEYS);
	r = voe_ui_field_action(ui, f.f);
	VOE_TEST_CHECK(!r.focused);
	VOE_TEST_CHECK(!r.cancelled);
	VOE_TEST_CHECK(r.text == hello);
}

// Enter and a press elsewhere each commit with the typed text.
static void enter_and_a_press_elsewhere_commit_the_typed_text(
	voe_ui_context *ui, voe_base_arena *arena)
{
	voe_ui_keyboard type_x = { .text = "x", .size = 1 };
	voe_ui_keyboard press_enter = { .enter = true };
	struct field_frame f;
	voe_ui_field_result r;

	focus_field(ui, arena, "hello");
	(void)build_field(ui, arena, IN_FIELD, true, false, "hello", type_x);
	f = build_field(ui, arena, IN_FIELD, true, false, "hello", press_enter);
	r = voe_ui_field_action(ui, f.f);
	VOE_TEST_CHECK(r.committed);
	VOE_TEST_CHECK(!r.focused);
	VOE_TEST_CHECK(strcmp(r.text, "x") == 0);

	focus_field(ui, arena, "hello");
	(void)build_field(ui, arena, IN_FIELD, true, false, "hello", type_x);
	f = build_field(ui, arena, OUTSIDE_FIELD, true, true, "hello", NO_KEYS);
	r = voe_ui_field_action(ui, f.f);
	VOE_TEST_CHECK(r.committed);
	VOE_TEST_CHECK(!r.entered);
	VOE_TEST_CHECK(!r.focused);
	VOE_TEST_CHECK(strcmp(r.text, "x") == 0);
}

struct two_fields {
	voe_ui_field_result a;
	voe_ui_field_result b;
};

// Fields "a" and "b" in a column, "a" at the origin, with no pointer.
static struct two_fields build_two_fields(voe_ui_context *ui,
					  voe_base_arena *arena,
					  voe_ui_pointer pointer,
					  voe_ui_keyboard keyboard)
{
	struct two_fields r;
	voe_ui_node a;
	voe_ui_node b;

	voe_ui_frame_begin(ui, arena);
	voe_ui_pointer_set(ui, pointer);
	voe_ui_keyboard_set(ui, keyboard);
	voe_ui_column_begin(ui, (voe_ui_container){ 0 });
	a = voe_ui_field(ui, "a", 0, "one", field_sizing());
	b = voe_ui_field(ui, "b", 0, "two", field_sizing());
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));
	r.a = voe_ui_field_action(ui, a);
	r.b = voe_ui_field_action(ui, b);
	return r;
}

// Tab from the first commits it and focuses the second; Tab from the second
// wraps to the first.
static void tab_moves_the_focus_to_the_next_field_and_wraps(
	voe_ui_context *ui, voe_base_arena *arena)
{
	voe_ui_pointer press = { .at = IN_FIELD, .over = true, .down = true };
	voe_ui_pointer none = { 0 };
	voe_ui_keyboard type_x = { .text = "x", .size = 1 };
	voe_ui_keyboard tab = { .tab = true };
	struct two_fields r;

	(void)build_two_fields(ui, arena, none, NO_KEYS);
	r = build_two_fields(ui, arena, press, NO_KEYS);
	VOE_TEST_CHECK(r.a.focused);
	(void)build_two_fields(ui, arena, none, type_x);

	r = build_two_fields(ui, arena, none, tab);
	VOE_TEST_CHECK(r.a.committed);
	VOE_TEST_CHECK(strcmp(r.a.text, "x") == 0);
	VOE_TEST_CHECK(!r.a.focused);
	VOE_TEST_CHECK(r.b.focused);

	// The second opens the next frame, its whole text selected.
	r = build_two_fields(ui, arena, none, type_x);
	VOE_TEST_CHECK(r.b.focused);
	VOE_TEST_CHECK(strcmp(r.b.text, "x") == 0);

	r = build_two_fields(ui, arena, none, tab);
	VOE_TEST_CHECK(r.b.committed);
	VOE_TEST_CHECK(r.a.focused);
	VOE_TEST_CHECK(!r.b.focused);
}

// A 0x09 or 0x7F byte in `text` is not appended; the bytes around it are.
static void control_bytes_are_not_appended(voe_ui_context *ui,
					   voe_base_arena *arena)
{
	voe_ui_keyboard typed = { .text = "a\tb\x7f" "c", .size = 5 };
	voe_ui_keyboard only_tab = { .text = "\t", .size = 1 };
	struct field_frame f;
	voe_ui_field_result r;

	// A control byte alone does not replace the selection either.
	focus_field(ui, arena, "hi");
	f = build_field(ui, arena, IN_FIELD, true, false, "hi", only_tab);
	r = voe_ui_field_action(ui, f.f);
	VOE_TEST_CHECK(!r.changed);
	VOE_TEST_CHECK(strcmp(r.text, "hi") == 0);

	f = build_field(ui, arena, IN_FIELD, true, false, "hi", typed);
	r = voe_ui_field_action(ui, f.f);
	VOE_TEST_CHECK(strcmp(r.text, "abc") == 0);
}

// voe_ui_typing is true exactly while a field held the focus at the end of
// the last frame.
static void typing_follows_the_focus(voe_ui_context *ui,
				     voe_base_arena *arena)
{
	voe_ui_keyboard press_enter = { .enter = true };

	(void)build_field(ui, arena, OUTSIDE_FIELD, true, false, "hi", NO_KEYS);
	(void)build_field(ui, arena, OUTSIDE_FIELD, true, true, "hi", NO_KEYS);
	VOE_TEST_CHECK(!voe_ui_typing(ui));
	(void)build_field(ui, arena, IN_FIELD, true, false, "hi", NO_KEYS);
	(void)build_field(ui, arena, IN_FIELD, true, true, "hi", NO_KEYS);
	VOE_TEST_CHECK(voe_ui_typing(ui));
	(void)build_field(ui, arena, IN_FIELD, true, false, "hi", press_enter);
	VOE_TEST_CHECK(!voe_ui_typing(ui));
}

// ------------------------------------------------ typing into a number box

// Two number boxes, "n" handed START over "m" handed M_START, each holding
// the box every number case above holds. "n" is at the origin, so the point
// ON_N is inside it whether it is open or not — open, it only grows. A click
// is all these cases do with the pointer; the rest is the keyboard. Open, a
// number box composes a label, which is why these run in the device group.
#define M_START 7.0
#define ON_N ((voe_math_float2){ 10.0f, ON_NUMBER_Y })

struct numbers_frame {
	voe_ui_node n;
	voe_ui_node m;
};

static struct numbers_frame build_numbers(voe_ui_context *ui,
					  voe_base_arena *arena,
					  voe_math_float2 at, bool down,
					  voe_ui_keyboard keyboard)
{
	struct numbers_frame f;

	voe_ui_frame_begin(ui, arena);
	voe_ui_pointer_set(ui, (voe_ui_pointer){ .at = at,
						 .over = true,
						 .down = down });
	voe_ui_keyboard_set(ui, keyboard);

	voe_ui_panel_begin(ui, "panel", 0, VOE_UI_SURFACE_SURFACE,
			   (voe_ui_container){ 0 });
	f.n = voe_ui_number_begin(ui, "n", 0, START, PER_MM);
	voe_ui_box(ui, (voe_math_float2){ BOX_WIDE, BOX_HIGH },
		   (voe_ui_sizing){ 0 });
	voe_ui_end(ui);
	f.m = voe_ui_number_begin(ui, "m", 0, M_START, PER_MM);
	voe_ui_box(ui, (voe_math_float2){ BOX_WIDE, BOX_HIGH },
		   (voe_ui_sizing){ 0 });
	voe_ui_end(ui);
	voe_ui_end(ui);

	VOE_TEST_CHECK(voe_ui_frame_end(ui));
	return f;
}

// A press and a release far away, which closes whatever an earlier case left
// open, then a click on "n" — which opens it.
static void open_n(voe_ui_context *ui, voe_base_arena *arena)
{
	(void)build_numbers(ui, arena, OUTSIDE_FIELD, true, NO_KEYS);
	(void)build_numbers(ui, arena, OUTSIDE_FIELD, false, NO_KEYS);
	(void)build_numbers(ui, arena, ON_N, true, NO_KEYS);
	(void)build_numbers(ui, arena, ON_N, false, NO_KEYS);
}

static voe_ui_keyboard typing(const char *text)
{
	return (voe_ui_keyboard){ .text = text,
				  .size = (uint32_t)strlen(text) };
}

// A CLICK OPENS IT AND A DRAG NEVER DOES. Clicked, it is typing on the frame
// after the release, draws open with its caller's content hidden, and Escape
// closes it; pressed and dragged 5 mm, nothing on any frame opens it.
static void a_click_opens_the_box_and_a_drag_never_does(
	voe_ui_context *ui, voe_base_arena *arena)
{
	voe_ui_keyboard escape = { .escape = true };
	struct numbers_frame f;
	voe_ui_number_result r;

	open_n(ui, arena);
	f = build_numbers(ui, arena, ON_N, false, NO_KEYS);
	r = voe_ui_number_action(ui, f.n);
	VOE_TEST_CHECK(r.typing);
	VOE_TEST_CHECK(!r.changed);
	VOE_TEST_CHECK_FLOAT((float)r.value, (float)START, 0.001f);
	VOE_TEST_CHECK(voe_ui_typing(ui));
	VOE_TEST_CHECK(!voe_ui_number_action(ui, f.m).typing);

	f = build_numbers(ui, arena, ON_N, false, escape);
	r = voe_ui_number_action(ui, f.n);
	VOE_TEST_CHECK(!r.typing);
	VOE_TEST_CHECK(!r.changed);
	VOE_TEST_CHECK(!voe_ui_typing(ui));

	(void)build_numbers(ui, arena, ON_N, true, NO_KEYS);
	(void)build_numbers(ui, arena, (voe_math_float2){ 15.0f, ON_NUMBER_Y },
			    true, NO_KEYS);
	f = build_numbers(ui, arena, (voe_math_float2){ 15.0f, ON_NUMBER_Y },
			  false, NO_KEYS);
	VOE_TEST_CHECK(!voe_ui_number_action(ui, f.n).typing);
	f = build_numbers(ui, arena, (voe_math_float2){ 15.0f, ON_NUMBER_Y },
			  false, NO_KEYS);
	VOE_TEST_CHECK(!voe_ui_number_action(ui, f.n).typing);
}

// Typed, then Enter: the typed number comes back as `value` with `changed`,
// on the Enter frame and not on the typing one, and the box closes.
static void typing_a_number_then_enter_changes_the_value(
	voe_ui_context *ui, voe_base_arena *arena)
{
	voe_ui_keyboard enter = { .enter = true };
	struct numbers_frame f;
	voe_ui_number_result r;

	open_n(ui, arena);
	f = build_numbers(ui, arena, ON_N, false, typing("0.1"));
	r = voe_ui_number_action(ui, f.n);
	VOE_TEST_CHECK(r.typing);
	VOE_TEST_CHECK(!r.changed);

	f = build_numbers(ui, arena, ON_N, false, enter);
	r = voe_ui_number_action(ui, f.n);
	VOE_TEST_CHECK(r.changed);
	VOE_TEST_CHECK(r.value == 0.1);
	VOE_TEST_CHECK(!r.typing);
	VOE_TEST_CHECK(!r.refused);

	f = build_numbers(ui, arena, ON_N, false, NO_KEYS);
	VOE_TEST_CHECK(!voe_ui_number_action(ui, f.n).changed);
}

// Opened and entered with nothing typed: the text is what it opened with, so
// nothing changes — not even the value going through `%.6g` and back.
static void enter_with_nothing_typed_changes_nothing(voe_ui_context *ui,
						     voe_base_arena *arena)
{
	voe_ui_keyboard enter = { .enter = true };
	struct numbers_frame f;
	voe_ui_number_result r;

	open_n(ui, arena);
	f = build_numbers(ui, arena, ON_N, false, enter);
	r = voe_ui_number_action(ui, f.n);
	VOE_TEST_CHECK(!r.changed);
	VOE_TEST_CHECK(!r.refused);
	VOE_TEST_CHECK(!r.typing);
	VOE_TEST_CHECK_FLOAT((float)r.value, (float)START, 0.001f);
}

// Not a number, then Enter: refused, nothing changed, still open — and the
// next frame says so with one more label's worth of letters. Escape then
// closes it with the caller's value standing.
static void a_refused_enter_stays_open_and_escape_closes(
	voe_ui_context *ui, voe_base_arena *arena)
{
	voe_ui_keyboard enter = { .enter = true };
	voe_ui_keyboard escape = { .escape = true };
	struct numbers_frame f;
	voe_ui_number_result r;
	uint32_t before;

	open_n(ui, arena);
	build_numbers(ui, arena, ON_N, false, typing("abc"));
	before = voe_ui_element_count(ui);
	f = build_numbers(ui, arena, ON_N, false, enter);
	r = voe_ui_number_action(ui, f.n);
	VOE_TEST_CHECK(r.refused);
	VOE_TEST_CHECK(!r.changed);
	VOE_TEST_CHECK(r.typing);
	VOE_TEST_CHECK(voe_ui_typing(ui));

	f = build_numbers(ui, arena, ON_N, false, NO_KEYS);
	r = voe_ui_number_action(ui, f.n);
	VOE_TEST_CHECK(r.typing);
	VOE_TEST_CHECK(!r.refused);
	VOE_TEST_CHECK(voe_ui_element_count(ui) > before);

	f = build_numbers(ui, arena, ON_N, false, escape);
	r = voe_ui_number_action(ui, f.n);
	VOE_TEST_CHECK(!r.changed);
	VOE_TEST_CHECK(!r.typing);
	VOE_TEST_CHECK_FLOAT((float)r.value, (float)START, 0.001f);
}

// `3` then Tab: "n" takes 3 and the focus goes to "m", which opens; Escape
// there leaves "m"'s value as it was handed in.
static void tab_commits_and_opens_the_next_number_box(voe_ui_context *ui,
						      voe_base_arena *arena)
{
	voe_ui_keyboard tab = { .text = "3", .size = 1, .tab = true };
	voe_ui_keyboard escape = { .escape = true };
	struct numbers_frame f;
	voe_ui_number_result r;

	open_n(ui, arena);
	f = build_numbers(ui, arena, ON_N, false, tab);
	r = voe_ui_number_action(ui, f.n);
	VOE_TEST_CHECK(r.changed);
	VOE_TEST_CHECK(r.value == 3.0);
	VOE_TEST_CHECK(!r.typing);
	VOE_TEST_CHECK(voe_ui_number_action(ui, f.m).typing);

	f = build_numbers(ui, arena, ON_N, false, NO_KEYS);
	VOE_TEST_CHECK(voe_ui_number_action(ui, f.m).typing);

	f = build_numbers(ui, arena, ON_N, false, escape);
	r = voe_ui_number_action(ui, f.m);
	VOE_TEST_CHECK(!r.changed);
	VOE_TEST_CHECK(!r.typing);
	VOE_TEST_CHECK_FLOAT((float)r.value, (float)M_START, 0.001f);
}

// Opens "n", types `text` and presses Enter, and hands back that frame's
// answer; a box left open by a refusal is closed by the next open_n.
static voe_ui_number_result enter_typed(voe_ui_context *ui,
					voe_base_arena *arena,
					const char *text)
{
	voe_ui_keyboard enter = typing(text);
	struct numbers_frame f;

	enter.enter = true;
	open_n(ui, arena);
	f = build_numbers(ui, arena, ON_N, false, enter);
	return voe_ui_number_action(ui, f.n);
}

// Blanks around a number are allowed; an infinity, a NaN and trailing
// letters are not one finite number, and are refused.
static void what_counts_as_a_number(voe_ui_context *ui, voe_base_arena *arena)
{
	voe_ui_number_result r = enter_typed(ui, arena, " 2.5 ");

	VOE_TEST_CHECK(r.changed);
	VOE_TEST_CHECK(!r.refused);
	VOE_TEST_CHECK(r.value == 2.5);

	r = enter_typed(ui, arena, "1e999");
	VOE_TEST_CHECK(r.refused);
	VOE_TEST_CHECK(!r.changed);
	r = enter_typed(ui, arena, "nan");
	VOE_TEST_CHECK(r.refused);
	VOE_TEST_CHECK(!r.changed);
	r = enter_typed(ui, arena, "2x");
	VOE_TEST_CHECK(r.refused);
	VOE_TEST_CHECK(!r.changed);

	// And a press elsewhere closes a refused box, changing nothing.
	r = voe_ui_number_action(
		ui, build_numbers(ui, arena, OUTSIDE_FIELD, true, NO_KEYS).n);
	VOE_TEST_CHECK(!r.typing);
	VOE_TEST_CHECK(!r.changed);
}

// The device and the font this section needs, and the skip that stands in for
// them where there is no driver.
static int the_field(voe_base_arena *arena)
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

	device = voe_render_device_new_headless(arena, size, capacities,
						&error);
	if (device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED) {
			printf("skip: no graphics driver — the field checks "
			       "did not run\n");
			return 0;
		}
		VOE_TEST_CHECK(device != NULL);
		return 0;
	}

	// Oxanium: nothing here cares which face, so the engine's default.
	font = voe_text_font_new(VOE_TEXT_TYPEFACE_OXANIUM, device, arena,
				 &error);
	if (font == NULL) {
		VOE_TEST_CHECK(font != NULL);
		voe_render_device_destroy(device);
		return 0;
	}

	// Elements enough for the capacity case's 256 glyphs plus a few
	// backgrounds and carets; nodes are two per field plus a panel.
	ui = voe_ui_context_new(arena, (voe_ui_capacities){ .nodes = 32,
							    .elements = 512 });
	voe_ui_font_set(ui, font);
	voe_ui_theme_set(ui, &TEST_THEME);

	a_press_focuses_and_a_press_elsewhere_unfocuses(ui, arena);
	typed_bytes_are_appended_in_order(ui, arena);
	two_fields_only_the_focused_one_changes(ui, arena);
	backspace_takes_a_two_byte_code_point_whole(ui, arena);
	backspace_on_an_empty_text_does_nothing(ui, arena);
	a_text_at_capacity_refuses_the_next_code_point_whole(ui, arena);
	entered_is_true_only_on_the_frame_enter_arrived(ui, arena);
	a_field_not_called_loses_focus(ui, arena);
	voe_ui_field_focus_takes_it(ui, arena);
	unfocused_hands_back_the_callers_own_pointer(ui, arena);
	typing_into_a_newly_focused_field_replaces_its_text(ui, arena);
	backspace_on_a_newly_focused_field_empties_it(ui, arena);
	escape_cancels_and_the_callers_text_stands(ui, arena);
	enter_and_a_press_elsewhere_commit_the_typed_text(ui, arena);
	tab_moves_the_focus_to_the_next_field_and_wraps(ui, arena);
	control_bytes_are_not_appended(ui, arena);
	typing_follows_the_focus(ui, arena);
	movement_inside_the_dead_zone_changes_nothing(ui, arena);
	a_click_opens_the_box_and_a_drag_never_does(ui, arena);
	typing_a_number_then_enter_changes_the_value(ui, arena);
	enter_with_nothing_typed_changes_nothing(ui, arena);
	a_refused_enter_stays_open_and_escape_closes(ui, arena);
	tab_commits_and_opens_the_next_number_box(ui, arena);
	what_counts_as_a_number(ui, arena);

	voe_text_font_destroy(font);
	voe_render_device_destroy(device);
	return 0;
}

// The device and the font this one case needs, and the skip that stands in for
// them where there is no driver.
static int the_theme_size(voe_base_arena *arena)
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

	// Oxanium: nothing here cares which face, so the engine's default.
	font = voe_text_font_new(VOE_TEXT_TYPEFACE_OXANIUM, device, arena,
				 &error);
	if (font == NULL) {
		VOE_TEST_CHECK(font != NULL);
		voe_render_device_destroy(device);
		return 0;
	}

	ui = voe_ui_context_new(arena, (voe_ui_capacities){ .nodes = 64,
							    .elements = 64 });
	voe_ui_font_set(ui, font);
	voe_ui_theme_set(ui, &TEST_THEME);

	a_bigger_theme_grows_the_row_and_not_the_gaps(ui, arena);
	a_label_emits_its_letters_after_the_panel(ui, arena);
	a_wholly_clipped_label_emits_nothing(ui, arena);
	a_label_in_accent_draws_the_accent(ui, arena);

	voe_text_font_destroy(font);
	voe_render_device_destroy(device);
	return 0;
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
	a_duplicate_key_refuses_the_frame(ui, arena);
	the_same_name_under_two_panels_is_two_widgets(ui, arena);
	the_number_box_is_where_the_tests_think_it_is(ui, arena);
	dragging_sideways_moves_the_value(ui, arena);
	a_fine_drag_moves_a_tenth_as_far(ui, arena);
	a_drag_past_the_edge_keeps_working(ui, arena);
	arriving_with_the_button_already_down_arms_no_number(ui, arena);
	a_number_box_that_stops_being_called_is_let_go(ui, arena);
	a_number_and_a_button_sharing_a_key_refuse_the_frame(ui, arena);
	a_known_tree_emits_a_known_list(ui, arena);
	a_transparent_panel_emits_nothing(ui, arena);
	the_nearest_theme_wins_and_a_pop_restores_it(ui, arena);
	an_unbalanced_push_refuses_the_frame(ui, arena);
	two_images_are_two_records_in_call_order(ui, arena);
	a_half_clipped_button_emits_its_visible_half(ui, arena);
	a_pointer_over_the_clipped_half_hovers_nothing(ui, arena);
	a_number_box_scrolled_away_mid_drag_keeps_dragging(ui, arena);
	too_many_elements_refuses_the_frame(arena);
	a_scroll_moves_the_next_frames_content(arena);
	an_area_skipped_for_a_frame_forgets(arena);
	a_nested_area_passes_on_what_it_cannot_take(arena);
	content_that_fits_has_no_bar(arena);
	the_thumb_is_as_long_and_as_far_as_the_offset_says(arena);
	dragging_the_thumb_moves_the_offset(arena);
	pressing_the_track_pages_once(arena);
	a_thumb_hides_the_button_beneath_it(arena);
	too_many_scroll_areas_refuses_the_frame(arena);

	(void)the_theme_size(arena);
	(void)the_field(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
