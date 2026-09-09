// The tree, and the two sweeps over it that turn a declaration into rectangles.
//
// THE ARRAY IS IN CALL ORDER, WHICH IS A PRE-ORDER WALK OF THE TREE, AND THAT IS
// WHAT MAKES BOTH PASSES FLAT LOOPS. A node is pushed when its call happens, so
// every child sits at a higher index than its parent. Measure needs children
// before parents, so it sweeps the array backwards; arrange needs parents before
// children, so it sweeps forwards. Each node is touched twice — once on its own
// account and once as one of its parent's children — so both passes are linear.
//
// SO THERE IS NEITHER RECURSION NOR AN EXPLICIT STACK IN EITHER PASS, and the
// question the card asked does not arise: the depth of the tree is not on the C
// stack at all, and no nesting limit is needed. The truetype composite walk had
// to choose, because a font file's nesting is the file's; this tree is built by
// the program's own calls, and the array it is built in already holds the order
// both passes want. The one stack in this file is the list of containers begun
// and not yet ended, which is the building phase and not a pass.
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
// against it is that there is one line each way.
#include <ui/layout.h>

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

struct node {
	uint32_t first_child;
	uint32_t last_child;
	uint32_t next_sibling;
	uint32_t children;

	bool leaf;
	// A container's direction. Unused on a leaf.
	bool row;

	// What the caller declared.
	voe_ui_sizing size;
	voe_math_float2 content;
	voe_ui_along along;
	voe_ui_across across;
	float gap;
	float pad;

	// What measure came to: this node's own content, in the panel's axes.
	voe_math_float2 content_natural;
	// What its parent made of that, in the panel's axes.
	voe_math_float2 natural;
	// What arrange came to.
	voe_ui_rect rect;
};

enum frame_state {
	NOT_IN_FRAME = 0,
	BUILDING,
	LAID_OUT,
};

struct voe_ui_context {
	voe_ui_capacities capacities;

	// This frame's, all of it out of the arena frame_begin was handed.
	struct node *nodes;
	// The containers begun and not yet ended, innermost last.
	uint32_t *open;

	uint32_t count;
	uint32_t depth;
	// Begins refused for want of a node, still to be ended. They are counted
	// rather than pushed, so that a refused frame stays balanced and the
	// caller never has to check a handle before ending it.
	uint32_t refused;

	bool overrun;
	enum frame_state state;
};

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

static void check_sizing(voe_ui_sizing sizing, const char *grow_refused_along)
{
	check_size(sizing.along, grow_refused_along);
	check_size(sizing.across,
		   "grow is along the flow only; across it, a child is natural "
		   "or fixed and the container's across stretches it");
}

// Pushes a node and hands back its index, or VOE_UI_NODE_NONE when the frame has
// no room for another one. The refusal is reported here, where both numbers are,
// and once per frame.
static uint32_t node_push(voe_ui_context *ui)
{
	struct node *n;
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
	n->children = 0;

	if (ui->depth > 0) {
		struct node *parent = &ui->nodes[ui->open[ui->depth - 1]];

		if (parent->last_child == VOE_UI_NODE_NONE)
			parent->first_child = index;
		else
			ui->nodes[parent->last_child].next_sibling = index;
		parent->last_child = index;
		parent->children++;
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

static void measure_natural(struct node *n, bool y_along)
{
	axis_set(&n->natural, y_along,
		 declared(n->size.along, axis(n->content_natural, y_along)));
	axis_set(&n->natural, !y_along,
		 declared(n->size.across, axis(n->content_natural, !y_along)));
}

// Summed along the flow with the gaps, the largest across, and the padding on
// both edges of each. Its children are already measured: they sit at higher
// indices and the sweep runs backwards.
static void measure_container(struct node *nodes, uint32_t index)
{
	struct node *c = &nodes[index];
	bool y = !c->row;
	float along = 0.0f;
	float across = 0.0f;
	uint32_t child;

	for (child = c->first_child; child != VOE_UI_NODE_NONE;
	     child = nodes[child].next_sibling) {
		struct node *n = &nodes[child];

		measure_natural(n, y);
		along += axis(n->natural, y);
		if (axis(n->natural, !y) > across)
			across = axis(n->natural, !y);
	}

	if (c->children > 1)
		along += c->gap * (float)(c->children - 1);

	axis_set(&c->content_natural, y, along + 2.0f * c->pad);
	axis_set(&c->content_natural, !y, across + 2.0f * c->pad);
}

static void measure(struct node *nodes, uint32_t count)
{
	uint32_t i = count;

	while (i-- > 0) {
		if (nodes[i].leaf)
			nodes[i].content_natural = nodes[i].content;
		else
			measure_container(nodes, i);
	}
}

static float along_size(const struct node *n, bool y, float share)
{
	if (n->size.along.kind == VOE_UI_SIZE_GROW)
		return n->size.along.value * share;
	return axis(n->natural, y);
}

// The container's rectangle is already known; this hands every child of it one.
static void arrange_children(struct node *nodes, uint32_t index)
{
	struct node *c = &nodes[index];
	bool y = !c->row;
	float inner_along = axis(c->rect.size, y) - 2.0f * c->pad;
	float inner_across = axis(c->rect.size, !y) - 2.0f * c->pad;
	// Both axes begin at the padded box's own corner and run away from it,
	// rightwards and downwards, so an offset is added on either. There is no
	// minus sign in this function and there is not meant to be one.
	float start_along = axis(c->rect.min, y) + c->pad;
	float start_across = axis(c->rect.min, !y) + c->pad;
	float gaps = c->children > 1 ? c->gap * (float)(c->children - 1) : 0.0f;
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
		struct node *n = &nodes[child];

		if (n->size.along.kind == VOE_UI_SIZE_GROW)
			weight += n->size.along.value;
		else
			taken += axis(n->natural, y);
	}

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
		struct node *n = &nodes[child];

		if (n->size.along.kind == VOE_UI_SIZE_GROW)
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
		// One child sits at START, and so does a run that overflows:
		// there is no free space to put between anything.
		if (c->children > 1 && free_space > 0.0f)
			extra = free_space / (float)(c->children - 1);
		break;
	}

	for (child = c->first_child; child != VOE_UI_NODE_NONE;
	     child = nodes[child].next_sibling) {
		struct node *n = &nodes[child];
		float size_along = along_size(n, y, share);
		float size_across;
		float offset_across = 0.0f;

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
}

static void arrange(struct node *nodes, uint32_t count)
{
	uint32_t i;

	for (i = 0; i < count; i++) {
		if (!nodes[i].leaf)
			arrange_children(nodes, i);
	}
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

	return ui;
}

void voe_ui_frame_begin(voe_ui_context *ui, voe_base_arena *arena)
{
	VOE_BASE_ASSERT(ui != NULL, "beginning a frame on no context");
	VOE_BASE_ASSERT(arena != NULL, "beginning a frame without an arena");
	VOE_BASE_ASSERT(ui->state != BUILDING,
			"beginning a frame inside another one");

	// One push per array, never a push per node: two pushes are not
	// guaranteed to be adjacent, so an array has to be one of them. The
	// stack of open containers is the same capacity because nesting cannot
	// be deeper than the number of nodes.
	ui->nodes = voe_base_arena_push(
		arena, (size_t)ui->capacities.nodes * sizeof(*ui->nodes));
	ui->open = voe_base_arena_push(
		arena, (size_t)ui->capacities.nodes * sizeof(*ui->open));

	ui->count = 0;
	ui->depth = 0;
	ui->refused = 0;
	ui->overrun = false;
	ui->state = BUILDING;
}

static voe_ui_node container_begin(voe_ui_context *ui,
				   voe_ui_container container, bool row)
{
	struct node *n;
	uint32_t index;

	VOE_BASE_ASSERT(ui != NULL, "beginning a container on no context");
	VOE_BASE_ASSERT(ui->state == BUILDING,
			"beginning a container outside a frame");
	VOE_BASE_ASSERT(ui->depth > 0 || ui->refused > 0 || ui->count == 0,
			"one root per frame; a second container at the top of "
			"one has nothing to sit in");
	VOE_BASE_ASSERT(container.gap >= 0.0f, "a gap below nought");
	VOE_BASE_ASSERT(container.pad >= 0.0f, "padding below nought");
	check_sizing(container.size,
		     ui->depth == 0 && ui->refused == 0
			     ? "the root has nothing to grow into; its size is "
			       "fixed or natural on each axis"
			     : NULL);

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
	VOE_BASE_ASSERT(ui->state == BUILDING,
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
	struct node *n;
	uint32_t index;

	VOE_BASE_ASSERT(ui != NULL, "adding a box to no context");
	VOE_BASE_ASSERT(ui->state == BUILDING, "adding a box outside a frame");
	VOE_BASE_ASSERT(ui->depth > 0 || ui->refused > 0,
			"a box cannot be the root; the root is a row or a "
			"column and a box goes inside one");
	VOE_BASE_ASSERT(content.x >= 0.0f && content.y >= 0.0f,
			"a box whose content is smaller than nothing");
	check_sizing(sizing, NULL);

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
	n->pad = 0.0f;

	return index;
}

bool voe_ui_frame_end(voe_ui_context *ui)
{
	struct node *root;

	VOE_BASE_ASSERT(ui != NULL, "ending a frame on no context");
	VOE_BASE_ASSERT(ui->state == BUILDING, "ending a frame that was not begun");
	VOE_BASE_ASSERT(ui->depth == 0 && ui->refused == 0,
			"a container was begun and not ended");

	ui->state = LAID_OUT;

	if (ui->overrun)
		return false;
	// A frame that declared nothing is not a failure: a program with no
	// interface on screen this frame builds no tree.
	if (ui->count == 0)
		return true;

	measure(ui->nodes, ui->count);

	// The root is measured against its own flow, because it has no parent
	// whose flow to be measured against, and it sits at the origin.
	root = &ui->nodes[0];
	measure_natural(root, !root->row);
	root->rect.min = (voe_math_float2){ 0.0f, 0.0f };
	root->rect.size = root->natural;

	arrange(ui->nodes, ui->count);

	return true;
}

voe_ui_rect voe_ui_node_rect(const voe_ui_context *ui, voe_ui_node node)
{
	VOE_BASE_ASSERT(ui != NULL, "reading a rectangle from no context");
	VOE_BASE_ASSERT(ui->state == LAID_OUT,
			"reading a rectangle before the frame has ended");
	VOE_BASE_ASSERT(node != VOE_UI_NODE_NONE,
			"reading a rectangle through a node the frame had no "
			"room for");
	VOE_BASE_ASSERT(node < ui->count, "reading a rectangle through a node "
					  "this frame never made");

	return ui->nodes[node].rect;
}
