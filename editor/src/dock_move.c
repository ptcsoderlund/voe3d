// Moving a panel: where a dragged one would land, and the tree edits that put
// it there (0439, 0491 points 2 and 4). All of it is arithmetic on the root's
// numbers in its millimetres, `area` being the dock area there; a floating
// box is stored from `area`'s top left, as dock.h says. The caller asks
// voe_editor_dock_target_at each frame of a drag for the outline, and hands
// what it answered to voe_editor_dock_move on the release;
// voe_editor_dock_back is a double-click on a floating header.
//
// A DOCK IS A REMOVAL, A COMPACTION AND AN INSERTION. The panel's leaf is taken
// out by copying its sibling over its parent split, so whoever named the
// parent now names the sibling and no parent index is looked for. The tree is
// then compacted, so the target is found by what it is (the root, the views,
// a closable's leaf) in the indices that now hold. The new split goes in the
// target's own slot and the target moves to a fresh one, which again needs
// nobody's parent.
//
// CONSTRAINTS. The compaction is the one walk, recursive and asserted under
// VOE_EDITOR_DOCK_DEPTH like dock.c's. Finding a leaf or a parent is a scan of
// the node array, which is a walk from the root only because the tree is
// compacted after every move and the default and every tree read back are
// compact (0491 point 7); a tree with dead nodes would need a walk instead. A
// compacted tree with each panel at most once is at most 17 nodes, so the two
// an insertion adds always fit; it is asserted. `raised` grows by one per
// float; four billion moves would need it renumbered.
#include "dock.h"

#include <base/assert.h>

#include <math.h>

// How near the dock area's edge a pointer docks there, and the most a leaf's
// or the views' edge reaches in from it (0491 point 4). Millimetres.
#define DOCK_REACH 4.0f
#define EDGE_MOST 15.0f

static bool rect_holds(voe_ui_rect rect, voe_math_float2 p)
{
	return p.x >= rect.min.x && p.x < rect.min.x + rect.size.x &&
	       p.y >= rect.min.y && p.y < rect.min.y + rect.size.y;
}

static bool edge_across(voe_editor_dock_edge edge)
{
	return edge == VOE_EDITOR_DOCK_EDGE_LEFT ||
	       edge == VOE_EDITOR_DOCK_EDGE_RIGHT;
}

// The edge of `rect` nearest `p` among those `p` is within reach of: `wide`
// for the left and right, `high` for the top and bottom. A pointer outside the
// rectangle is within reach of the edge it is past. False when none is.
static bool edge_near(voe_ui_rect rect, voe_math_float2 p, float wide,
		      float high, voe_editor_dock_edge *edge)
{
	const float away[4] = {
		[VOE_EDITOR_DOCK_EDGE_LEFT] = p.x - rect.min.x,
		[VOE_EDITOR_DOCK_EDGE_RIGHT] = rect.min.x + rect.size.x - p.x,
		[VOE_EDITOR_DOCK_EDGE_TOP] = p.y - rect.min.y,
		[VOE_EDITOR_DOCK_EDGE_BOTTOM] = rect.min.y + rect.size.y - p.y,
	};
	bool found = false;

	VOE_BASE_ASSERT(edge != NULL, "finding an edge into nothing");

	for (uint32_t e = 0; e < 4; e++) {
		float reach = edge_across((voe_editor_dock_edge)e) ? wide : high;

		if (away[e] > reach || (found && away[e] >= away[*edge]))
			continue;
		*edge = (voe_editor_dock_edge)e;
		found = true;
	}
	return found;
}

// A leaf's or the views' edge: within a quarter of the side, at most EDGE_MOST.
static bool side_near(voe_ui_rect rect, voe_math_float2 p,
		      voe_editor_dock_edge *edge)
{
	return edge_near(rect, p, fminf(rect.size.x / 4.0f, EDGE_MOST),
			 fminf(rect.size.y / 4.0f, EDGE_MOST), edge);
}

// A dock target at `rect`'s `edge`, held at `box`'s size along its axis and
// outlined by the strip of `rect` that deep, at most `rect`'s extent.
static voe_editor_dock_target docked(voe_editor_dock_target_kind kind,
				     voe_editor_closable panel, voe_ui_rect rect,
				     voe_editor_dock_edge edge, voe_ui_rect box)
{
	bool across = edge_across(edge);
	float length = across ? box.size.x : box.size.y;
	voe_ui_rect strip = rect;
	float *size = across ? &strip.size.x : &strip.size.y;
	float *start = across ? &strip.min.x : &strip.min.y;

	*size = fmaxf(fminf(length, *size), 0.0f);
	if (edge == VOE_EDITOR_DOCK_EDGE_RIGHT ||
	    edge == VOE_EDITOR_DOCK_EDGE_BOTTOM)
		*start += (across ? rect.size.x : rect.size.y) - *size;

	return (voe_editor_dock_target){ .kind = kind, .panel = panel,
					 .edge = edge, .length = length,
					 .outline = strip };
}

// The leaf of `which`, or UINT32_MAX; a scan, the tree being compact.
static uint32_t leaf_of(const voe_editor_dock_tree *tree,
			voe_editor_closable which)
{
	for (uint32_t i = 0; i < tree->count; i++)
		if (tree->nodes[i].kind == VOE_EDITOR_DOCK_LEAF &&
		    voe_editor_dock_leaf_closable(&tree->nodes[i]) == which)
			return i;
	return UINT32_MAX;
}

voe_ui_rect voe_editor_dock_panel_rect(const voe_editor_dock_root *root,
				       voe_ui_rect area,
				       voe_editor_closable which)
{
	voe_editor_dock_arrangement places;
	uint32_t leaf;

	VOE_BASE_ASSERT(root != NULL, "asking where a panel is on no root");
	VOE_BASE_ASSERT(which < VOE_EDITOR_CLOSABLE_COUNT,
			"asking where a panel is that is not closable");

	if (root->floats[which].floating && !root->closed[which])
		return voe_editor_dock_float_shown(&root->floats[which], area);
	leaf = leaf_of(&root->tree, which);
	if (leaf == UINT32_MAX)
		return (voe_ui_rect){ 0 };
	// A leaf the arrangement did not lay is left zeroed there.
	voe_editor_dock_arrange(root, area, &places);
	return places.nodes[leaf].rect;
}

// Whether `p` is over a shown floating box other than `dragged`'s.
static bool over_float(const voe_editor_dock_root *root, voe_ui_rect area,
		       voe_editor_closable dragged, voe_math_float2 p)
{
	for (uint32_t c = 0; c < VOE_EDITOR_CLOSABLE_COUNT; c++)
		if (c != dragged && root->floats[c].floating &&
		    !root->closed[c] &&
		    rect_holds(voe_editor_dock_float_shown(&root->floats[c],
							   area),
			       p))
			return true;
	return false;
}

voe_editor_dock_target voe_editor_dock_target_at(
	const voe_editor_dock_root *root, voe_ui_rect area,
	voe_editor_closable dragged, voe_ui_rect box, voe_math_float2 grab,
	voe_math_float2 pointer)
{
	const voe_editor_dock_float moved = {
		.floating = true,
		.box = { .min = { pointer.x - grab.x - area.min.x,
				  pointer.y - grab.y - area.min.y },
			 .size = box.size }
	};
	voe_editor_dock_arrangement places;
	voe_editor_dock_edge edge;
	uint32_t views;

	VOE_BASE_ASSERT(root != NULL, "finding a drop target on no root");
	VOE_BASE_ASSERT(dragged < VOE_EDITOR_CLOSABLE_COUNT &&
				voe_editor_closable_panel(dragged) !=
					VOE_EDITOR_PANEL_COUNT,
			"dragging a panel that does not move");

	if (edge_near(area, pointer, DOCK_REACH, DOCK_REACH, &edge))
		return docked(VOE_EDITOR_DOCK_TARGET_AT_DOCK,
			      VOE_EDITOR_CLOSABLE_COUNT, area, edge, box);

	if (!over_float(root, area, dragged, pointer)) {
		voe_editor_dock_arrange(root, area, &places);
		for (uint32_t i = 0; i < root->tree.count; i++) {
			const voe_editor_dock_node *node = &root->tree.nodes[i];
			voe_editor_closable which;

			if (node->kind != VOE_EDITOR_DOCK_LEAF ||
			    node->panel == VOE_EDITOR_PANEL_SCENE_VIEW ||
			    !places.nodes[i].laid ||
			    !rect_holds(places.nodes[i].rect, pointer))
				continue;
			which = voe_editor_dock_leaf_closable(node);
			if (which != dragged &&
			    side_near(places.nodes[i].rect, pointer, &edge))
				return docked(VOE_EDITOR_DOCK_TARGET_AT_PANEL,
					      which, places.nodes[i].rect,
					      edge, box);
		}
		views = voe_editor_dock_views_split(&root->tree);
		if (views != UINT32_MAX && places.nodes[views].laid &&
		    rect_holds(places.nodes[views].rect, pointer) &&
		    side_near(places.nodes[views].rect, pointer, &edge))
			return docked(VOE_EDITOR_DOCK_TARGET_AT_VIEWS,
				      VOE_EDITOR_CLOSABLE_COUNT,
				      places.nodes[views].rect, edge, box);
	}

	return (voe_editor_dock_target){
		.kind = VOE_EDITOR_DOCK_TARGET_FLOAT,
		.panel = VOE_EDITOR_CLOSABLE_COUNT,
		.outline = voe_editor_dock_float_shown(&moved, area)
	};
}

// Copies `index`'s subtree of `from` into `to` in pre-order, returning where
// it went, so the root lands at nought and nothing it cannot reach is copied.
static uint32_t compact_node(const voe_editor_dock_tree *from, uint32_t index,
			     uint32_t depth, voe_editor_dock_tree *to)
{
	uint32_t at;

	VOE_BASE_ASSERT(depth < VOE_EDITOR_DOCK_DEPTH,
			"a dock tree deeper than VOE_EDITOR_DOCK_DEPTH — the child indices are a cycle");
	VOE_BASE_ASSERT(index < from->count,
			"a dock node naming a child past the end of the tree");
	VOE_BASE_ASSERT(to->count < VOE_EDITOR_DOCK_NODES,
			"a dock tree reaching more nodes than it holds");

	at = to->count++;
	to->nodes[at] = from->nodes[index];
	if (from->nodes[index].kind == VOE_EDITOR_DOCK_SPLIT) {
		to->nodes[at].first =
			compact_node(from, from->nodes[index].first, depth + 1, to);
		to->nodes[at].second = compact_node(
			from, from->nodes[index].second, depth + 1, to);
	}
	return at;
}

// The leaf at `leaf` taken out: its sibling copied over its parent split.
static void leaf_remove(voe_editor_dock_tree *tree, uint32_t leaf)
{
	VOE_BASE_ASSERT(leaf != tree->root,
			"taking out the only node of a dock tree");

	for (uint32_t i = 0; i < tree->count; i++) {
		const voe_editor_dock_node *node = &tree->nodes[i];

		if (node->kind != VOE_EDITOR_DOCK_SPLIT ||
		    (node->first != leaf && node->second != leaf))
			continue;
		tree->nodes[i] = tree->nodes[node->first == leaf ? node->second :
								   node->first];
		return;
	}
	VOE_BASE_ASSERT(false, "a dock leaf with no parent split");
}

// The node `target` names in the tree as it stands, and the edge to dock at:
// the dock's root and right edge when the views or the panel are not laid.
static uint32_t target_node(const voe_editor_dock_root *root, voe_ui_rect area,
			    const voe_editor_dock_target *target,
			    voe_editor_dock_edge *edge)
{
	voe_editor_dock_arrangement places;
	uint32_t node;

	*edge = target->edge;
	if (target->kind == VOE_EDITOR_DOCK_TARGET_AT_DOCK)
		return root->tree.root;

	VOE_BASE_ASSERT(target->kind == VOE_EDITOR_DOCK_TARGET_AT_VIEWS ||
				target->panel < VOE_EDITOR_CLOSABLE_COUNT,
			"docking at a panel that is not closable");
	node = target->kind == VOE_EDITOR_DOCK_TARGET_AT_VIEWS ?
		       voe_editor_dock_views_split(&root->tree) :
		       leaf_of(&root->tree, target->panel);
	voe_editor_dock_arrange(root, area, &places);
	if (node != UINT32_MAX && places.nodes[node].laid)
		return node;
	*edge = VOE_EDITOR_DOCK_EDGE_RIGHT;
	return root->tree.root;
}

// A new split in `place`'s slot, `leaf` held on `edge`'s side at `length` and
// what was at `place` moved to a fresh slot beside it.
static void split_insert(voe_editor_dock_tree *tree, uint32_t place,
			 voe_editor_dock_edge edge, float length,
			 voe_editor_dock_node leaf)
{
	bool head = edge == VOE_EDITOR_DOCK_EDGE_LEFT ||
		    edge == VOE_EDITOR_DOCK_EDGE_TOP;
	uint32_t moved = tree->count;
	uint32_t added = tree->count + 1;

	VOE_BASE_ASSERT(tree->count + 2 <= VOE_EDITOR_DOCK_NODES,
			"no room in the dock tree for a docked panel");
	VOE_BASE_ASSERT(place < tree->count, "docking at a node past the tree");

	tree->nodes[moved] = tree->nodes[place];
	tree->nodes[added] = leaf;
	tree->nodes[place] = (voe_editor_dock_node){
		.kind = VOE_EDITOR_DOCK_SPLIT,
		.axis = edge_across(edge) ? VOE_EDITOR_DOCK_ROW :
					    VOE_EDITOR_DOCK_COLUMN,
		.fraction = 0.5,
		.hold = head ? VOE_EDITOR_DOCK_HOLD_FIRST :
			       VOE_EDITOR_DOCK_HOLD_SECOND,
		.length = length,
		.first = head ? added : moved,
		.second = head ? moved : added
	};
	tree->count += 2;
}

void voe_editor_dock_move(voe_editor_dock_root *root, voe_ui_rect area,
			  voe_editor_closable which,
			  const voe_editor_dock_target *target)
{
	voe_editor_dock_node leaf = { .kind = VOE_EDITOR_DOCK_LEAF };
	voe_editor_dock_tree compact = { 0 };
	voe_editor_dock_edge edge;
	uint32_t top = 0;
	uint32_t at;

	VOE_BASE_ASSERT(root != NULL && target != NULL,
			"moving a panel on no root or to no target");
	VOE_BASE_ASSERT(which < VOE_EDITOR_CLOSABLE_COUNT &&
				voe_editor_closable_panel(which) !=
					VOE_EDITOR_PANEL_COUNT,
			"moving a panel that does not move");

	if (target->kind == VOE_EDITOR_DOCK_TARGET_FLOAT) {
		for (uint32_t c = 0; c < VOE_EDITOR_CLOSABLE_COUNT; c++)
			if (c != which)
				top = root->floats[c].raised > top ?
					      root->floats[c].raised :
					      top;
		VOE_BASE_ASSERT(top < UINT32_MAX,
				"a floating box raised as high as it goes");
		root->floats[which] = (voe_editor_dock_float){
			.floating = true,
			.box = { .min = { target->outline.min.x - area.min.x,
					  target->outline.min.y - area.min.y },
				 .size = target->outline.size },
			.raised = top + 1
		};
		return;
	}

	leaf.panel = voe_editor_closable_panel(which);
	at = leaf_of(&root->tree, which);
	if (at != UINT32_MAX) {
		leaf = root->tree.nodes[at];
		leaf_remove(&root->tree, at);
	}
	compact.root = compact_node(&root->tree, root->tree.root, 0, &compact);
	root->tree = compact;

	at = target_node(root, area, target, &edge);
	split_insert(&root->tree, at, edge, target->length, leaf);
	root->floats[which].floating = false;
}

void voe_editor_dock_back(voe_editor_dock_root *root, voe_ui_rect area,
			  voe_editor_closable which)
{
	voe_editor_dock_target target = {
		.kind = VOE_EDITOR_DOCK_TARGET_AT_DOCK,
		.panel = VOE_EDITOR_CLOSABLE_COUNT,
		.edge = VOE_EDITOR_DOCK_EDGE_RIGHT
	};
	voe_ui_rect box;

	VOE_BASE_ASSERT(root != NULL, "docking a panel back on no root");
	VOE_BASE_ASSERT(which < VOE_EDITOR_CLOSABLE_COUNT,
			"docking back a panel that is not closable");

	if (leaf_of(&root->tree, which) != UINT32_MAX) {
		root->floats[which].floating = false;
		return;
	}

	switch (which) {
	case VOE_EDITOR_CLOSABLE_FRAME:
	case VOE_EDITOR_CLOSABLE_LANDSCAPE:
		target = (voe_editor_dock_target){
			.kind = VOE_EDITOR_DOCK_TARGET_AT_PANEL,
			.panel = VOE_EDITOR_CLOSABLE_INSPECTOR,
			.edge = VOE_EDITOR_DOCK_EDGE_BOTTOM
		};
		break;
	case VOE_EDITOR_CLOSABLE_PROJECT:
		target = (voe_editor_dock_target){
			.kind = VOE_EDITOR_DOCK_TARGET_AT_PANEL,
			.panel = VOE_EDITOR_CLOSABLE_ASSETS,
			.edge = VOE_EDITOR_DOCK_EDGE_BOTTOM
		};
		break;
	case VOE_EDITOR_CLOSABLE_ERRORS:
		target.kind = VOE_EDITOR_DOCK_TARGET_AT_VIEWS;
		target.edge = VOE_EDITOR_DOCK_EDGE_BOTTOM;
		break;
	default:
		break;
	}

	box = voe_editor_dock_float_shown(&root->floats[which], area);
	target.length = edge_across(target.edge) ? box.size.x : box.size.y;
	voe_editor_dock_move(root, area, which, &target);
}
