// The tree, and the two sweeps over it that turn a declaration into rectangles.
//
// THE ARRAY IS IN CALL ORDER, WHICH IS A PRE-ORDER WALK OF THE TREE, AND THAT IS
// WHAT MAKES BOTH PASSES FLAT LOOPS. A node is pushed when its call happens, so
// every child sits at a higher index than its parent. Measure needs children
// before parents, so it sweeps the array backwards; arrange needs parents before
// children, so it sweeps forwards. Each node is touched twice — once on its own
// account and once as one of its parent's children — so both passes are linear.
//
// SO THERE IS NEITHER RECURSION NOR AN EXPLICIT STACK IN ANY PASS, and the
// question the card asked does not arise: the depth of the tree is not on the C
// stack at all, and no nesting limit is needed. The truetype composite walk had
// to choose, because a font file's nesting is the file's; this tree is built by
// the program's own calls, and the array it is built in already holds the order
// both passes want. The one stack in this file is the list of containers begun
// and not yet ended, which is the building phase and not a pass.
//
// THERE IS A THIRD PASS SINCE CARD 041 AND IT IS FLAT FOR THE SAME REASON.
// Anchored children paint after their in-flow siblings, so paint order is no
// longer the array's order and has to be worked out — but a pre-order array
// gives it in three linear sweeps and no stack: count each node's subtree
// backwards, which is the width of the slot that subtree needs; sweep forwards
// handing each node's children their slots inside its own, the in-flow ones
// before the anchored ones; and invert what that wrote. See paint_order.
//
// A NODE'S NATURAL SIZE IS WRITTEN BY ITS PARENT, NOT BY ITSELF, because what
// `along` means to a node is a question about its parent's direction. So each
// node measures its own content into `content_natural`, in the panel's axes, and
// the parent turns that into `natural` by reading the node's declaration against
// its own flow. The root has no parent and is measured against its own flow.
//
// A GROW CHILD CONTRIBUTES NOTHING ALONG THE FLOW TO ITS CONTAINER'S NATURAL
// SIZE, and its content across. That is the circular question this card had to
// answer — grow means share what is left, natural means what the content needs,
// and a container whose natural size depended on a share of itself would need
// iterating. Contributing nothing keeps natural size well defined in one pass,
// it is what flexbox settles on with a zero basis, and it means a grow spacer in
// a natural panel collapses to nothing, which is what a spacer should do when
// there is nothing to fill. The two alternatives were measuring a grow child's
// content anyway, which costs a second measure and blurs what grow means, and
// refusing grow inside a natural container, which is too strict for a panel that
// fits its children and has one spacer in it.
//
// GAPS ARE COUNTED FOR EVERY CHILD, GROW ONES INCLUDED. The gap is a property of
// the run and not of a child's size, so a natural row of two boxes and a
// collapsed spacer between them is still two gaps wide.
//
// AN ANCHORED CHILD IS NOT IN THE RUN AT ALL, WHICH IS A STRONGER STATEMENT THAN
// A GROW CHILD'S. A grow child contributes nothing to the natural size but is
// still a member of the run and still earns a gap beside it; an anchored child
// is skipped by both passes' arithmetic entirely — no gap, no share, no
// contribution — and is placed afterwards against its parent's CONTENT BOX,
// which is the padded rectangle and not the outer one. So `flow`, the count of
// the children still in the run, is what every piece of run arithmetic here is
// written against, and the total child count is not kept at all. The consequence
// worth saying out loud is that a fit-to-children container holding only
// anchored children measures to its padding and nothing more.
//
// AN ANCHORED CHILD'S TWO AXES ARE THE PANEL'S OWN X AND Y, NEVER ITS PARENT'S
// FLOW, and that is why measure_natural is called with `false` for one — the
// mapping that reads `size.along` as X and `size.across` as Y. A child out of
// the flow has no flow, and an anchor that changed which edge it meant when its
// parent turned from a row into a column is the defect this folder's vocabulary
// exists to avoid. The padding is four numbers for the same reason.
//
// NOTHING IN THIS FILE FLIPS A Y AND NOTHING IN IT SUBTRACTS ONE. The space runs
// down from the panel's top-left corner, so a container's children accumulate
// away from its own corner on both axes and the two are the same arithmetic with
// the components swapped — which is why `axis` and `axis_set` are the whole of
// the direction handling and why arrange_children has one expression for a
// child's corner rather than two. A reader who knows this engine will expect a
// flip here, because the world is +Y up; the surface being laid out is a flat
// thing's own parameter space and voe_render_element's header is where the one
// sign that reconciles the two is written down. Putting a second one here is the
// way this file goes wrong, and it would pass a symmetrical test.
//
// PADDING IS SUBTRACTED IN ONE PLACE, IN ARRANGE, AND ADDED IN ONE PLACE, IN
// MEASURE. Counting it twice is the classic off-by-pad and the only guard
// against it is that there is one line each way. Four numbers give it four ways
// to happen, so both directions go through pad_axis and pad_near and neither
// names a side of the struct: a pass asks for the two sides on an axis, or for
// the near side of one, and never for `left` as such.
//
// THE TREE AND THE CONTEXT ARE DECLARED IN src/context.h AND NOT HERE, because
// widgets.c is the other half of the same frame and reads the same rectangles.
// This file still owns every one of the layout fields and writes all of them;
// what it does not own it does not touch. The two calls it makes into widgets.c
// are at the bottom of context.h, and both are at frame boundaries: a widget
// pass that ran anywhere but after arrange would be testing a click against
// rectangles that do not exist yet.
#include "context.h"

#include <base/assert.h>

#include <stdio.h>

// The two axes are the panel's, and `y` in this file always means "this axis is
// Y". A row flows along X, a column along Y, so a container's along axis is Y
// exactly when it is not a row.
static float axis(voe_math_float2 v, bool y)
{
	return y ? v.y : v.x;
}

static void axis_set(voe_math_float2 *v, bool y, float value)
{
	if (y)
		v->y = value;
	else
		v->x = value;
}

// The two sides of the padding on one axis, added, and the near side of it on
// its own. Padding is named by absolute side and both passes want it by axis, so
// these two are the only readers of the four fields.
static float pad_axis(voe_ui_pad pad, bool y)
{
	return y ? pad.top + pad.bottom : pad.left + pad.right;
}

static float pad_near(voe_ui_pad pad, bool y)
{
	return y ? pad.top : pad.left;
}

// `grow_refused` is the message for a grow that is not allowed on this axis, and
// NULL where one is. Across the flow, and at the root, there is nothing to grow
// into.
static void check_size(voe_ui_size size, const char *grow_refused)
{
	switch (size.kind) {
	case VOE_UI_SIZE_NATURAL:
		VOE_BASE_ASSERT(size.value == 0.0f,
				"a natural size carries no number; a box's is "
				"its content and a container's is its children");
		break;
	case VOE_UI_SIZE_FIXED:
		VOE_BASE_ASSERT(size.value >= 0.0f, "a fixed size below nought");
		break;
	case VOE_UI_SIZE_GROW:
		VOE_BASE_ASSERT(grow_refused == NULL, grow_refused);
		VOE_BASE_ASSERT(size.value > 0.0f,
				"a grow weight of nought shares nothing");
		break;
	}
}

// An anchored child has no flow, so both of its axes refuse grow and for one
// reason; everything else refuses it across the flow and for another.
static const char *const grow_unanchored =
	"an anchored child is out of the flow, so there is no run and no "
	"leftover to share; its size is fixed or natural on each axis";

static void check_sizing(voe_ui_sizing sizing, const char *grow_refused_along,
			 const char *grow_refused_across)
{
	check_size(sizing.along, grow_refused_along);
	check_size(sizing.across, grow_refused_across);
}

// What a child in the flow says about its across axis, which is every case but
// an anchored one.
static const char *const grow_across =
	"grow is along the flow only; across it, a child is natural or fixed "
	"and the container's across stretches it";

// Pushes a node and hands back its index, or VOE_UI_NODE_NONE when the frame has
// no room for another one. The refusal is reported here, where both numbers are,
// and once per frame.
static uint32_t node_push(voe_ui_context *ui)
{
	struct voe_ui_node_record *n;
	uint32_t index;

	if (ui->count == ui->capacities.nodes) {
		if (!ui->overrun)
			fprintf(stderr,
				"voe_ui: node %u refused, this context was "
				"created with room for %u\n",
				ui->count + 1, ui->capacities.nodes);
		ui->overrun = true;
		return VOE_UI_NODE_NONE;
	}

	index = ui->count++;
	n = &ui->nodes[index];
	n->first_child = VOE_UI_NODE_NONE;
	n->last_child = VOE_UI_NODE_NONE;
	n->next_sibling = VOE_UI_NODE_NONE;

	if (ui->depth > 0) {
		struct voe_ui_node_record *parent =
			&ui->nodes[ui->open[ui->depth - 1]];

		if (parent->last_child == VOE_UI_NODE_NONE)
			parent->first_child = index;
		else
			ui->nodes[parent->last_child].next_sibling = index;
		parent->last_child = index;
	}

	return index;
}

// A node's natural size in the panel's axes, as its parent sees it: the
// declaration decides, and content_natural is what NATURAL reads.
static float declared(voe_ui_size size, float natural)
{
	switch (size.kind) {
	case VOE_UI_SIZE_FIXED:
		return size.value;
	case VOE_UI_SIZE_GROW:
		return 0.0f;
	case VOE_UI_SIZE_NATURAL:
		break;
	}
	return natural;
}

static void measure_natural(struct voe_ui_node_record *n, bool y_along)
{
	axis_set(&n->natural, y_along,
		 declared(n->size.along, axis(n->content_natural, y_along)));
	axis_set(&n->natural, !y_along,
		 declared(n->size.across, axis(n->content_natural, !y_along)));
}

// Summed along the flow with the gaps, the largest across, and the padding on
// both edges of each. Its children are already measured: they sit at higher
// indices and the sweep runs backwards.
static void measure_container(struct voe_ui_node_record *nodes, uint32_t index)
{
	struct voe_ui_node_record *c = &nodes[index];
	bool y = !c->row;
	float along = 0.0f;
	float across = 0.0f;
	uint32_t flow = 0;
	uint32_t child;

	for (child = c->first_child; child != VOE_UI_NODE_NONE;
	     child = nodes[child].next_sibling) {
		struct voe_ui_node_record *n = &nodes[child];

		// An anchored child is measured in the panel's own axes, having
		// no flow of its parent's to be measured against, and then
		// contributes nothing to this run: no size, and no gap either.
		// So a container that fits its children and holds only anchored
		// ones comes out at its padding and nothing else, which is the
		// trap this folder's header says out loud rather than hides.
		if (n->anchor.anchored) {
			measure_natural(n, false);
			continue;
		}

		measure_natural(n, y);
		flow++;
		along += axis(n->natural, y);
		if (axis(n->natural, !y) > across)
			across = axis(n->natural, !y);
	}

	if (flow > 1)
		along += c->gap * (float)(flow - 1);

	axis_set(&c->content_natural, y, along + pad_axis(c->pad, y));
	axis_set(&c->content_natural, !y, across + pad_axis(c->pad, !y));
}

static void measure(struct voe_ui_node_record *nodes, uint32_t count)
{
	uint32_t i = count;

	while (i-- > 0) {
		if (nodes[i].leaf)
			nodes[i].content_natural = nodes[i].content;
		else
			measure_container(nodes, i);
	}
}

static float along_size(const struct voe_ui_node_record *n, bool y, float share)
{
	if (n->size.along.kind == VOE_UI_SIZE_GROW)
		return n->size.along.value * share;
	return axis(n->natural, y);
}

// One axis of one anchored child, against its parent's content box. `y` says
// which axis it is, and it is the panel's own X or Y and never the parent's
// flow: a child out of the flow has no flow, so `size.along` is read as X and
// `size.across` as Y, matching the two anchors beside them.
static void anchor_axis(struct voe_ui_node_record *n, voe_ui_anchor_axis a,
			bool y, float inner_min, float inner_size)
{
	voe_ui_size declared_size = y ? n->size.across : n->size.along;
	float size = axis(n->natural, y);
	// START and FILL share this corner. A FILL that kept a size the child
	// declared has no stretching left to do and so sits at the start of the
	// axis, which is the answer a fixed child under a container's FILL
	// already gets: one question, one rule, both paths.
	float min = inner_min + a.offset;

	if (a.align == VOE_UI_ACROSS_FILL &&
	    declared_size.kind != VOE_UI_SIZE_FIXED) {
		// Both edges, so the offset is taken off each of them.
		size = inner_size - 2.0f * a.offset;
		// Offsets that meet or cross leave nothing to draw, and that is
		// nought and not a negative number: a rectangle of negative size
		// reaching an element record is wound the wrong way, while an
		// empty one is simply not seen.
		if (size < 0.0f)
			size = 0.0f;
	}

	switch (a.align) {
	case VOE_UI_ACROSS_START:
	case VOE_UI_ACROSS_FILL:
		break;
	case VOE_UI_ACROSS_CENTER:
		min = inner_min + (inner_size - size) * 0.5f + a.offset;
		break;
	case VOE_UI_ACROSS_END:
		// The far edge less the child and less the offset, so a
		// positive offset moves it inward exactly as it does at START.
		// This subtracts sizes on an axis; it does not negate a Y.
		min = inner_min + inner_size - size - a.offset;
		break;
	}

	axis_set(&n->rect.size, y, size);
	axis_set(&n->rect.min, y, min);
}

// The container's rectangle is already known; this hands every child of it one.
static void arrange_children(struct voe_ui_node_record *nodes, uint32_t index)
{
	struct voe_ui_node_record *c = &nodes[index];
	bool y = !c->row;
	float inner_along = axis(c->rect.size, y) - pad_axis(c->pad, y);
	float inner_across = axis(c->rect.size, !y) - pad_axis(c->pad, !y);
	// Both axes begin at the padded box's own corner and run away from it,
	// rightwards and downwards, so an offset is added on either. There is no
	// minus sign in this function and there is not meant to be one.
	float start_along = axis(c->rect.min, y) + pad_near(c->pad, y);
	float start_across = axis(c->rect.min, !y) + pad_near(c->pad, !y);
	// The run is the in-flow children and only them, so every number below
	// is counted against `flow` and never against how many children there
	// are: an anchored child takes no space in the run and earns no gap.
	uint32_t flow = 0;
	float gaps;
	float taken = 0.0f;
	float weight = 0.0f;
	float leftover;
	float share = 0.0f;
	float used;
	float free_space;
	float offset = 0.0f;
	float extra = 0.0f;
	uint32_t child;

	for (child = c->first_child; child != VOE_UI_NODE_NONE;
	     child = nodes[child].next_sibling) {
		struct voe_ui_node_record *n = &nodes[child];

		if (n->anchor.anchored)
			continue;

		flow++;
		if (n->size.along.kind == VOE_UI_SIZE_GROW)
			weight += n->size.along.value;
		else
			taken += axis(n->natural, y);
	}

	gaps = flow > 1 ? c->gap * (float)(flow - 1) : 0.0f;

	// What the grow children divide between them. A row that already
	// overflows has nothing left to share, and a grow child in it gets
	// nought rather than a negative size.
	leftover = inner_along - taken - gaps;
	if (weight > 0.0f && leftover > 0.0f)
		share = leftover / weight;

	// What is left after the grow children have taken theirs is what `along`
	// distributes. With a grow child present that is nought, which is why
	// grow and a distribution do not fight.
	used = taken + gaps;
	for (child = c->first_child; child != VOE_UI_NODE_NONE;
	     child = nodes[child].next_sibling) {
		struct voe_ui_node_record *n = &nodes[child];

		if (!n->anchor.anchored &&
		    n->size.along.kind == VOE_UI_SIZE_GROW)
			used += along_size(n, y, share);
	}
	free_space = inner_along - used;

	switch (c->along) {
	case VOE_UI_ALONG_START:
		break;
	case VOE_UI_ALONG_CENTER:
		// Not clamped when the run overflows: a run wider than its
		// container is centred on it and sticks out at both ends, which
		// is the true rectangle and this folder reports those.
		offset = free_space * 0.5f;
		break;
	case VOE_UI_ALONG_END:
		offset = free_space;
		break;
	case VOE_UI_ALONG_SPREAD:
	case VOE_UI_ALONG_EVENLY:
		// The two distributions degenerate identically and share the one
		// guard that says so: with a single child, or with a run that
		// already overflows, there is no free space to hand out and both
		// are START.
		if (flow > 1 && free_space > 0.0f) {
			if (c->along == VOE_UI_ALONG_SPREAD) {
				// Between the children and none at the ends.
				extra = free_space / (float)(flow - 1);
			} else {
				// Every gap the same, the ends included, so one
				// more gap than SPREAD has — and the first of
				// them goes in front of the run.
				extra = free_space / (float)(flow + 1);
				offset = extra;
			}
		}
		break;
	}

	for (child = c->first_child; child != VOE_UI_NODE_NONE;
	     child = nodes[child].next_sibling) {
		struct voe_ui_node_record *n = &nodes[child];
		float size_along;
		float size_across;
		float offset_across = 0.0f;

		if (n->anchor.anchored)
			continue;

		size_along = along_size(n, y, share);

		// A child's own fixed size across the flow beats the container's
		// FILL: the more specific statement wins.
		if (n->size.across.kind == VOE_UI_SIZE_FIXED)
			size_across = n->size.across.value;
		else if (c->across == VOE_UI_ACROSS_FILL)
			size_across = inner_across;
		else
			size_across = axis(n->natural, !y);

		switch (c->across) {
		case VOE_UI_ACROSS_START:
			break;
		case VOE_UI_ACROSS_CENTER:
			offset_across = (inner_across - size_across) * 0.5f;
			break;
		case VOE_UI_ACROSS_END:
			offset_across = inner_across - size_across;
			break;
		case VOE_UI_ACROSS_FILL:
			// Filled, or fixed and so left at the start of the axis.
			break;
		}

		axis_set(&n->rect.size, y, size_along);
		axis_set(&n->rect.size, !y, size_across);
		axis_set(&n->rect.min, y, start_along + offset);
		axis_set(&n->rect.min, !y, start_across + offset_across);

		offset += size_along + c->gap + extra;
	}

	// AND THE ANCHORED CHILDREN AFTERWARDS, IN CALL ORDER AMONG THEMSELVES.
	// After, and not before, because submission order is paint order on the
	// element path: an anchored panel has to be handed over later than the
	// siblings it floats above or it goes underneath them, and that is
	// invisible until something draws. paint_order is what carries this into
	// emission; this loop is only where the rectangles come from.
	//
	// Against the content box on both axes — inside the padding, which is
	// what start_* and inner_* already are — and each axis worked out on its
	// own, so the sixteen combinations need no cases of their own.
	for (child = c->first_child; child != VOE_UI_NODE_NONE;
	     child = nodes[child].next_sibling) {
		struct voe_ui_node_record *n = &nodes[child];
		bool row = c->row;

		if (!n->anchor.anchored)
			continue;

		// start_along and inner_along are the X pair in a row and the Y
		// pair in a column, so the anchor's own X and Y are picked out
		// of them here rather than a second set being worked out.
		anchor_axis(n, n->anchor.x, false,
			    row ? start_along : start_across,
			    row ? inner_along : inner_across);
		anchor_axis(n, n->anchor.y, true,
			    row ? start_across : start_along,
			    row ? inner_across : inner_along);
	}
}

static void arrange(struct voe_ui_node_record *nodes, uint32_t count)
{
	uint32_t i;

	for (i = 0; i < count; i++) {
		if (!nodes[i].leaf)
			arrange_children(nodes, i);
	}
}

// Hands every child of `parent` whose anchoring matches `anchored` its slot in
// paint order, the first at `at`, and answers where the next run would begin.
// Call order among them is the sibling list's order, which is call order.
static uint32_t paint_run(struct voe_ui_node_record *nodes, uint32_t parent,
			  bool anchored, uint32_t at)
{
	uint32_t child;

	for (child = nodes[parent].first_child; child != VOE_UI_NODE_NONE;
	     child = nodes[child].next_sibling) {
		if (nodes[child].anchor.anchored != anchored)
			continue;

		nodes[child].paint = at;
		at += nodes[child].subtree;
	}

	return at;
}

// The order the tree is painted in: a parent before all of its children, and a
// parent's in-flow children before its anchored ones, so a floating panel is
// submitted after what it floats above and draws on top of it.
//
// THREE FLAT SWEEPS AND NO STACK, for the reason at the top of this file. The
// backward one counts each subtree, which is how wide a slot that subtree needs.
// The forward one gives each node's children slots inside its own — and it may
// read a node's own slot the moment it reaches it, because the array is a
// pre-order walk and a parent is therefore always reached first. The last
// inverts node-to-position into position-to-node, which is what the accessor
// hands out.
static void paint_order(struct voe_ui_node_record *nodes, uint32_t count,
			uint32_t *order)
{
	uint32_t i = count;

	while (i-- > 0) {
		uint32_t child;

		nodes[i].subtree = 1;
		for (child = nodes[i].first_child; child != VOE_UI_NODE_NONE;
		     child = nodes[child].next_sibling)
			nodes[i].subtree += nodes[child].subtree;
	}

	// The root is not anybody's child, so it is the one slot given here.
	nodes[0].paint = 0;
	for (i = 0; i < count; i++)
		paint_run(nodes, i, true,
			  paint_run(nodes, i, false, nodes[i].paint + 1));

	for (i = 0; i < count; i++)
		order[nodes[i].paint] = i;
}

voe_ui_context *voe_ui_context_new(voe_base_arena *arena,
				   voe_ui_capacities capacities)
{
	voe_ui_context *ui;

	VOE_BASE_ASSERT(arena != NULL, "making a ui context without an arena");
	VOE_BASE_ASSERT(capacities.nodes > 0,
			"a ui context with room for no nodes");

	ui = voe_base_arena_push(arena, sizeof(*ui));
	ui->capacities = capacities;
	// The one field that is not nought to begin with. See widgets.h.
	ui->text_scale = 1.0f;

	return ui;
}

void voe_ui_frame_begin(voe_ui_context *ui, voe_base_arena *arena)
{
	VOE_BASE_ASSERT(ui != NULL, "beginning a frame on no context");
	VOE_BASE_ASSERT(arena != NULL, "beginning a frame without an arena");
	VOE_BASE_ASSERT(ui->state != VOE_UI_BUILDING,
			"beginning a frame inside another one");

	// One push per array, never a push per node: two pushes are not
	// guaranteed to be adjacent, so an array has to be one of them. The
	// stack of open containers is the same capacity because nesting cannot
	// be deeper than the number of nodes.
	ui->nodes = voe_base_arena_push(
		arena, (size_t)ui->capacities.nodes * sizeof(*ui->nodes));
	ui->open = voe_base_arena_push(
		arena, (size_t)ui->capacities.nodes * sizeof(*ui->open));
	ui->order = voe_base_arena_push(
		arena, (size_t)ui->capacities.nodes * sizeof(*ui->order));

	ui->count = 0;
	ui->depth = 0;
	ui->refused = 0;
	ui->overrun = false;
	ui->state = VOE_UI_BUILDING;

	voe_ui_widgets_frame_begin(ui, arena);
}

static voe_ui_node container_begin(voe_ui_context *ui,
				   voe_ui_container container, bool row)
{
	struct voe_ui_node_record *n;
	uint32_t index;

	VOE_BASE_ASSERT(ui != NULL, "beginning a container on no context");
	VOE_BASE_ASSERT(ui->state == VOE_UI_BUILDING,
			"beginning a container outside a frame");
	VOE_BASE_ASSERT(ui->depth > 0 || ui->refused > 0 || ui->count == 0,
			"one root per frame; a second container at the top of "
			"one has nothing to sit in");
	VOE_BASE_ASSERT(container.gap >= 0.0f, "a gap below nought");
	VOE_BASE_ASSERT(container.pad.left >= 0.0f &&
				container.pad.top >= 0.0f &&
				container.pad.right >= 0.0f &&
				container.pad.bottom >= 0.0f,
			"padding below nought on one of the four sides");
	// An anchor's offsets are deliberately unchecked: a negative one moves a
	// child outward and out of its parent, and this folder reports the
	// rectangle that comes of it rather than correcting it.
	VOE_BASE_ASSERT(!container.anchor.anchored ||
				ui->depth > 0 || ui->refused > 0,
			"the root has nothing to anchor to; anchoring pins a "
			"child to its parent's content box and the root has no "
			"parent");
	check_sizing(container.size,
		     ui->depth == 0 && ui->refused == 0
			     ? "the root has nothing to grow into; its size is "
			       "fixed or natural on each axis"
		     : container.anchor.anchored ? grow_unanchored
						 : NULL,
		     container.anchor.anchored ? grow_unanchored : grow_across);

	index = node_push(ui);
	if (index == VOE_UI_NODE_NONE) {
		ui->refused++;
		return VOE_UI_NODE_NONE;
	}

	n = &ui->nodes[index];
	n->leaf = false;
	n->row = row;
	n->size = container.size;
	n->content = (voe_math_float2){ 0.0f, 0.0f };
	n->along = container.along;
	n->across = container.across;
	n->gap = container.gap;
	n->pad = container.pad;
	n->anchor = container.anchor;

	ui->open[ui->depth++] = index;

	return index;
}

voe_ui_node voe_ui_row_begin(voe_ui_context *ui, voe_ui_container container)
{
	return container_begin(ui, container, true);
}

voe_ui_node voe_ui_column_begin(voe_ui_context *ui, voe_ui_container container)
{
	return container_begin(ui, container, false);
}

void voe_ui_end(voe_ui_context *ui)
{
	VOE_BASE_ASSERT(ui != NULL, "ending a container on no context");
	VOE_BASE_ASSERT(ui->state == VOE_UI_BUILDING,
			"ending a container outside a frame");
	VOE_BASE_ASSERT(ui->depth > 0 || ui->refused > 0,
			"ending a container that was never begun");

	// A refused begin was never pushed, so it is unwound here rather than
	// popped. Refusals are always the innermost containers open, because
	// once the array is full nothing else is pushed.
	if (ui->refused > 0)
		ui->refused--;
	else
		ui->depth--;
}

voe_ui_node voe_ui_box(voe_ui_context *ui, voe_math_float2 content,
		       voe_ui_sizing sizing)
{
	struct voe_ui_node_record *n;
	uint32_t index;

	VOE_BASE_ASSERT(ui != NULL, "adding a box to no context");
	VOE_BASE_ASSERT(ui->state == VOE_UI_BUILDING,
			"adding a box outside a frame");
	VOE_BASE_ASSERT(ui->depth > 0 || ui->refused > 0,
			"a box cannot be the root; the root is a row or a "
			"column and a box goes inside one");
	VOE_BASE_ASSERT(content.x >= 0.0f && content.y >= 0.0f,
			"a box whose content is smaller than nothing");
	check_sizing(sizing, NULL, grow_across);

	index = node_push(ui);
	if (index == VOE_UI_NODE_NONE)
		return VOE_UI_NODE_NONE;

	n = &ui->nodes[index];
	n->leaf = true;
	n->row = false;
	n->size = sizing;
	n->content = content;
	n->along = VOE_UI_ALONG_START;
	n->across = VOE_UI_ACROSS_START;
	n->gap = 0.0f;
	n->pad = (voe_ui_pad){ 0 };
	// Anchoring is a container's, so a leaf that wants to float is wrapped
	// in an anchored one. See voe_ui_box's header.
	n->anchor = (voe_ui_anchor){ 0 };

	return index;
}

bool voe_ui_frame_end(voe_ui_context *ui)
{
	struct voe_ui_node_record *root;
	bool ok;

	VOE_BASE_ASSERT(ui != NULL, "ending a frame on no context");
	VOE_BASE_ASSERT(ui->state == VOE_UI_BUILDING,
			"ending a frame that was not begun");
	VOE_BASE_ASSERT(ui->depth == 0 && ui->refused == 0,
			"a container was begun and not ended");

	ui->state = VOE_UI_LAID_OUT;

	// A frame that declared nothing is not a failure: a program with no
	// interface on screen this frame builds no tree. It still goes through
	// the widget pass, because letting go of a held button is something a
	// frame with nothing in it has to do.
	ok = !ui->overrun && !ui->collision;
	// Paint order is the tree's shape and not the arrangement's, so it is
	// worked out even for a refused frame — which keeps the accessor's
	// answer a real position in every frame that built any tree at all,
	// rather than one that is only meaningful when `ok`.
	if (ui->count > 0)
		paint_order(ui->nodes, ui->count, ui->order);

	if (ok && ui->count > 0) {
		measure(ui->nodes, ui->count);

		// The root is measured against its own flow, because it has no
		// parent whose flow to be measured against, and it sits at the
		// origin.
		root = &ui->nodes[0];
		measure_natural(root, !root->row);
		root->rect.min = (voe_math_float2){ 0.0f, 0.0f };
		root->rect.size = root->natural;

		arrange(ui->nodes, ui->count);
	}

	voe_ui_widgets_frame_end(ui, ok);

	// Emission is the last thing that can want more than it was given, and
	// it happens inside this call, so it is answered by this call.
	return ok && !ui->element_overrun;
}

uint32_t voe_ui_paint_order(const voe_ui_context *ui, uint32_t position)
{
	VOE_BASE_ASSERT(position < ui->count,
			"asking layout for a position past the tree");

	return ui->order[position];
}

voe_ui_rect voe_ui_node_rect(const voe_ui_context *ui, voe_ui_node node)
{
	VOE_BASE_ASSERT(ui != NULL, "reading a rectangle from no context");
	VOE_BASE_ASSERT(ui->state == VOE_UI_LAID_OUT,
			"reading a rectangle before the frame has ended");
	VOE_BASE_ASSERT(node != VOE_UI_NODE_NONE,
			"reading a rectangle through a node the frame had no "
			"room for");
	VOE_BASE_ASSERT(node < ui->count, "reading a rectangle through a node "
					  "this frame never made");

	return ui->nodes[node].rect;
}

voe_math_float2 voe_ui_node_measured(const voe_ui_context *ui, voe_ui_node node)
{
	VOE_BASE_ASSERT(ui != NULL, "reading a measured size from no context");
	VOE_BASE_ASSERT(ui->state == VOE_UI_LAID_OUT,
			"reading a measured size before the frame has ended");
	VOE_BASE_ASSERT(node != VOE_UI_NODE_NONE,
			"reading a measured size through a node the frame had "
			"no room for");
	VOE_BASE_ASSERT(node < ui->count, "reading a measured size through a "
					  "node this frame never made");

	// content_natural and not natural: this is what the node's own content
	// measured to, before its own FIXED or GROW declaration was applied to
	// it. `natural` is that declaration's answer and it is already the
	// rectangle, so handing it back here would make the comparison the whole
	// accessor exists for read nought every time.
	return ui->nodes[node].content_natural;
}
