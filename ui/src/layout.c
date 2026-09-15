// The tree, and the sweeps over it that turn a declaration into rectangles.
//
// THE ARRAY IS IN CALL ORDER, WHICH IS A PRE-ORDER WALK OF THE TREE, AND THAT IS
// WHAT MAKES BOTH PASSES FLAT LOOPS. A node is pushed when its call happens, so
// every child sits at a higher index than its parent. Measure needs children
// before parents, so it sweeps the array backwards; arrange needs parents before
// children, so it sweeps forwards. Each node is touched twice — once on its own
// account and once as one of its parent's children — so both passes are linear.
//
// SINCE CARD 072 THE PAIR RUNS ONCE PER AXIS: measure and arrange X over the
// whole tree, then measure and arrange Y. Nothing on one axis reads the other,
// except the one thing the order exists for — a wrapping row's lines, broken
// while arranging X, are what measuring Y stacks up. `line` on each child is how
// the two passes speak, and a container that does not wrap leaves every child on
// line nought, which is a single run and exactly the arithmetic there was before.
//
// A WRAP IS DECIDED IN arrange, SO THE measure THAT FED ITS ANCESTORS HAS
// ALREADY RUN, and every one of them keeps the pre-wrap number. A row breaks
// while arranging X, after measure(X) told its parent it was one line long; a
// column breaks while arranging Y and revises both of its own axes there, after
// measure(Y) and measure(X) had both been believed. So there is a corrective
// sweep, remeasure, between the two passes and the clip: it measures each axis
// again — reading the `line` numbers the arrangement wrote — and re-clamps every
// container's offset against the number that comes out.
//
// IT REVISES MEASURES AND NEVER RECTANGLES, the root's included. The width a
// container wrapped at is the width it was arranged to, and re-arranging X from
// the revised measure could break the lines differently and want measuring
// again; that iteration is the one thing the axis order exists to rule out. So
// a NATURAL-width ancestor keeps its pre-wrap rectangle and now measures less
// than it, which is the true statement and the one voe_ui_node_measured exists
// to make.
//
// IT RE-CLAMPS AND SHIFTS, because an offset clamped against range that is not
// there is an offset the person cannot undo: the content sits away from the
// corner with nothing to scroll it back by, and the bar over it is drawn on
// slack the content does not need. Parents first, as arrange goes, so that a
// parent's shift carries its descendants and a nested area's own re-clamp adds
// to it. A container whose offset did not move is not touched, and a tree with
// no wrapping container in it comes out of remeasure to the bit as it went in —
// nothing between the passes changed a leaf's content, so the arithmetic is the
// same arithmetic.
//
// A WRAPPING COLUMN'S OWN REVISIT, below, IS THIS SAME MOVE MADE EARLY, so the
// sweep finds nothing left to do on the column itself and everything left to do
// above it.
//
// A WRAPPING COLUMN IS THE ONE PLACE A PASS REVISITS THE OTHER AXIS. Its length
// is Y, so it breaks in the Y pass, after its children were placed on X as one
// line; breaking then moves each child and its whole subtree across to its line,
// and never resizes one, because a width that waited on a height is what the
// order rules out. A subtree is consecutive in the array, so the move is a flat
// loop — but a node under several wrapping columns is moved once for each, which
// is the one sweep here that is not strictly linear in the node count.
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
// which is the padded rectangle and not the outer one. So a line's `count`, the
// children of that line still in the run, is what every piece of run arithmetic
// here is written against, and the total child count is not kept at all. The
// consequence
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
// the direction handling and why each arrange has one expression for a child's
// corner rather than two. A reader who knows this engine will expect a
// flip here, because the world is +Y up; the surface being laid out is a flat
// thing's own parameter space and voe_render_element's header is where the one
// sign that reconciles the two is written down. Putting a second one here is the
// way this file goes wrong, and it would pass a symmetrical test.
//
// PADDING IS SUBTRACTED IN ONE PLACE, inner_min and inner_size, AND ADDED IN
// ONE PLACE, measured_set. Counting it twice is the classic off-by-pad and the
// only guard against it is that there is one line each way. Four numbers give it
// four ways to happen, so both directions go through pad_axis and pad_near and
// neither names a side of the struct: a pass asks for the two sides on an axis,
// or for the near side of one, and never for `left` as such.
//
// A CLIP AND AN OFFSET ARE THE LAST THINGS ADDED, AND NEITHER IS A NEW PASS.
// The offset is clamped inside arrange, at the one moment a container's measure
// and its rectangle on an axis are both known and its children are not yet
// placed — which is where its children's start is worked out, so moving them is
// a subtraction from that start and nothing else. Their descendants follow for
// free, being placed from their parent's rectangle later in the same sweep. The
// one exception is the one above: a wrapping column only knows its width's
// measure once it breaks in the Y pass, so it clamps X again there and moves
// what it had placed, rather than show an offset clamped against one line.
//
// THE CLIP IS A FOURTH FLAT SWEEP, forwards and after both axes: a parent's
// limit is known before its children's, so each child's limit is its parent's,
// narrowed to the parent's visible rectangle on the axes the parent clips. It
// narrows only what is seen; no rectangle moves for it, so nothing that measures
// or arranges reads it.
//
// THE TREE AND THE CONTEXT ARE DECLARED IN src/context.h AND NOT HERE, because
// widgets.c is the other half of the same frame and reads the same rectangles.
// This file still owns every one of the layout fields and writes all of them;
// what it does not own it does not touch. The three calls it makes into widgets.c
// are at the bottom of context.h: one when the context is made, and two at frame
// boundaries, because a widget pass that ran anywhere but after arrange would be
// testing a click against rectangles that do not exist yet.
#include "context.h"

#include <base/assert.h>
#include <base/report.h>

#include <float.h>

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

// A container's content box on one axis, inside its padding, from the rectangle
// arrange has already given it. The one place padding is subtracted.
static float inner_min(const struct voe_ui_node_record *c, bool y)
{
	return axis(c->rect.min, y) + pad_near(c->pad, y);
}

static float inner_size(const struct voe_ui_node_record *c, bool y)
{
	return axis(c->rect.size, y) - pad_axis(c->pad, y);
}

// What a container's children came to on one axis, with its padding. The one
// place padding is added — by measure, and by a wrapping container revising what
// it measured to once it has broken.
static void measured_set(struct voe_ui_node_record *c, bool y, float children)
{
	axis_set(&c->content_natural, y, children + pad_axis(c->pad, y));
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
			VOE_BASE_ERROR("ui",
				       "node %u refused, this context was "
				       "created with room for %u",
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

// One axis of it. `y_along` is the parent's flow and `y` the axis being
// measured, so the declaration read is `along` exactly when the two agree.
static void measure_natural(struct voe_ui_node_record *n, bool y_along, bool y)
{
	voe_ui_size size = y == y_along ? n->size.along : n->size.across;

	axis_set(&n->natural, y, declared(size, axis(n->content_natural, y)));
}

// The first child at or after `child` that is in the run, or VOE_UI_NODE_NONE.
static uint32_t in_flow(const struct voe_ui_node_record *nodes, uint32_t child)
{
	while (child != VOE_UI_NODE_NONE && nodes[child].anchor.anchored)
		child = nodes[child].next_sibling;
	return child;
}

// One line of a container's run: its in-flow children from `first` up to and
// not including `next`, how long their natural sizes and gaps come to along the
// flow, and how thick the thickest of them is across it.
struct voe_ui_line {
	uint32_t first;
	uint32_t next;
	uint32_t count;
	float length;
	float thickness;
};

// Reads the line that begins at `first`, false when `first` is
// VOE_UI_NODE_NONE and there is no line left. A line is the in-flow children
// that share a `line` number, and they are consecutive because breaking only
// ever counts upwards.
//
// The length is summed before its gaps are added, in that order, because that
// is the order measure has always used and one line has to come out at today's
// number exactly and not to within rounding.
static bool line_read(const struct voe_ui_node_record *nodes, uint32_t first,
		      float gap, bool y, struct voe_ui_line *line)
{
	uint32_t child;

	if (first == VOE_UI_NODE_NONE)
		return false;

	*line = (struct voe_ui_line){ .first = first };
	for (child = first; child != VOE_UI_NODE_NONE &&
			    nodes[child].line == nodes[first].line;
	     child = in_flow(nodes, nodes[child].next_sibling)) {
		const struct voe_ui_node_record *n = &nodes[child];

		line->count++;
		line->length += axis(n->natural, y);
		if (axis(n->natural, !y) > line->thickness)
			line->thickness = axis(n->natural, !y);
	}
	if (line->count > 1)
		line->length += gap * (float)(line->count - 1);
	line->next = child;

	return true;
}

// What a container's run comes to along its flow — its longest line — or, with
// `across` set, across it: every line's thickness and the gaps between them.
// Neither counts the padding. With one line these are the sum and the largest,
// which is all they were before a run could wrap.
static float run_extent(const struct voe_ui_node_record *nodes,
			const struct voe_ui_node_record *c, bool across)
{
	bool y = !c->row;
	struct voe_ui_line line;
	float extent = 0.0f;
	uint32_t lines = 0;
	uint32_t child;

	for (child = in_flow(nodes, c->first_child);
	     line_read(nodes, child, c->gap, y, &line); child = line.next) {
		lines++;
		if (across)
			extent += line.thickness;
		else if (line.length > extent)
			extent = line.length;
	}
	if (across && lines > 1)
		extent += c->gap * (float)(lines - 1);

	return extent;
}

// One axis of a container: its children's natural sizes on that axis, and its
// own content from them and its padding. Its children are already measured on
// this axis: they sit at higher indices and the sweep runs backwards.
static void measure_container(struct voe_ui_node_record *nodes, uint32_t index,
			      bool axis_y)
{
	struct voe_ui_node_record *c = &nodes[index];
	bool y = !c->row;
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
		// run_extent never sees one.
		measure_natural(n, n->anchor.anchored ? false : y, axis_y);
	}

	measured_set(c, axis_y, run_extent(nodes, c, axis_y != y));
}

static void measure(struct voe_ui_node_record *nodes, uint32_t count,
		    bool axis_y)
{
	uint32_t i = count;

	while (i-- > 0) {
		if (nodes[i].leaf)
			axis_set(&nodes[i].content_natural, axis_y,
				 axis(nodes[i].content, axis_y));
		else
			measure_container(nodes, i, axis_y);
	}
}

static float along_size(const struct voe_ui_node_record *n, bool y, float share)
{
	if (n->size.along.kind == VOE_UI_SIZE_GROW)
		return n->size.along.value * share;
	return axis(n->natural, y);
}

// The offset a container moves its children by on one axis, clamped to between
// nought and what its content measured past its rectangle, remembered in
// `scrolled` for the accessor and returned. Called where both numbers are final
// and before a child is placed. On a VISIBLE axis the request is nought, which
// begin asserts, so this is nought there too and needs no case of its own — and
// a container that never scrolls places its children at `start - 0`, which is
// `start` to the bit.
static float scroll_clamp(struct voe_ui_node_record *c, bool y)
{
	float range = axis(c->content_natural, y) - axis(c->rect.size, y);
	float offset = axis(c->scroll, y);

	if (offset > range)
		offset = range;
	if (offset < 0.0f)
		offset = 0.0f;

	axis_set(&c->scrolled, y, offset);
	return offset;
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

// Numbers the in-flow children of a wrapping container into lines against its
// inner length along the flow, which arrange has just come to.
//
// A RUN THAT FITS IS ONE LINE AND IS NOT WALKED AT ALL. The comparison is
// against what measure made of the run, which is the number a natural length
// was copied from, so a container at its natural length never breaks — rather
// than breaking on a rounding when the same lengths are summed a second time
// in another order.
static void break_lines(struct voe_ui_node_record *nodes,
			const struct voe_ui_node_record *c, bool y, float inner)
{
	bool fits = axis(c->rect.size, y) >= axis(c->content_natural, y);
	uint32_t number = 0;
	uint32_t on_line = 0;
	float length = 0.0f;
	uint32_t child;

	for (child = in_flow(nodes, c->first_child); child != VOE_UI_NODE_NONE;
	     child = in_flow(nodes, nodes[child].next_sibling)) {
		struct voe_ui_node_record *n = &nodes[child];
		float size = axis(n->natural, y);

		// The first child on a line never breaks, so a child longer
		// than the whole line has that line to itself and sticks out.
		if (!fits && on_line > 0 && length + c->gap + size > inner) {
			number++;
			on_line = 0;
		}

		length = on_line > 0 ? length + c->gap + size : size;
		on_line++;
		n->line = number;
	}
}

// One line laid out along the flow as a whole run always was: grow children
// share what the line leaves, and `along` distributes what is left after them.
static void arrange_line(struct voe_ui_node_record *nodes,
			 const struct voe_ui_node_record *c,
			 const struct voe_ui_line *line, bool y, float start,
			 float inner)
{
	float gaps = line->count > 1 ? c->gap * (float)(line->count - 1) : 0.0f;
	float taken = 0.0f;
	float weight = 0.0f;
	float leftover;
	float share = 0.0f;
	float used;
	float free_space;
	float offset = 0.0f;
	float extra = 0.0f;
	uint32_t child;

	for (child = line->first; child != line->next;
	     child = in_flow(nodes, nodes[child].next_sibling)) {
		const struct voe_ui_node_record *n = &nodes[child];

		if (n->size.along.kind == VOE_UI_SIZE_GROW)
			weight += n->size.along.value;
		else
			taken += axis(n->natural, y);
	}

	// What the grow children divide between them. A line that already
	// overflows has nothing left to share, and a grow child in it gets
	// nought rather than a negative size.
	leftover = inner - taken - gaps;
	if (weight > 0.0f && leftover > 0.0f)
		share = leftover / weight;

	// What is left after the grow children have taken theirs is what `along`
	// distributes. With a grow child present that is nought, which is why
	// grow and a distribution do not fight.
	used = taken + gaps;
	for (child = line->first; child != line->next;
	     child = in_flow(nodes, nodes[child].next_sibling)) {
		const struct voe_ui_node_record *n = &nodes[child];

		if (n->size.along.kind == VOE_UI_SIZE_GROW)
			used += along_size(n, y, share);
	}
	free_space = inner - used;

	switch (c->along) {
	case VOE_UI_ALONG_START:
		break;
	case VOE_UI_ALONG_CENTER:
		// Not clamped when the line overflows: a line wider than its
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
		// guard that says so: with a single child, or with a line that
		// already overflows, there is no free space to hand out and both
		// are START.
		if (line->count > 1 && free_space > 0.0f) {
			if (c->along == VOE_UI_ALONG_SPREAD) {
				// Between the children and none at the ends.
				extra = free_space / (float)(line->count - 1);
			} else {
				// Every gap the same, the ends included, so one
				// more gap than SPREAD has — and the first of
				// them goes in front of the line.
				extra = free_space / (float)(line->count + 1);
				offset = extra;
			}
		}
		break;
	}

	for (child = line->first; child != line->next;
	     child = in_flow(nodes, nodes[child].next_sibling)) {
		struct voe_ui_node_record *n = &nodes[child];
		float size = along_size(n, y, share);

		axis_set(&n->rect.size, y, size);
		axis_set(&n->rect.min, y, start + offset);

		offset += size + c->gap + extra;
	}
}

// Moves a node and everything inside it along one axis. A subtree is that many
// consecutive entries of the pre-order array, so this is a flat loop too.
static void shift_subtree(struct voe_ui_node_record *nodes, uint32_t node,
			  bool y, float by)
{
	uint32_t end = node + nodes[node].subtree;
	uint32_t i;

	for (i = node; i < end; i++)
		axis_set(&nodes[i].rect.min, y,
			 axis(nodes[i].rect.min, y) + by);
}

// The container's rectangle is known on the axis across its flow; this places
// its in-flow children on it, line by line.
//
// ONE LINE IS TODAY'S LAYOUT EXACTLY, SO ONE LINE IS THE INNER LENGTH. A single
// line's thickness is the container's inner size across, whatever its children
// came to — FILL stretched to it and CENTER centred on it even when a child
// overflows it, as they always were. Several lines are as thick as their
// thickest child, and share whatever the container has left over across once
// they and their gaps are counted, each growing by the same amount.
//
// `settled` IS A WRAPPING COLUMN'S SECOND VISIT. Its width, and its children's,
// were settled in the X pass while it was still one line; it breaks in the Y
// pass, and this is then called again to MOVE each child, and everything inside
// it, to the line it landed on — never to resize one. The reading was chosen by
// the principal when card 072 asked: FILL stretching a child to a line only
// known after its height would be a width that needed a height, which is the
// one thing the axis order exists to rule out. So in a column broken into
// several lines, a FILL child keeps its one-line width and sits at its line's
// start.
static void arrange_across(struct voe_ui_node_record *nodes,
			   const struct voe_ui_node_record *c, bool settled)
{
	bool y = !c->row;
	float inner = inner_size(c, !y);
	// Minus the offset, clamped by the caller before this was called.
	float at = inner_min(c, !y) - axis(c->scrolled, !y);
	struct voe_ui_line line;
	float total = 0.0f;
	float spare;
	uint32_t lines = 0;
	uint32_t child;

	for (child = in_flow(nodes, c->first_child);
	     line_read(nodes, child, c->gap, y, &line); child = line.next) {
		lines++;
		total += line.thickness;
	}
	if (lines > 1)
		total += c->gap * (float)(lines - 1);
	// Shared only when there is some. Lines that already overflow across
	// keep their thicknesses and stick out, as everything here does.
	spare = lines > 0 && inner > total ? (inner - total) / (float)lines
					   : 0.0f;

	for (child = in_flow(nodes, c->first_child);
	     line_read(nodes, child, c->gap, y, &line); child = line.next) {
		float thickness = lines > 1 ? line.thickness + spare : inner;
		uint32_t in_line;

		for (in_line = line.first; in_line != line.next;
		     in_line = in_flow(nodes, nodes[in_line].next_sibling)) {
			struct voe_ui_node_record *n = &nodes[in_line];
			float size;
			float offset = 0.0f;

			// A child's own fixed size across the flow beats the
			// container's FILL: the more specific statement wins.
			if (settled)
				size = axis(n->rect.size, !y);
			else if (n->size.across.kind == VOE_UI_SIZE_FIXED)
				size = n->size.across.value;
			else if (c->across == VOE_UI_ACROSS_FILL)
				size = thickness;
			else
				size = axis(n->natural, !y);

			switch (c->across) {
			case VOE_UI_ACROSS_START:
				break;
			case VOE_UI_ACROSS_CENTER:
				offset = (thickness - size) * 0.5f;
				break;
			case VOE_UI_ACROSS_END:
				offset = thickness - size;
				break;
			case VOE_UI_ACROSS_FILL:
				// Filled, or fixed and so left at the start of
				// the line.
				break;
			}

			if (settled) {
				shift_subtree(nodes, in_line, !y,
					      at + offset -
						      axis(n->rect.min, !y));
			} else {
				axis_set(&n->rect.size, !y, size);
				axis_set(&n->rect.min, !y, at + offset);
			}
		}

		at += thickness + c->gap;
	}
}

// The container's rectangle is known on the axis along its flow; this breaks
// its run into lines if it wraps and lays each of them out along that axis.
static void arrange_along(struct voe_ui_node_record *nodes, uint32_t index)
{
	struct voe_ui_node_record *c = &nodes[index];
	bool y = !c->row;
	float inner = inner_size(c, y);
	// The axis begins at the padded box's own corner and runs away from it,
	// rightwards or downwards, so an offset is added on either. There is no
	// minus sign in front of a position here and there is not meant to be;
	// the one below is a scroll offset taken off the start, on either axis
	// alike, and not a flip.
	float start = inner_min(c, y);
	struct voe_ui_line line;
	float offset;
	float offset_across;
	uint32_t child;

	if (c->wrap) {
		break_lines(nodes, c, y, inner);
		// What it measured to is now what it wrapped to: the longest line
		// along the flow, and every line and its gaps across it. Along is
		// the flow's own axis, which measure saw as one line. Before the
		// offset is clamped, which is against this number.
		measured_set(c, y, run_extent(nodes, c, false));
	}

	offset = scroll_clamp(c, y);
	for (child = in_flow(nodes, c->first_child);
	     line_read(nodes, child, c->gap, y, &line); child = line.next)
		arrange_line(nodes, c, &line, y, start - offset, inner);

	// A COLUMN BREAKS IN THE Y PASS, AFTER ITS X WAS SETTLED AS ONE LINE, so
	// its across has been measured and arranged already and both are
	// revisited here: the measure for the accessor, the arrangement to move
	// the extra lines out to the right of its rectangle. A row's across is
	// Y, which has not been measured yet and reads the lines when it is.
	if (!c->wrap || c->row)
		return;

	measured_set(c, !y, run_extent(nodes, c, true));

	// AND ITS X OFFSET IS CLAMPED AGAIN, against the width it has only now
	// measured to. The X pass clamped against one line, which is too little
	// range as soon as there are two; the in-flow children are moved by the
	// arrangement below, and the anchored ones by the difference.
	offset_across = axis(c->scrolled, !y);
	(void)scroll_clamp(c, !y);
	arrange_across(nodes, c, true);
	for (child = c->first_child; child != VOE_UI_NODE_NONE;
	     child = nodes[child].next_sibling)
		if (nodes[child].anchor.anchored)
			shift_subtree(nodes, child, !y,
				      offset_across - axis(c->scrolled, !y));
}

// One axis of every container, parents first. Along a container's flow the run
// is broken and laid out; across it the lines are stacked; and the anchored
// children are placed on that axis against the content box in both cases, after
// their in-flow siblings.
static void arrange(struct voe_ui_node_record *nodes, uint32_t count,
		    bool axis_y)
{
	uint32_t i;

	for (i = 0; i < count; i++) {
		struct voe_ui_node_record *c = &nodes[i];
		uint32_t child;

		if (c->leaf)
			continue;

		if (axis_y == !c->row) {
			arrange_along(nodes, i);
		} else {
			(void)scroll_clamp(c, axis_y);
			arrange_across(nodes, c, false);
		}

		// AND THE ANCHORED CHILDREN AFTERWARDS, IN CALL ORDER AMONG
		// THEMSELVES. After, and not before, because submission order is
		// paint order on the element path: an anchored panel has to be
		// handed over later than the siblings it floats above or it goes
		// underneath them, and that is invisible until something draws.
		// paint_order is what carries this into emission; this loop is
		// only where the rectangles come from.
		//
		// Against the content box — inside the padding — and each axis
		// worked out on its own pass, so the sixteen combinations need no
		// cases of their own. Moved by the container's offset like every
		// in-flow child (ADR-0153).
		for (child = c->first_child; child != VOE_UI_NODE_NONE;
		     child = nodes[child].next_sibling) {
			struct voe_ui_node_record *n = &nodes[child];

			if (!n->anchor.anchored)
				continue;

			anchor_axis(n, axis_y ? n->anchor.y : n->anchor.x,
				    axis_y,
				    inner_min(c, axis_y) -
					    axis(c->scrolled, axis_y),
				    inner_size(c, axis_y));
		}
	}
}

// One axis measured again, now that every wrap has been decided, and every
// container's offset re-clamped against what came out. See this file's header
// for why a wrapping container's ancestors need it and why nothing is resized
// or re-arranged here.
//
// measure reads the `line` numbers arrange wrote, so a wrapping container comes
// out at its longest line and its lines' thicknesses rather than at the one run
// it was first measured as, and every ancestor reads that through `natural`.
// Then forwards, parents before children: where the re-clamp moves a
// container's offset, its children — anchored and in flow alike — move by the
// difference, which is the shift arrange_along already makes for a wrapping
// column's across axis.
static void remeasure(voe_ui_context *ui, bool axis_y)
{
	struct voe_ui_node_record *nodes = ui->nodes;
	uint32_t i;

	measure(nodes, ui->count, axis_y);

	for (i = 0; i < ui->count; i++) {
		struct voe_ui_node_record *c = &nodes[i];
		float was;
		float now;
		uint32_t child;

		if (c->leaf)
			continue;

		was = axis(c->scrolled, axis_y);
		now = scroll_clamp(c, axis_y);
		if (now == was)
			continue;

		for (child = c->first_child; child != VOE_UI_NODE_NONE;
		     child = nodes[child].next_sibling)
			shift_subtree(nodes, child, axis_y, was - now);
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

// One axis of a rectangle narrowed to an interval. Only a side that reaches past
// the interval is moved, so a rectangle already inside it keeps its own min and
// size exactly rather than a size re-derived from a sum that may round.
static void limit_axis(voe_ui_rect *rect, bool y, float low, float high)
{
	float min = axis(rect->min, y);
	float size = axis(rect->size, y);

	if (min < low) {
		size -= low - min;
		min = low;
	}
	if (min + size > high)
		size = high - min;
	// Nothing left is nought and not a negative size, for the reason an
	// anchored FILL's crossed offsets are.
	if (size < 0.0f)
		size = 0.0f;

	axis_set(&rect->min, y, min);
	axis_set(&rect->size, y, size);
}

voe_ui_rect voe_ui_limit(const voe_ui_context *ui, uint32_t node,
			 voe_ui_rect rect)
{
	const struct voe_ui_node_record *n;

	VOE_BASE_ASSERT(ui != NULL, "limiting a rectangle on no context");
	VOE_BASE_ASSERT(node < ui->count,
			"limiting a rectangle by a node this frame never made");

	n = &ui->nodes[node];
	limit_axis(&rect, false, n->limit_min.x, n->limit_max.x);
	limit_axis(&rect, true, n->limit_min.y, n->limit_max.y);

	return rect;
}

// What a clipping container hands its children on one axis: its own visible
// interval where it clips, and whatever it was handed where it does not.
static void limit_hand_down(const struct voe_ui_node_record *c,
			    struct voe_ui_node_record *n, bool y, bool clips)
{
	if (clips) {
		axis_set(&n->limit_min, y, axis(c->visible.min, y));
		axis_set(&n->limit_max, y,
			 axis(c->visible.min, y) + axis(c->visible.size, y));
	} else {
		axis_set(&n->limit_min, y, axis(c->limit_min, y));
		axis_set(&n->limit_max, y, axis(c->limit_max, y));
	}
}

// Every node's limit and visible rectangle, parents first. A parent's visible
// rectangle is already inside its own limit, so handing it down on a clipping
// axis is the intersection of every clip above, and nested clips need nothing
// more.
static void clip(voe_ui_context *ui)
{
	struct voe_ui_node_record *nodes = ui->nodes;

	// The root is limited by nothing, and so is visible whole.
	nodes[0].limit_min = (voe_math_float2){ -FLT_MAX, -FLT_MAX };
	nodes[0].limit_max = (voe_math_float2){ FLT_MAX, FLT_MAX };
	nodes[0].visible = nodes[0].rect;

	for (uint32_t i = 0; i < ui->count; i++) {
		const struct voe_ui_node_record *c = &nodes[i];
		uint32_t child;

		for (child = c->first_child; child != VOE_UI_NODE_NONE;
		     child = nodes[child].next_sibling) {
			struct voe_ui_node_record *n = &nodes[child];

			limit_hand_down(c, n, false,
					c->overflow.x == VOE_UI_OVERFLOW_CLIP);
			limit_hand_down(c, n, true,
					c->overflow.y == VOE_UI_OVERFLOW_CLIP);
			n->visible = voe_ui_limit(ui, child, n->rect);
		}
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
	// The one field that is not nought to begin with. See widgets.h.
	ui->text_scale = 1.0f;
	voe_ui_widgets_init(ui, arena);

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
	VOE_BASE_ASSERT((container.overflow.x == VOE_UI_OVERFLOW_CLIP ||
			 container.scroll.x == 0.0f) &&
				(container.overflow.y == VOE_UI_OVERFLOW_CLIP ||
				 container.scroll.y == 0.0f),
			"a scroll offset on an axis that does not clip; content "
			"moved on a VISIBLE axis would stick out where it was "
			"scrolled to — set overflow to CLIP on that axis");

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
	n->wrap = container.wrap;
	n->overflow = container.overflow;
	n->scroll = container.scroll;

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
	n->wrap = false;
	n->overflow = (voe_ui_overflow){ 0 };
	n->scroll = (voe_math_float2){ 0.0f, 0.0f };

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
	ok = !ui->overrun && !ui->collision && !ui->scroll_overrun;
	// Paint order is the tree's shape and not the arrangement's, so it is
	// worked out even for a refused frame — which keeps the accessor's
	// answer a real position in every frame that built any tree at all,
	// rather than one that is only meaningful when `ok`.
	if (ui->count > 0)
		paint_order(ui->nodes, ui->count, ui->order);

	// X for the whole tree, then Y for the whole tree, and never the other
	// way round: a wrapping row's height is its lines, and its lines are
	// only known once its width has been arranged. Paint order is worked
	// out above and before either, and a wrapping column leans on it.
	for (int pass = 0; ok && ui->count > 0 && pass < 2; pass++) {
		bool axis_y = pass == 1;

		measure(ui->nodes, ui->count, axis_y);

		// The root is measured against its own flow, because it has no
		// parent whose flow to be measured against, and it sits at the
		// origin.
		root = &ui->nodes[0];
		measure_natural(root, !root->row, axis_y);
		axis_set(&root->rect.min, axis_y, 0.0f);
		axis_set(&root->rect.size, axis_y, axis(root->natural, axis_y));

		arrange(ui->nodes, ui->count, axis_y);
	}

	// And then each axis measured again, because a wrap is decided in
	// arrange and so lands after the measure that fed the wrapping
	// container's ancestors. Measure only: no rectangle moves for it but
	// the ones a re-clamped offset shifts.
	for (int pass = 0; ok && ui->count > 0 && pass < 2; pass++)
		remeasure(ui, pass == 1);

	// After both axes, because a clip is a rectangle and a rectangle is not
	// known until both are arranged.
	if (ok && ui->count > 0)
		clip(ui);

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

voe_ui_rect voe_ui_node_visible(const voe_ui_context *ui, voe_ui_node node)
{
	VOE_BASE_ASSERT(ui != NULL, "reading a visible rectangle from no context");
	VOE_BASE_ASSERT(ui->state == VOE_UI_LAID_OUT,
			"reading a visible rectangle before the frame has ended");
	VOE_BASE_ASSERT(node != VOE_UI_NODE_NONE,
			"reading a visible rectangle through a node the frame "
			"had no room for");
	VOE_BASE_ASSERT(node < ui->count, "reading a visible rectangle through "
					  "a node this frame never made");

	return ui->nodes[node].visible;
}

voe_math_float2 voe_ui_node_scroll(const voe_ui_context *ui, voe_ui_node node)
{
	VOE_BASE_ASSERT(ui != NULL, "reading a scroll offset from no context");
	VOE_BASE_ASSERT(ui->state == VOE_UI_LAID_OUT,
			"reading a scroll offset before the frame has ended");
	VOE_BASE_ASSERT(node != VOE_UI_NODE_NONE,
			"reading a scroll offset through a node the frame had "
			"no room for");
	VOE_BASE_ASSERT(node < ui->count, "reading a scroll offset through a "
					  "node this frame never made");

	return ui->nodes[node].scrolled;
}
