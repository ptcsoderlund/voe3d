// The button, and what is worth pinning down about it: what a press and a
// release do in every awkward order a hand can produce, and that a clip decides
// both what is drawn and what can be hit. What a drag on a number box comes to
// is `ui/tests/number.c`. What every widget shares — keys, themes, panels,
// labels and the known list — is `ui/tests/widgets.c`.
//
// EVERY CLICK CASE NEEDS NO GRAPHICS CARD AND NO WINDOW SYSTEM, which is the
// property the whole design is arranged around: the pointer is a value handed
// in (ADR-0093), so a press is two calls in a row and not a mouse. A button is
// composed rather than given a string, so none of them touches a font, opens a
// window, asks a compositor for anything or names `platform`.
//
// STATE IS DRAWN INVERTED (ADR-0196), and the cases at the end of this file are
// where that is pinned down: a held button, a number box being dragged and a
// selected choice each draw a fill of `inverse` with the label on it in
// `inverse_ink`, while merely hovered stays the control_hovered step it was.
// Those cases put a label on a control and so measure a string, which needs a
// real font and so a device — a headless one, with no window and no compositor,
// and a skip naming what did not run where there is no driver at all
// (ADR-0106). The last case pins that a button's pad follows the theme's
// spacing (ADR-0344).
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
// A CONTAINER MAY TAKE THE POINTER, and the cases in the middle of this file
// are what that comes to: nothing painted under a blocker is hovered or fired
// through it, not even where the blocker holds nothing but padding, while the
// button the blocker itself holds is hit as it always was and a blocker clipped
// out of sight stops nothing (ADR-0199).
//
// A CLIP IS WHAT CAN BE SEEN AND SO WHAT CAN BE HIT, since spec 001. A button
// half clipped emits a record clipped to its visible half and answers the pointer
// only there; one wholly clipped emits nothing. What a drag makes of the same
// rule — a number box scrolled out of sight mid-drag goes on reporting it — is
// `ui/tests/number.c`. A label under a clip is the rule again and needs a font,
// so it is in `ui/tests/widgets.c`.
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
	// The button a blocker holds, and VOE_UI_NODE_NONE in every tree
	// below that has no blocker in it.
	voe_ui_node inner;
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
	struct frame f = { VOE_UI_NODE_NONE, VOE_UI_NODE_NONE,
			   VOE_UI_NODE_NONE, false };

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

// A row 12.5 wide clipping X, holding two buttons 25 by 15: "half" at the origin,
// of which the left 12.5 is seen, and "gone" at 25, of which nothing is. The row
// is no panel and emits nothing, so every record here is a button's.
#define HALF_WIDE 12.5f

static struct frame build_clipped(voe_ui_context *ui, voe_base_arena *arena,
				  voe_math_float2 at, bool down)
{
	struct frame f = { VOE_UI_NODE_NONE, VOE_UI_NODE_NONE,
			   VOE_UI_NODE_NONE, false };

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

// ---- A CONTAINER THAT TAKES THE POINTER ----

// The two buttons of `build` again, in the same column, with a blocker
// anchored over the lower one:
//
//   panel      pad 4, gap 3, at the origin
//     button "a"    25 x 15 at (4, 4)
//     button "b"    25 x 15 at (4, 22)
//     blocker       anchored, blocks_pointer, FIXED 25 x 15 at (4, 22),
//                   pad 14 on the left and 2.5 on the other three
//       button "in"   pad 2.5 round a 3 x 3 box  ->  8 x 8 at (18, 24.5)
//
// The blocker covers "b" exactly. Its wide left padding is what makes IN_B —
// a point inside the covered button — land inside the blocker and on nothing
// the blocker holds, which is the gap a cursor must not fall through.
#define BLOCKER_LEFT 14.0f
#define INNER_BOX 3.0f

// A point on the blocker's own button, which is painted after it.
#define IN_INNER ((voe_math_float2){ 22.0f, 28.0f })

// One frame of that tree. `clipped` wraps the blocker in an anchored container
// of no size at all that clips both axes, so the blocker keeps the rectangle
// it had and loses every millimetre of the part that can be seen.
static struct frame build_blocked(voe_ui_context *ui, voe_base_arena *arena,
				  voe_math_float2 at, bool down, bool clipped)
{
	struct frame f = { VOE_UI_NODE_NONE, VOE_UI_NODE_NONE,
			   VOE_UI_NODE_NONE, false };
	voe_ui_anchor over_b = { .anchored = true,
				 .y = { VOE_UI_ACROSS_START, B_Y - 4.0f } };
	voe_ui_anchor corner = { .anchored = true };

	voe_ui_frame_begin(ui, arena);
	voe_ui_pointer_set(ui, (voe_ui_pointer){ .at = at,
						 .over = true,
						 .down = down });

	voe_ui_panel_begin(ui, "panel", 0, VOE_UI_SURFACE_SURFACE,
			   (voe_ui_container){ .gap = 3.0f,
					       .pad = pad_all(4.0f) });
	f.a = voe_ui_button_begin(ui, "a", 0);
	voe_ui_box(ui, (voe_math_float2){ BOX_WIDE, BOX_HIGH },
		   (voe_ui_sizing){ 0 });
	voe_ui_end(ui);
	f.b = voe_ui_button_begin(ui, "b", 0);
	voe_ui_box(ui, (voe_math_float2){ BOX_WIDE, BOX_HIGH },
		   (voe_ui_sizing){ 0 });
	voe_ui_end(ui);

	if (clipped)
		voe_ui_row_begin(ui, (voe_ui_container){
					     .size = { { VOE_UI_SIZE_FIXED,
							 0.0f },
						       { VOE_UI_SIZE_FIXED,
							 0.0f } },
					     .anchor = over_b,
					     .overflow = { VOE_UI_OVERFLOW_CLIP,
							   VOE_UI_OVERFLOW_CLIP } });

	voe_ui_row_begin(ui, (voe_ui_container){
				     .size = { { VOE_UI_SIZE_FIXED,
						 BUTTON_WIDE },
					       { VOE_UI_SIZE_FIXED,
						 BUTTON_HIGH } },
				     .pad = { BLOCKER_LEFT, 2.5f, 2.5f, 2.5f },
				     .anchor = clipped ? corner : over_b,
				     .blocks_pointer = true });
	f.inner = voe_ui_button_begin(ui, "in", 0);
	voe_ui_box(ui, (voe_math_float2){ INNER_BOX, INNER_BOX },
		   (voe_ui_sizing){ 0 });
	voe_ui_end(ui);
	voe_ui_end(ui);

	if (clipped)
		voe_ui_end(ui);
	voe_ui_end(ui);

	f.ok = voe_ui_frame_end(ui);
	return f;
}

// THE GAP IS THE BLOCKER'S. The pointer is inside the covered button and
// inside the blocker's padding, on none of its children: it hovers neither,
// and a press and a release there fire nothing.
static void a_pointer_in_a_blockers_gap_hovers_nothing(voe_ui_context *ui,
						       voe_base_arena *arena)
{
	struct frame f = build_blocked(ui, arena, IN_B, false, false);

	VOE_TEST_CHECK(f.ok);
	VOE_TEST_CHECK(!voe_ui_button_action(ui, f.b).hovered);
	VOE_TEST_CHECK(!voe_ui_button_action(ui, f.inner).hovered);

	f = build_blocked(ui, arena, IN_B, true, false);
	VOE_TEST_CHECK(!voe_ui_button_action(ui, f.b).held);

	f = build_blocked(ui, arena, IN_B, false, false);
	VOE_TEST_CHECK(!voe_ui_button_action(ui, f.b).fired);
}

// And the blocker's own button wins the pointer back, being painted after it.
static void a_button_inside_a_blocker_is_still_hit(voe_ui_context *ui,
						   voe_base_arena *arena)
{
	struct frame f = build_blocked(ui, arena, IN_INNER, false, false);

	VOE_TEST_CHECK(voe_ui_button_action(ui, f.inner).hovered);

	f = build_blocked(ui, arena, IN_INNER, true, false);
	VOE_TEST_CHECK(voe_ui_button_action(ui, f.inner).held);

	f = build_blocked(ui, arena, IN_INNER, false, false);
	VOE_TEST_CHECK(voe_ui_button_action(ui, f.inner).fired);
}

// A blocker clears what is under IT and nothing else: the upper button is
// outside its rectangle and answers the pointer as it does in a tree with no
// blocker in it at all.
static void a_pointer_beside_a_blocker_hovers_what_it_is_over(
	voe_ui_context *ui, voe_base_arena *arena)
{
	struct frame f = build_blocked(ui, arena, IN_A, false, false);

	VOE_TEST_CHECK(voe_ui_button_action(ui, f.a).hovered);

	f = build_blocked(ui, arena, IN_A, true, false);
	VOE_TEST_CHECK(voe_ui_button_action(ui, f.a).held);

	f = build_blocked(ui, arena, IN_A, false, false);
	VOE_TEST_CHECK(voe_ui_button_action(ui, f.a).fired);
}

// IT IS THE VISIBLE RECTANGLE THAT BLOCKS, the same rule that decides what can
// be hit at all: the same tree with the blocker clipped away stops nothing, so
// the covered button is hovered and fired again.
static void a_blocker_clipped_away_blocks_nothing(voe_ui_context *ui,
						  voe_base_arena *arena)
{
	struct frame f = build_blocked(ui, arena, IN_B, false, true);

	VOE_TEST_CHECK(f.ok);
	VOE_TEST_CHECK(voe_ui_button_action(ui, f.b).hovered);

	f = build_blocked(ui, arena, IN_B, true, true);
	VOE_TEST_CHECK(voe_ui_button_action(ui, f.b).held);

	f = build_blocked(ui, arena, IN_B, false, true);
	VOE_TEST_CHECK(voe_ui_button_action(ui, f.b).fired);
}

// ---- STATE DRAWN INVERTED, WHICH NEEDS A FONT ----

// The three controls the cases below are built from, each round the same
// one-letter label.
enum control { CONTROL_BUTTON, CONTROL_NUMBER, CONTROL_CHOICE };

// The value and the step the number box below is built with. Nothing here reads
// either: this block is about what an inverted control draws. What a drag makes
// of them is `ui/tests/number.c`.
#define START 100.0
#define PER_MM 2.0

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

// THE THEME'S SPACING SCALES A BUTTON'S PAD (ADR-0344): the same labelled
// button under a copy of the theme with spacing 0.5 has 1.25 of pad a side
// rather than 2.5, so it is 2.5 narrower and 2.5 shorter.
static void button_pad_follows_spacing(voe_ui_context *ui,
				       voe_base_arena *arena)
{
	voe_ui_theme tight = TEST_THEME;
	voe_ui_rect wide;
	voe_ui_rect narrow;
	voe_ui_node b;

	tight.spacing = 0.5f;

	b = build_labelled(ui, arena, CONTROL_BUTTON, false, 1.0f, false,
			   false);
	wide = voe_ui_node_rect(ui, b);

	voe_ui_theme_set(ui, &tight);
	b = build_labelled(ui, arena, CONTROL_BUTTON, false, 1.0f, false,
			   false);
	narrow = voe_ui_node_rect(ui, b);
	voe_ui_theme_set(ui, &TEST_THEME);

	VOE_TEST_CHECK_FLOAT(wide.size.x - narrow.size.x, 2.5f, 0.001f);
	VOE_TEST_CHECK_FLOAT(wide.size.y - narrow.size.y, 2.5f, 0.001f);
}

// The device and the font the four cases above need, and the skip that stands
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
			       "inversion checks and the spacing check did "
			       "not run\n");
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
	button_pad_follows_spacing(ui, arena);

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
	a_half_clipped_button_emits_its_visible_half(ui, arena);
	a_pointer_over_the_clipped_half_hovers_nothing(ui, arena);
	a_pointer_in_a_blockers_gap_hovers_nothing(ui, arena);
	a_button_inside_a_blocker_is_still_hit(ui, arena);
	a_pointer_beside_a_blocker_hovers_what_it_is_over(ui, arena);
	a_blocker_clipped_away_blocks_nothing(ui, arena);

	the_inverted_states(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
