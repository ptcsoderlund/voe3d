// The layout arithmetic, as rectangles. Every case here is a number worked out
// by hand and written down, because a layout test that recomputes the layout to
// check it proves only that the code agrees with itself.
//
// FOUR THINGS THIS FILE IS PINNING DOWN RATHER THAN MERELY EXERCISING, each one
// a decision that could reasonably have gone the other way:
//
//   - The space runs DOWN from the top-left corner, so a column's first child
//     has the smallest Y and nothing is subtracted. The way to break that is to
//     reintroduce a flip, out of habit or because the engine's world is Y-up, so
//     the column is laid out with three children of DIFFERENT heights: a flip
//     would put them back in the same order at different offsets, and a
//     symmetrical case would not notice.
//   - A grow child contributes nothing along the flow to a natural container's
//     size, so a spacer in a panel that fits its children collapses. The same
//     tree in a fixed container is checked beside it, where the spacer is what
//     takes up the slack.
//   - A child's own fixed size across the flow beats the container's FILL.
//   - Overflow is not shrunk. The true rectangles come back, including one whose
//     corner is negative because it is centred on a container smaller than it.
//
// AND TWO THAT ARE ABOUT THE MACHINERY RATHER THAN THE ARITHMETIC: a frame that
// wants more nodes than the context has is refused and the next frame is fine,
// and the same tree built twice on one context gives the same answer, which is
// what "nothing is kept between frames" means when it is a claim with evidence.
//
// Needs no graphics card and no window system: this folder draws nothing and
// names nothing that talks to a machine.
#include <ui/layout.h>

#include <base/arena.h>
#include <testing/test.h>

// Millimetres, at panel scale, and every expected number below is exact in
// binary but for the halves — so this is slack for the arithmetic and not for
// the answer.
#define TOLERANCE 1e-5f

// A macro and not a function so that a failure names the line of the case that
// failed rather than the line of the checker.
#define CHECK_RECT(rect, min_x, min_y, size_x, size_y)                         \
	do {                                                                   \
		voe_ui_rect checked = (rect);                                  \
									       \
		VOE_TEST_CHECK_FLOAT(checked.min.x, (min_x), TOLERANCE);       \
		VOE_TEST_CHECK_FLOAT(checked.min.y, (min_y), TOLERANCE);       \
		VOE_TEST_CHECK_FLOAT(checked.size.x, (size_x), TOLERANCE);     \
		VOE_TEST_CHECK_FLOAT(checked.size.y, (size_y), TOLERANCE);     \
	} while (0)

static voe_ui_size natural(void)
{
	return (voe_ui_size){ VOE_UI_SIZE_NATURAL, 0.0f };
}

static voe_ui_size fixed(float mm)
{
	return (voe_ui_size){ VOE_UI_SIZE_FIXED, mm };
}

static voe_ui_size grow(float weight)
{
	return (voe_ui_size){ VOE_UI_SIZE_GROW, weight };
}

static voe_ui_sizing sizing(voe_ui_size along, voe_ui_size across)
{
	return (voe_ui_sizing){ .along = along, .across = across };
}

// A box that declares its size on both axes, so its content is never read.
static voe_ui_node fixed_box(voe_ui_context *ui, float along, float across)
{
	return voe_ui_box(ui, (voe_math_float2){ 0.0f, 0.0f },
			  sizing(fixed(along), fixed(across)));
}

// A box that is natural on both axes, so its content is the whole of its size.
static voe_ui_node content_box(voe_ui_context *ui, float x, float y)
{
	return voe_ui_box(ui, (voe_math_float2){ x, y },
			  sizing(natural(), natural()));
}

// ---------------------------------------------------------------- a row

// 100 by 20 with 4 of padding and 2 of gap, holding three boxes 10 by 6. The
// run is 34 long inside 92 of room, so 58 is free and each `along` puts it
// somewhere different. Across is START, which in a row is the top, and the top
// inside the padding is 4.
static void row_of_three(voe_ui_context *ui, voe_base_arena *frames,
			 voe_ui_along along, float first, float second,
			 float third)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(frames);
	voe_ui_node a;
	voe_ui_node b;
	voe_ui_node c;

	voe_ui_frame_begin(ui, frames);
	(void)voe_ui_row_begin(ui, (voe_ui_container){
					   .size = sizing(fixed(100.0f),
							  fixed(20.0f)),
					   .along = along,
					   .across = VOE_UI_ACROSS_START,
					   .gap = 2.0f,
					   .pad = 4.0f });
	a = fixed_box(ui, 10.0f, 6.0f);
	b = fixed_box(ui, 10.0f, 6.0f);
	c = fixed_box(ui, 10.0f, 6.0f);
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));

	CHECK_RECT(voe_ui_node_rect(ui, a), first, 4.0f, 10.0f, 6.0f);
	CHECK_RECT(voe_ui_node_rect(ui, b), second, 4.0f, 10.0f, 6.0f);
	CHECK_RECT(voe_ui_node_rect(ui, c), third, 4.0f, 10.0f, 6.0f);

	voe_base_arena_rewind(frames, mark);
}

static void row(voe_ui_context *ui, voe_base_arena *frames)
{
	row_of_three(ui, frames, VOE_UI_ALONG_START, 4.0f, 16.0f, 28.0f);
	// 58 / 2 in front of the run.
	row_of_three(ui, frames, VOE_UI_ALONG_CENTER, 33.0f, 45.0f, 57.0f);
	// All 58 in front, so the last box ends at 100 - 4.
	row_of_three(ui, frames, VOE_UI_ALONG_END, 62.0f, 74.0f, 86.0f);
	// 58 / 2 into each of the two gaps, and none at either end.
	row_of_three(ui, frames, VOE_UI_ALONG_SPREAD, 4.0f, 45.0f, 86.0f);
}

// ------------------------------------------------------------- a column

// The same panel turned on its side, and the case the space's direction exists
// to get wrong: the first box called is the top one and Y INCREASES in call
// order, because the origin is the top-left corner and Y runs down.
static void column(voe_ui_context *ui, voe_base_arena *frames)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(frames);
	voe_ui_node a;
	voe_ui_node b;
	voe_ui_node c;

	voe_ui_frame_begin(ui, frames);
	(void)voe_ui_column_begin(ui, (voe_ui_container){
					      .size = sizing(fixed(100.0f),
							     fixed(20.0f)),
					      .along = VOE_UI_ALONG_START,
					      .across = VOE_UI_ACROSS_START,
					      .gap = 2.0f,
					      .pad = 4.0f });
	a = fixed_box(ui, 10.0f, 6.0f);
	b = fixed_box(ui, 10.0f, 6.0f);
	c = fixed_box(ui, 10.0f, 6.0f);
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));

	// The first box sits on the padding at the top, and across a column
	// START is the left: 0 + 4.
	CHECK_RECT(voe_ui_node_rect(ui, a), 4.0f, 4.0f, 6.0f, 10.0f);
	CHECK_RECT(voe_ui_node_rect(ui, b), 4.0f, 16.0f, 6.0f, 10.0f);
	CHECK_RECT(voe_ui_node_rect(ui, c), 4.0f, 28.0f, 6.0f, 10.0f);

	VOE_TEST_CHECK(voe_ui_node_rect(ui, a).min.y <
		       voe_ui_node_rect(ui, b).min.y);
	VOE_TEST_CHECK(voe_ui_node_rect(ui, b).min.y <
		       voe_ui_node_rect(ui, c).min.y);

	voe_base_arena_rewind(frames, mark);

	// END in a column is the bottom, so the last box ends on the padding.
	mark = voe_base_arena_mark(frames);
	voe_ui_frame_begin(ui, frames);
	(void)voe_ui_column_begin(ui, (voe_ui_container){
					      .size = sizing(fixed(100.0f),
							     fixed(20.0f)),
					      .along = VOE_UI_ALONG_END,
					      .across = VOE_UI_ACROSS_START,
					      .gap = 2.0f,
					      .pad = 4.0f });
	a = fixed_box(ui, 10.0f, 6.0f);
	b = fixed_box(ui, 10.0f, 6.0f);
	c = fixed_box(ui, 10.0f, 6.0f);
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));

	CHECK_RECT(voe_ui_node_rect(ui, a), 4.0f, 62.0f, 6.0f, 10.0f);
	CHECK_RECT(voe_ui_node_rect(ui, b), 4.0f, 74.0f, 6.0f, 10.0f);
	CHECK_RECT(voe_ui_node_rect(ui, c), 4.0f, 86.0f, 6.0f, 10.0f);

	voe_base_arena_rewind(frames, mark);

	// AND THE ASYMMETRIC ONE, WHICH IS THE CASE A REINTRODUCED FLIP CANNOT
	// SURVIVE. Three heights of 3, 7 and 5 with no gap and no padding start
	// at 0, 3 and 10. Flipped, and hung from the bottom of a container 15
	// tall instead, they would be 12, 5 and 0 — three numbers that no
	// tolerance confuses with these.
	mark = voe_base_arena_mark(frames);
	voe_ui_frame_begin(ui, frames);
	(void)voe_ui_column_begin(ui, (voe_ui_container){
					      .size = sizing(fixed(15.0f),
							     fixed(8.0f)),
					      .along = VOE_UI_ALONG_START,
					      .across = VOE_UI_ACROSS_START });
	a = fixed_box(ui, 3.0f, 8.0f);
	b = fixed_box(ui, 7.0f, 8.0f);
	c = fixed_box(ui, 5.0f, 8.0f);
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));

	CHECK_RECT(voe_ui_node_rect(ui, a), 0.0f, 0.0f, 8.0f, 3.0f);
	CHECK_RECT(voe_ui_node_rect(ui, b), 0.0f, 3.0f, 8.0f, 7.0f);
	CHECK_RECT(voe_ui_node_rect(ui, c), 0.0f, 10.0f, 8.0f, 5.0f);

	voe_base_arena_rewind(frames, mark);
}

// ---------------------------------------------------------------- grow

// A fixed box, a natural box, and two grow children of weight 1 and 3 in a row
// 100 long with no padding and no gap. 30 is taken, 70 is left, and the shares
// are 17.5 and 52.5.
static void grow_shares(voe_ui_context *ui, voe_base_arena *frames)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(frames);
	voe_ui_node a;
	voe_ui_node b;
	voe_ui_node c;
	voe_ui_node d;

	voe_ui_frame_begin(ui, frames);
	(void)voe_ui_row_begin(ui, (voe_ui_container){
					   .size = sizing(fixed(100.0f),
							  fixed(20.0f)),
					   .along = VOE_UI_ALONG_START,
					   .across = VOE_UI_ACROSS_START });
	a = fixed_box(ui, 20.0f, 5.0f);
	b = voe_ui_box(ui, (voe_math_float2){ 10.0f, 5.0f },
		       sizing(natural(), fixed(5.0f)));
	c = voe_ui_box(ui, (voe_math_float2){ 0.0f, 0.0f },
		       sizing(grow(1.0f), fixed(5.0f)));
	d = voe_ui_box(ui, (voe_math_float2){ 0.0f, 0.0f },
		       sizing(grow(3.0f), fixed(5.0f)));
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));

	CHECK_RECT(voe_ui_node_rect(ui, a), 0.0f, 0.0f, 20.0f, 5.0f);
	CHECK_RECT(voe_ui_node_rect(ui, b), 20.0f, 0.0f, 10.0f, 5.0f);
	CHECK_RECT(voe_ui_node_rect(ui, c), 30.0f, 0.0f, 17.5f, 5.0f);
	CHECK_RECT(voe_ui_node_rect(ui, d), 47.5f, 0.0f, 52.5f, 5.0f);

	voe_base_arena_rewind(frames, mark);
}

// ------------------------------------------------- grow in a natural row

// THE ANSWER THIS CARD HAD TO GIVE, both ways round. A grow child contributes
// nothing along the flow to its container's natural size, so a spacer between
// two boxes in a row that fits its children collapses to nothing — while the
// gaps on either side of it remain, because a gap belongs to the run and not to
// a child. The same tree in a row 40 long is underneath it, where the spacer is
// the thing that takes up the 16 that is left.
static void grow_in_natural(voe_ui_context *ui, voe_base_arena *frames)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(frames);
	voe_ui_node root;
	voe_ui_node a;
	voe_ui_node spacer;
	voe_ui_node b;

	voe_ui_frame_begin(ui, frames);
	root = voe_ui_row_begin(ui, (voe_ui_container){
					   .size = sizing(natural(),
							  fixed(10.0f)),
					   .along = VOE_UI_ALONG_START,
					   .across = VOE_UI_ACROSS_START,
					   .gap = 2.0f });
	a = fixed_box(ui, 10.0f, 4.0f);
	spacer = voe_ui_box(ui, (voe_math_float2){ 0.0f, 0.0f },
			    sizing(grow(1.0f), natural()));
	b = fixed_box(ui, 10.0f, 4.0f);
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));

	// 10 + nothing + 10, and two gaps of 2.
	CHECK_RECT(voe_ui_node_rect(ui, root), 0.0f, 0.0f, 24.0f, 10.0f);
	CHECK_RECT(voe_ui_node_rect(ui, a), 0.0f, 0.0f, 10.0f, 4.0f);
	CHECK_RECT(voe_ui_node_rect(ui, spacer), 12.0f, 0.0f, 0.0f, 0.0f);
	CHECK_RECT(voe_ui_node_rect(ui, b), 14.0f, 0.0f, 10.0f, 4.0f);

	voe_base_arena_rewind(frames, mark);

	mark = voe_base_arena_mark(frames);
	voe_ui_frame_begin(ui, frames);
	root = voe_ui_row_begin(ui, (voe_ui_container){
					   .size = sizing(fixed(40.0f),
							  fixed(10.0f)),
					   .along = VOE_UI_ALONG_START,
					   .across = VOE_UI_ACROSS_START,
					   .gap = 2.0f });
	a = fixed_box(ui, 10.0f, 4.0f);
	spacer = voe_ui_box(ui, (voe_math_float2){ 0.0f, 0.0f },
			    sizing(grow(1.0f), natural()));
	b = fixed_box(ui, 10.0f, 4.0f);
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));

	CHECK_RECT(voe_ui_node_rect(ui, root), 0.0f, 0.0f, 40.0f, 10.0f);
	CHECK_RECT(voe_ui_node_rect(ui, a), 0.0f, 0.0f, 10.0f, 4.0f);
	CHECK_RECT(voe_ui_node_rect(ui, spacer), 12.0f, 0.0f, 16.0f, 0.0f);
	CHECK_RECT(voe_ui_node_rect(ui, b), 30.0f, 0.0f, 10.0f, 4.0f);

	voe_base_arena_rewind(frames, mark);
}

// -------------------------------------------------------------- across

// One box in a row 40 by 20 with no padding, once for each `across`. The box is
// 6 across, so 14 of the 20 is spare and the four answers put it in four places.
static void across_one(voe_ui_context *ui, voe_base_arena *frames,
		       voe_ui_across across, voe_ui_size box_across,
		       float min_y, float size_y)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(frames);
	voe_ui_node a;

	voe_ui_frame_begin(ui, frames);
	(void)voe_ui_row_begin(ui, (voe_ui_container){
					   .size = sizing(fixed(40.0f),
							  fixed(20.0f)),
					   .along = VOE_UI_ALONG_START,
					   .across = across });
	a = voe_ui_box(ui, (voe_math_float2){ 0.0f, 3.0f },
		       sizing(fixed(10.0f), box_across));
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));

	CHECK_RECT(voe_ui_node_rect(ui, a), 0.0f, min_y, 10.0f, size_y);

	voe_base_arena_rewind(frames, mark);
}

static void across(voe_ui_context *ui, voe_base_arena *frames)
{
	struct voe_base_arena_mark mark;
	voe_ui_node a;

	// START is the top of a row, END is the bottom, CENTER is the middle.
	across_one(ui, frames, VOE_UI_ACROSS_START, fixed(6.0f), 0.0f, 6.0f);
	across_one(ui, frames, VOE_UI_ACROSS_CENTER, fixed(6.0f), 7.0f, 6.0f);
	across_one(ui, frames, VOE_UI_ACROSS_END, fixed(6.0f), 14.0f, 6.0f);

	// FILL stretches a box that said nothing about its size across — the
	// content of 3 is overridden — and leaves one that named a size alone,
	// at the start of the axis. That is the more-specific-wins answer.
	across_one(ui, frames, VOE_UI_ACROSS_FILL, natural(), 0.0f, 20.0f);
	across_one(ui, frames, VOE_UI_ACROSS_FILL, fixed(6.0f), 0.0f, 6.0f);

	// And across a column START is the left, not the top.
	mark = voe_base_arena_mark(frames);
	voe_ui_frame_begin(ui, frames);
	(void)voe_ui_column_begin(ui, (voe_ui_container){
					      .size = sizing(fixed(40.0f),
							     fixed(20.0f)),
					      .along = VOE_UI_ALONG_START,
					      .across = VOE_UI_ACROSS_END });
	a = fixed_box(ui, 10.0f, 6.0f);
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));

	// END across a column is the right-hand edge: 20 - 6.
	CHECK_RECT(voe_ui_node_rect(ui, a), 14.0f, 0.0f, 6.0f, 10.0f);

	voe_base_arena_rewind(frames, mark);
}

// -------------------------------------------------------- a natural root

// The stretch-to-children case: a column that says nothing about its own size
// and comes out the size of what is in it — three boxes 8 by 5, two gaps of 2
// and 4 of padding all round, so 16 by 27.
static void natural_root(voe_ui_context *ui, voe_base_arena *frames)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(frames);
	voe_ui_node root;
	voe_ui_node a;
	voe_ui_node c;

	voe_ui_frame_begin(ui, frames);
	root = voe_ui_column_begin(ui, (voe_ui_container){
					      .size = sizing(natural(),
							     natural()),
					      .along = VOE_UI_ALONG_START,
					      .across = VOE_UI_ACROSS_START,
					      .gap = 2.0f,
					      .pad = 4.0f });
	a = content_box(ui, 8.0f, 5.0f);
	(void)content_box(ui, 8.0f, 5.0f);
	c = content_box(ui, 8.0f, 5.0f);
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));

	CHECK_RECT(voe_ui_node_rect(ui, root), 0.0f, 0.0f, 16.0f, 27.0f);
	// The first child sits on the padding at the top, the last ends on it at
	// the bottom: 18 + 5 is 23, and 27 - 4 is 23.
	CHECK_RECT(voe_ui_node_rect(ui, a), 4.0f, 4.0f, 8.0f, 5.0f);
	CHECK_RECT(voe_ui_node_rect(ui, c), 4.0f, 18.0f, 8.0f, 5.0f);

	voe_base_arena_rewind(frames, mark);
}

// ------------------------------------------------------------ overflow

// Nothing is shrunk to fit and nothing is clipped: two boxes 15 long in a row 20
// long keep their 15 and the second one ends at 30, and a box 20 across in a row
// 10 across hangs 10 past the bottom edge. Both rectangles are true and both are
// the caller's to clip.
static void overflow(voe_ui_context *ui, voe_base_arena *frames)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(frames);
	voe_ui_node a;
	voe_ui_node b;

	voe_ui_frame_begin(ui, frames);
	(void)voe_ui_row_begin(ui, (voe_ui_container){
					   .size = sizing(fixed(20.0f),
							  fixed(10.0f)),
					   .along = VOE_UI_ALONG_START,
					   .across = VOE_UI_ACROSS_START });
	a = fixed_box(ui, 15.0f, 4.0f);
	b = fixed_box(ui, 15.0f, 20.0f);
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));

	CHECK_RECT(voe_ui_node_rect(ui, a), 0.0f, 0.0f, 15.0f, 4.0f);
	CHECK_RECT(voe_ui_node_rect(ui, b), 15.0f, 0.0f, 15.0f, 20.0f);

	voe_base_arena_rewind(frames, mark);

	// AND A CORNER BEFORE THE ORIGIN, which is what centring something
	// bigger than its container comes to: 10 less 20, halved, is -5. It is
	// not clamped, because a rectangle that has been moved to keep it
	// positive is not the rectangle the caller asked for.
	mark = voe_base_arena_mark(frames);
	voe_ui_frame_begin(ui, frames);
	(void)voe_ui_row_begin(ui, (voe_ui_container){
					   .size = sizing(fixed(20.0f),
							  fixed(10.0f)),
					   .along = VOE_UI_ALONG_CENTER,
					   .across = VOE_UI_ACROSS_CENTER });
	a = fixed_box(ui, 30.0f, 20.0f);
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));

	CHECK_RECT(voe_ui_node_rect(ui, a), -5.0f, -5.0f, 30.0f, 20.0f);

	voe_base_arena_rewind(frames, mark);
}

// -------------------------------------------------------------- SPREAD

// The two ways SPREAD has nothing to spread. One child has no gap to put the
// free space in, and a run that already overflows has no free space; both are
// START.
static void spread_degenerates(voe_ui_context *ui, voe_base_arena *frames)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(frames);
	voe_ui_node a;
	voe_ui_node b;

	voe_ui_frame_begin(ui, frames);
	(void)voe_ui_row_begin(ui, (voe_ui_container){
					   .size = sizing(fixed(100.0f),
							  fixed(10.0f)),
					   .along = VOE_UI_ALONG_SPREAD,
					   .across = VOE_UI_ACROSS_START });
	a = fixed_box(ui, 10.0f, 4.0f);
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));

	CHECK_RECT(voe_ui_node_rect(ui, a), 0.0f, 0.0f, 10.0f, 4.0f);
	voe_base_arena_rewind(frames, mark);

	mark = voe_base_arena_mark(frames);
	voe_ui_frame_begin(ui, frames);
	(void)voe_ui_row_begin(ui, (voe_ui_container){
					   .size = sizing(fixed(20.0f),
							  fixed(10.0f)),
					   .along = VOE_UI_ALONG_SPREAD,
					   .across = VOE_UI_ACROSS_START });
	a = fixed_box(ui, 15.0f, 4.0f);
	b = fixed_box(ui, 15.0f, 4.0f);
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));

	CHECK_RECT(voe_ui_node_rect(ui, a), 0.0f, 0.0f, 15.0f, 4.0f);
	CHECK_RECT(voe_ui_node_rect(ui, b), 15.0f, 0.0f, 15.0f, 4.0f);

	voe_base_arena_rewind(frames, mark);
}

// -------------------------------------------------------------- nesting

// A row inside a column inside a row, and the numbers are worked through in the
// comments because this is the case where a padding counted twice or an axis
// swapped shows up. Each container's `size.along` is along ITS PARENT'S flow,
// which is the one thing to keep hold of while reading it.
static void nesting(voe_ui_context *ui, voe_base_arena *frames)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(frames);
	voe_ui_node outer;
	voe_ui_node middle;
	voe_ui_node inner;
	voe_ui_node leaf;

	voe_ui_frame_begin(ui, frames);
	outer = voe_ui_row_begin(ui, (voe_ui_container){
					     .size = sizing(fixed(100.0f),
							    fixed(50.0f)),
					     .along = VOE_UI_ALONG_START,
					     .across = VOE_UI_ACROSS_START,
					     .pad = 5.0f });
	// A column 40 wide (along the row) and 30 tall (across it).
	middle = voe_ui_column_begin(ui, (voe_ui_container){
						 .size = sizing(fixed(40.0f),
								fixed(30.0f)),
						 .along = VOE_UI_ALONG_START,
						 .across = VOE_UI_ACROSS_START,
						 .gap = 4.0f,
						 .pad = 2.0f });
	// A row 10 tall (along the column) and 20 wide (across it).
	inner = voe_ui_row_begin(ui, (voe_ui_container){
					     .size = sizing(fixed(10.0f),
							    fixed(20.0f)),
					     .along = VOE_UI_ALONG_START,
					     .across = VOE_UI_ACROSS_START,
					     .pad = 1.0f });
	leaf = fixed_box(ui, 6.0f, 3.0f);
	voe_ui_end(ui);
	voe_ui_end(ui);
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));

	CHECK_RECT(voe_ui_node_rect(ui, outer), 0.0f, 0.0f, 100.0f, 50.0f);
	// Inside 5 of padding, on both axes, from the top-left corner.
	CHECK_RECT(voe_ui_node_rect(ui, middle), 5.0f, 5.0f, 40.0f, 30.0f);
	// Inside the column's own 2, from its corner at (5, 5).
	CHECK_RECT(voe_ui_node_rect(ui, inner), 7.0f, 7.0f, 20.0f, 10.0f);
	// Inside the inner row's 1, from its corner at (7, 7).
	CHECK_RECT(voe_ui_node_rect(ui, leaf), 8.0f, 8.0f, 6.0f, 3.0f);

	voe_base_arena_rewind(frames, mark);
}

// ------------------------------------------------------------- capacity

// A frame that wants more nodes than the context was made with is refused, not
// fatal: the calls that did not fit hand back VOE_UI_NODE_NONE, frame_end says
// false, and the frame after it lays out as if nothing had happened. A refused
// container is ended like any other, which is what keeps the frame balanced
// without the caller having to check a handle.
static void capacity(voe_base_arena *arena, voe_base_arena *frames)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(frames);
	voe_ui_context *ui = voe_ui_context_new(arena,
						(voe_ui_capacities){ .nodes = 2 });
	voe_ui_node a;
	voe_ui_node refused_row;
	voe_ui_node refused_box;

	voe_ui_frame_begin(ui, frames);
	(void)voe_ui_row_begin(ui, (voe_ui_container){
					   .size = sizing(fixed(40.0f),
							  fixed(10.0f)),
					   .along = VOE_UI_ALONG_START,
					   .across = VOE_UI_ACROSS_START });
	a = fixed_box(ui, 10.0f, 4.0f);
	refused_row = voe_ui_row_begin(ui, (voe_ui_container){
						   .size = sizing(fixed(10.0f),
								  fixed(4.0f)),
						   .along = VOE_UI_ALONG_START,
						   .across = VOE_UI_ACROSS_START });
	refused_box = fixed_box(ui, 1.0f, 1.0f);
	voe_ui_end(ui);
	voe_ui_end(ui);
	VOE_TEST_CHECK(!voe_ui_frame_end(ui));

	VOE_TEST_CHECK(a != VOE_UI_NODE_NONE);
	VOE_TEST_CHECK(refused_row == VOE_UI_NODE_NONE);
	VOE_TEST_CHECK(refused_box == VOE_UI_NODE_NONE);

	voe_base_arena_rewind(frames, mark);

	// The same context, the next frame, within its room: an ordinary answer.
	mark = voe_base_arena_mark(frames);
	voe_ui_frame_begin(ui, frames);
	(void)voe_ui_row_begin(ui, (voe_ui_container){
					   .size = sizing(fixed(40.0f),
							  fixed(10.0f)),
					   .along = VOE_UI_ALONG_END,
					   .across = VOE_UI_ACROSS_START });
	a = fixed_box(ui, 10.0f, 4.0f);
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));

	CHECK_RECT(voe_ui_node_rect(ui, a), 30.0f, 0.0f, 10.0f, 4.0f);

	voe_base_arena_rewind(frames, mark);
}

// ---------------------------------------------------------- twice over

// The same tree, twice on one context, and the same rectangles both times.
// Nothing about layout is kept between frames, and this is what that claim
// looks like as a test rather than as a sentence.
static voe_ui_rect twice_over_frame(voe_ui_context *ui, voe_base_arena *frames)
{
	voe_ui_node a;

	voe_ui_frame_begin(ui, frames);
	(void)voe_ui_column_begin(ui, (voe_ui_container){
					      .size = sizing(natural(),
							     fixed(30.0f)),
					      .along = VOE_UI_ALONG_CENTER,
					      .across = VOE_UI_ACROSS_CENTER,
					      .gap = 3.0f,
					      .pad = 1.0f });
	(void)content_box(ui, 8.0f, 5.0f);
	a = voe_ui_box(ui, (voe_math_float2){ 12.0f, 7.0f },
		       sizing(natural(), fixed(9.0f)));
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));

	return voe_ui_node_rect(ui, a);
}

static void twice_over(voe_ui_context *ui, voe_base_arena *frames)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(frames);
	voe_ui_rect first = twice_over_frame(ui, frames);
	voe_ui_rect second;

	voe_base_arena_rewind(frames, mark);

	mark = voe_base_arena_mark(frames);
	second = twice_over_frame(ui, frames);

	VOE_TEST_CHECK_FLOAT(second.min.x, first.min.x, 0.0);
	VOE_TEST_CHECK_FLOAT(second.min.y, first.min.y, 0.0);
	VOE_TEST_CHECK_FLOAT(second.size.x, first.size.x, 0.0);
	VOE_TEST_CHECK_FLOAT(second.size.y, first.size.y, 0.0);

	voe_base_arena_rewind(frames, mark);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_base_arena *frames = voe_base_arena_new(64 * 1024);
	voe_ui_context *ui = voe_ui_context_new(arena,
						(voe_ui_capacities){ .nodes = 64 });

	row(ui, frames);
	column(ui, frames);
	grow_shares(ui, frames);
	grow_in_natural(ui, frames);
	across(ui, frames);
	natural_root(ui, frames);
	overflow(ui, frames);
	spread_degenerates(ui, frames);
	nesting(ui, frames);
	twice_over(ui, frames);
	capacity(arena, frames);

	voe_base_arena_destroy(frames);
	voe_base_arena_destroy(arena);

	return voe_test_result();
}
