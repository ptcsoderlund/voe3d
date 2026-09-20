// What every widget shares, and the things about it worth pinning down: that a
// key is a path and two widgets given the same one are caught rather than
// silently joined, that a known tree comes out as a known list of records —
// two images in a row among them, each one IMAGE record carrying the texture
// index and sheet it was given — and that a widget draws with the nearest theme
// in force. The button's and the number box's gestures are `ui/tests/button.c`,
// the field's `ui/tests/field.c` and the scroll area's `ui/tests/scroll.c`.
//
// THE COLLISION CASE IS THE MOST VALUABLE ONE IN THIS FILE. Two widgets given
// the same key share state silently: the second lights up when the first is
// hovered and nothing says why. Here it is a refused frame and a line on stderr,
// and this case is what keeps it one.
//
// A THEME IS THE NEAREST ONE IN FORCE, and a push that is never popped refuses
// the frame rather than leaving the rest of the tree drawing with the wrong
// one — as does a tree with more records in it than the context has room for.
//
// A CASE THAT MEASURES A STRING IS THE EXCEPTION AND IT IS DELIBERATE
// (ADR-0106). The text scale multiplies a MEASUREMENT and a label emits one
// record per letter, so proving either needs a real font, and a font uploads an
// atlas and so needs a device. Those cases take a headless one — no window, no
// surface, no compositor — and where there is no driver at all they skip,
// saying which check did not run rather than only why. Everything else here
// needs no graphics card and no window system: the pointer is a value handed in
// (ADR-0093), and nothing opens a window, asks a compositor for anything or
// names `platform`.
//
// A CLIP IS WHAT CAN BE SEEN AND SO WHAT CAN BE DRAWN, since spec 001. A label
// wholly clipped emits nothing. A button and a number box under a clip are the
// same rule against the pointer as well, and they are in `ui/tests/button.c`.
//
// THE GEOMETRY IS WORKED OUT BY HAND AND WRITTEN AS NUMBERS. A widget's
// rectangle comes out of layout, so a test that asked layout where a widget was
// and then checked the record there would pass with the arithmetic inverted.
// The numbers below say where the widgets are meant to be.
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

// A point inside button "a".
#define IN_A ((voe_math_float2){ 16.0f, 10.0f })

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

// A number box is made with a value and a rate per millimetre of drag. Nothing
// here drags one — that is `ui/tests/button.c` — so these two are only what the
// one number box below is handed.
#define PER_MM 2.0
#define START 100.0

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

	a_duplicate_key_refuses_the_frame(ui, arena);
	the_same_name_under_two_panels_is_two_widgets(ui, arena);
	a_number_and_a_button_sharing_a_key_refuse_the_frame(ui, arena);
	a_known_tree_emits_a_known_list(ui, arena);
	a_transparent_panel_emits_nothing(ui, arena);
	the_nearest_theme_wins_and_a_pop_restores_it(ui, arena);
	an_unbalanced_push_refuses_the_frame(ui, arena);
	too_many_elements_refuses_the_frame(arena);
	two_images_are_two_records_in_call_order(ui, arena);

	(void)the_theme_size(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
