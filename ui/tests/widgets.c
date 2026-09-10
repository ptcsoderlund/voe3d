// The widgets, and the two things about them that are worth pinning down: what
// a press and a release do in every awkward order a hand can produce, and that
// a known tree comes out as a known list of records.
//
// EVERY CASE HERE EXCEPT THE LAST NEEDS NO GRAPHICS CARD AND NO WINDOW SYSTEM,
// which is the property the whole design is arranged around: the pointer is a
// value handed in (ADR-0093), so a drag is three calls in a row and not a mouse.
// A button is composed rather than given a string, so nothing in the click cases
// touches a font. Nothing here opens a window, asks a compositor for anything or
// names `platform`.
//
// THE LAST CASE IS THE EXCEPTION AND IT IS DELIBERATE (ADR-0106). The text scale
// multiplies a MEASUREMENT, so proving it needs a real font, and a font uploads
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
			printf("skip: no graphics driver — the text-size and "
			       "label-emission checks did not run\n");
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
	a_known_tree_emits_a_known_list(ui, arena);
	a_transparent_panel_emits_nothing(ui, arena);
	too_many_elements_refuses_the_frame(arena);

	(void)the_text_scale(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
