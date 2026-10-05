// The tree: the default one, its arrangement in millimetres, the held lengths
// and the views' share a drag writes, and the questions asked of it (which
// views it shows, whether a point is over a panel). See dock.h for why the
// layout is data rather than call order, and for what a root is; dock_walk.c
// turns what is arranged here into `ui` calls.
//
// voe_editor_dock_arrange IS THE ONE DIVISION THERE IS. A held length is the
// number a drag writes, through resize.h, clamped here to what each side needs
// (0226); the walk takes every child's size from the arrangement and resize.h
// hit-tests its seams, so neither can disagree with the other.
//
// A SEAM IS A MILLIMETRE NEITHER CHILD FILLS. It is taken off the split's
// length before it is divided, and the arrangement's `seam` is the rectangle
// resize.h hit-tests for a border; dock_walk.c draws it.
#include "dock.h"

#include <base/assert.h>

#include <math.h>

// What a subtree needs along `axis` (0226): a scene view VIEW_ROOM, another
// leaf PANEL_MIN; along a split its children's needs and the seam, a held child
// needing at least its own length; across a split the larger of the two.
static float need_along(const voe_editor_dock_tree *tree, uint32_t index,
			voe_editor_dock_axis axis, uint32_t depth)
{
	const voe_editor_dock_node *node;
	float first;
	float second;

	VOE_BASE_ASSERT(depth < VOE_EDITOR_DOCK_DEPTH,
			"a dock tree deeper than VOE_EDITOR_DOCK_DEPTH — the child indices are a cycle");
	VOE_BASE_ASSERT(index < tree->count,
			"a dock node naming a child past the end of the tree");

	node = &tree->nodes[index];
	if (node->kind == VOE_EDITOR_DOCK_LEAF)
		return node->panel == VOE_EDITOR_PANEL_SCENE_VIEW ?
			       VOE_EDITOR_DOCK_VIEW_ROOM :
			       VOE_EDITOR_DOCK_PANEL_MIN;

	first = need_along(tree, node->first, axis, depth + 1);
	second = need_along(tree, node->second, axis, depth + 1);
	if (node->axis != axis)
		return fmaxf(first, second);

	if (node->hold == VOE_EDITOR_DOCK_HOLD_FIRST)
		first = fmaxf(first, node->length);
	else if (node->hold == VOE_EDITOR_DOCK_HOLD_SECOND)
		second = fmaxf(second, node->length);
	return first + VOE_EDITOR_DOCK_SEAM + second;
}

// `length` within a place's least..most, `most` winning so the other side's
// room is kept, and never below nought.
static float clamp_to_place(const voe_editor_dock_place *place, float length)
{
	VOE_BASE_ASSERT(place != NULL, "clamping to no place");
	VOE_BASE_ASSERT(!isnan(length), "clamping a length that is not a number");

	return fmaxf(fminf(fmaxf(length, place->least), place->most), 0.0f);
}

static void arrange_node(const voe_editor_dock_tree *tree, uint32_t index,
			 voe_ui_rect rect, uint32_t depth,
			 voe_editor_dock_arrangement *out)
{
	const voe_editor_dock_node *node;
	voe_editor_dock_place *place;
	voe_ui_rect head = rect;
	voe_ui_rect tail = rect;
	bool row;
	float along;
	float share;
	float wanted;
	float first;
	bool sized_first;

	VOE_BASE_ASSERT(depth < VOE_EDITOR_DOCK_DEPTH,
			"a dock tree deeper than VOE_EDITOR_DOCK_DEPTH — the child indices are a cycle, or this is not the shallow tree a dock is");
	VOE_BASE_ASSERT(index < tree->count,
			"a dock node naming a child past the end of the tree");

	node = &tree->nodes[index];
	place = &out->nodes[index];
	place->rect = rect;
	if (node->kind == VOE_EDITOR_DOCK_LEAF)
		return;

	// The seam comes off the length before it is divided, so the two
	// children and the gap between them add up to exactly what this node
	// was given. A surface too narrow to hold the seam leaves both children
	// at nothing rather than at a negative size, which `ui` would lay out as
	// a rectangle that is inside out.
	row = node->axis == VOE_EDITOR_DOCK_ROW;
	along = row ? rect.size.x : rect.size.y;
	share = fmaxf(along - VOE_EDITOR_DOCK_SEAM, 0.0f);

	// The sized child is the held one, or the first when the split divides
	// by `fraction`; its bounds are its own need and the rest less the other
	// side's, so a scene view keeps VIEW_ROOM either side of the views' seam.
	if (node->hold == VOE_EDITOR_DOCK_HOLD_FRACTION) {
		VOE_BASE_ASSERT(node->fraction > 0.0 && node->fraction < 1.0,
				"a dock split whose fraction is not between nought and one");
		wanted = (float)((double)share * node->fraction);
	} else {
		wanted = node->length;
	}
	sized_first = node->hold != VOE_EDITOR_DOCK_HOLD_SECOND;
	place->least = need_along(tree, sized_first ? node->first : node->second,
				  node->axis, depth + 1);
	place->most = along - VOE_EDITOR_DOCK_SEAM -
		      need_along(tree, sized_first ? node->second : node->first,
				 node->axis, depth + 1);
	place->shown = clamp_to_place(place, wanted);
	first = sized_first ? place->shown : share - place->shown;

	place->seam = rect;
	if (row) {
		head.size.x = first;
		place->seam.min.x = rect.min.x + first;
		place->seam.size.x = along - share;
		tail.min.x = place->seam.min.x + place->seam.size.x;
		tail.size.x = share - first;
	} else {
		head.size.y = first;
		place->seam.min.y = rect.min.y + first;
		place->seam.size.y = along - share;
		tail.min.y = place->seam.min.y + place->seam.size.y;
		tail.size.y = share - first;
	}

	arrange_node(tree, node->first, head, depth + 1, out);
	arrange_node(tree, node->second, tail, depth + 1, out);
}

void voe_editor_dock_arrange(const voe_editor_dock_tree *tree, voe_ui_rect area,
			     voe_editor_dock_arrangement *out)
{
	VOE_BASE_ASSERT(tree != NULL, "arranging no tree");
	VOE_BASE_ASSERT(out != NULL, "arranging a tree into nothing");
	VOE_BASE_ASSERT(tree->count > 0, "arranging an empty dock tree");

	*out = (voe_editor_dock_arrangement){ 0 };
	arrange_node(tree, tree->root, area, 0, out);
}

bool voe_editor_dock_over_panel(const voe_editor_dock_root *root, float top,
				voe_editor_panel panel, voe_math_float2 pointer)
{
	voe_editor_dock_arrangement places;

	VOE_BASE_ASSERT(root != NULL, "asking where a panel is on no root");
	voe_editor_dock_arrange(
		&root->tree,
		(voe_ui_rect){ .min = { 0.0f, top },
			       .size = { root->size.x, root->size.y - top } },
		&places);
	for (uint32_t i = 0; i < root->tree.count; i++) {
		const voe_editor_dock_node *node = &root->tree.nodes[i];
		voe_ui_rect r = places.nodes[i].rect;

		if (node->kind == VOE_EDITOR_DOCK_LEAF && node->panel == panel &&
		    pointer.x >= r.min.x && pointer.x < r.min.x + r.size.x &&
		    pointer.y >= r.min.y && pointer.y < r.min.y + r.size.y)
			return true;
	}
	return false;
}

voe_editor_dock_tree voe_editor_dock_default(void)
{
	voe_editor_dock_tree tree = { 0 };

	// Three columns out of two ROW splits: the Scene list is held on the
	// left and the Inspector on the right, each at SIDE_WIDE millimetres,
	// and the views between them take whatever the window has left. The
	// left column is the Scene list over Assets, Assets held at ASSETS_TALL
	// (0279).
	tree.nodes[0] = (voe_editor_dock_node){ .kind = VOE_EDITOR_DOCK_SPLIT,
						.axis = VOE_EDITOR_DOCK_ROW,
						.hold = VOE_EDITOR_DOCK_HOLD_FIRST,
						.length = VOE_EDITOR_DOCK_SIDE_WIDE,
						.first = 1,
						.second = 2 };
	tree.nodes[1] = (voe_editor_dock_node){ .kind = VOE_EDITOR_DOCK_SPLIT,
						.axis = VOE_EDITOR_DOCK_COLUMN,
						.hold = VOE_EDITOR_DOCK_HOLD_SECOND,
						.length = VOE_EDITOR_DOCK_ASSETS_TALL,
						.first = 7,
						.second = 8 };
	tree.nodes[2] = (voe_editor_dock_node){ .kind = VOE_EDITOR_DOCK_SPLIT,
						.axis = VOE_EDITOR_DOCK_ROW,
						.hold = VOE_EDITOR_DOCK_HOLD_SECOND,
						.length = VOE_EDITOR_DOCK_SIDE_WIDE,
						.first = 3,
						.second = 6 };
	// THE TWO VIEWS ARE A COLUMN SPLIT AT A HALF, AND WHICH IS ON TOP IS THE
	// `view` ON EACH LEAF. Swap the two numbers and the pictures change
	// places, with nothing else in this folder touched — the edit a tree of
	// data exists to make that small. Its `fraction` is the views' share,
	// the number a person drags (0229).
	tree.nodes[3] = (voe_editor_dock_node){ .kind = VOE_EDITOR_DOCK_SPLIT,
						.axis = VOE_EDITOR_DOCK_COLUMN,
						.fraction = 0.5,
						.first = 4,
						.second = 5 };
	tree.nodes[4] = (voe_editor_dock_node){
		.kind = VOE_EDITOR_DOCK_LEAF,
		.panel = VOE_EDITOR_PANEL_SCENE_VIEW,
		.view = 0
	};
	tree.nodes[5] = (voe_editor_dock_node){
		.kind = VOE_EDITOR_DOCK_LEAF,
		.panel = VOE_EDITOR_PANEL_SCENE_VIEW,
		.view = 1
	};
	tree.nodes[6] = (voe_editor_dock_node){
		.kind = VOE_EDITOR_DOCK_LEAF,
		.panel = VOE_EDITOR_PANEL_INSPECTOR
	};
	tree.nodes[7] = (voe_editor_dock_node){ .kind = VOE_EDITOR_DOCK_LEAF,
						.panel = VOE_EDITOR_PANEL_SCENE };
	tree.nodes[8] = (voe_editor_dock_node){ .kind = VOE_EDITOR_DOCK_LEAF,
						.panel = VOE_EDITOR_PANEL_ASSETS };
	tree.count = 9;
	tree.root = 0;

	return tree;
}

// Every node and not a walk from the root: a leaf the root cannot reach is not
// laid out either, but a tree with one in it is a tree nobody built on purpose,
// and the walk already asserts on the shapes that would make one.
bool voe_editor_dock_shows_view(const voe_editor_dock_tree *tree, uint32_t view)
{
	VOE_BASE_ASSERT(tree != NULL, "asking no tree what it shows");

	for (uint32_t i = 0; i < tree->count; i++) {
		const voe_editor_dock_node *node = &tree->nodes[i];

		if (node->kind == VOE_EDITOR_DOCK_LEAF &&
		    node->panel == VOE_EDITOR_PANEL_SCENE_VIEW &&
		    node->view == view)
			return true;
	}

	return false;
}

// The split holding a leaf of `panel` as its held child, or UINT32_MAX. The
// held child is looked at, and a split's first child when it is one: a side
// panel is held as a leaf, or as the head of its column (the Scene list).
static uint32_t split_holding(const voe_editor_dock_tree *tree,
			      voe_editor_panel panel)
{
	for (uint32_t i = 0; i < tree->count; i++) {
		const voe_editor_dock_node *node = &tree->nodes[i];
		uint32_t held;

		if (node->kind != VOE_EDITOR_DOCK_SPLIT ||
		    node->hold == VOE_EDITOR_DOCK_HOLD_FRACTION)
			continue;
		held = node->hold == VOE_EDITOR_DOCK_HOLD_FIRST ? node->first :
								   node->second;
		if (held < tree->count &&
		    tree->nodes[held].kind == VOE_EDITOR_DOCK_SPLIT)
			held = tree->nodes[held].first;
		if (held < tree->count &&
		    tree->nodes[held].kind == VOE_EDITOR_DOCK_LEAF &&
		    tree->nodes[held].panel == panel)
			return i;
	}

	return UINT32_MAX;
}

float voe_editor_dock_panel_length(const voe_editor_dock_tree *tree,
				   voe_editor_panel panel)
{
	uint32_t split;

	VOE_BASE_ASSERT(tree != NULL, "asking no tree for a panel's length");

	split = split_holding(tree, panel);
	return split == UINT32_MAX ? 0.0f : tree->nodes[split].length;
}

void voe_editor_dock_panel_length_set(voe_editor_dock_tree *tree,
				      voe_editor_panel panel, float length)
{
	uint32_t split;

	VOE_BASE_ASSERT(tree != NULL, "setting a panel's length in no tree");

	split = split_holding(tree, panel);
	if (split != UINT32_MAX)
		tree->nodes[split].length = length;
}

// The nearest a written `fraction` comes to nought or one, so a split set by
// share never divides into a side of nothing and still passes arrange's assert.
#define FRACTION_EDGE 0.001

void voe_editor_dock_split_set(voe_editor_dock_tree *tree, uint32_t node,
			       const voe_editor_dock_arrangement *arrangement,
			       float length)
{
	const voe_editor_dock_place *place;
	voe_editor_dock_node *split;
	float divided;
	float shown;

	VOE_BASE_ASSERT(tree != NULL && arrangement != NULL,
			"setting a split's edge in no tree or from no arrangement");
	VOE_BASE_ASSERT(node < tree->count &&
				tree->nodes[node].kind == VOE_EDITOR_DOCK_SPLIT,
			"setting the edge of a node that is not a split in the tree");

	split = &tree->nodes[node];
	place = &arrangement->nodes[node];
	shown = clamp_to_place(place, length);
	if (split->hold != VOE_EDITOR_DOCK_HOLD_FRACTION) {
		split->length = shown;
		return;
	}

	// The divided length is what arrange_node divided: the node's own
	// length less the seam. None of it, and there is no share to write.
	divided = (split->axis == VOE_EDITOR_DOCK_ROW ? place->rect.size.x :
							 place->rect.size.y) -
		  VOE_EDITOR_DOCK_SEAM;
	if (divided <= 0.0f)
		return;
	split->fraction = fmin(fmax((double)shown / (double)divided,
				    FRACTION_EDGE),
			       1.0 - FRACTION_EDGE);
}

// The split whose two children are both scene-view leaves, or UINT32_MAX.
static uint32_t views_split(const voe_editor_dock_tree *tree)
{
	for (uint32_t i = 0; i < tree->count; i++) {
		const voe_editor_dock_node *node = &tree->nodes[i];

		if (node->kind == VOE_EDITOR_DOCK_SPLIT &&
		    node->first < tree->count && node->second < tree->count &&
		    tree->nodes[node->first].kind == VOE_EDITOR_DOCK_LEAF &&
		    tree->nodes[node->first].panel == VOE_EDITOR_PANEL_SCENE_VIEW &&
		    tree->nodes[node->second].kind == VOE_EDITOR_DOCK_LEAF &&
		    tree->nodes[node->second].panel == VOE_EDITOR_PANEL_SCENE_VIEW)
			return i;
	}

	return UINT32_MAX;
}

double voe_editor_dock_view_share(const voe_editor_dock_tree *tree)
{
	uint32_t split;

	VOE_BASE_ASSERT(tree != NULL, "asking no tree for the views' share");

	split = views_split(tree);
	return split == UINT32_MAX ? 0.5 : tree->nodes[split].fraction;
}

void voe_editor_dock_view_share_set(voe_editor_dock_tree *tree, double share)
{
	uint32_t split;

	VOE_BASE_ASSERT(tree != NULL, "setting the views' share in no tree");
	VOE_BASE_ASSERT(share > 0.0 && share < 1.0,
			"a views' share that is not between nought and one");

	split = views_split(tree);
	if (split != UINT32_MAX)
		tree->nodes[split].fraction = share;
}

