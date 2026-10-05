// The borders' hover, press, drag, double-click and release, each frame
// against the tree laid out below the bar. A border's band is found by walking
// the tree's nodes once, at most VOE_EDITOR_DOCK_NODES of them.
#include "resize.h"

#include <base/assert.h>

#include <math.h>

#define NO_BORDER UINT32_MAX
#define BAR_BORDER VOE_EDITOR_DOCK_NODES

static bool border_is_row(const voe_editor_dock_tree *tree, uint32_t border)
{
	VOE_BASE_ASSERT(tree != NULL, "asking a border's axis of no tree");
	VOE_BASE_ASSERT(border == BAR_BORDER || border < tree->count,
			"asking the axis of a border that is not one");

	return border != BAR_BORDER &&
	       tree->nodes[border].axis == VOE_EDITOR_DOCK_ROW;
}

// Where `at` is along the border's axis: across a ROW's seam, down otherwise.
static float along_border(const voe_editor_dock_tree *tree, uint32_t border,
			  voe_math_float2 at)
{
	VOE_BASE_ASSERT(tree != NULL, "measuring along a border of no tree");
	VOE_BASE_ASSERT(border != NO_BORDER, "measuring along no border");

	return border_is_row(tree, border) ? at.x : at.y;
}

// The border's edge along its axis: the seam's start, or the bar's height.
static float border_edge(const voe_editor_dock_tree *tree,
			 const voe_editor_dock_arrangement *places,
			 uint32_t border, float high)
{
	VOE_BASE_ASSERT(places != NULL, "finding an edge in no arrangement");
	VOE_BASE_ASSERT(border != NO_BORDER, "finding the edge of no border");

	if (border == BAR_BORDER)
		return high;
	return border_is_row(tree, border) ? places->nodes[border].seam.min.x :
					     places->nodes[border].seam.min.y;
}

static bool band_holds(voe_ui_rect band, voe_math_float2 at)
{
	VOE_BASE_ASSERT(band.size.x >= 0.0f, "a band that is inside out");
	VOE_BASE_ASSERT(band.size.y >= 0.0f, "a band that is inside out");

	return at.x >= band.min.x && at.x <= band.min.x + band.size.x &&
	       at.y >= band.min.y && at.y <= band.min.y + band.size.y;
}

// The border whose band holds `at`: a split's seam widened by
// VOE_EDITOR_RESIZE_REACH either side along its axis, then the bar's edge.
static uint32_t border_under(const voe_editor_dock_root *root,
			     const voe_editor_dock_arrangement *places,
			     float high, voe_math_float2 at)
{
	const float reach = VOE_EDITOR_RESIZE_REACH;

	VOE_BASE_ASSERT(root != NULL, "hit-testing the borders of no root");
	VOE_BASE_ASSERT(root->tree.count <= VOE_EDITOR_DOCK_NODES,
			"a dock tree with more nodes than it can hold");

	for (uint32_t i = 0; i < root->tree.count; i++) {
		const voe_editor_dock_node *node = &root->tree.nodes[i];
		voe_ui_rect band = places->nodes[i].seam;

		if (node->kind != VOE_EDITOR_DOCK_SPLIT)
			continue;
		if (node->axis == VOE_EDITOR_DOCK_ROW) {
			band.min.x -= reach;
			band.size.x += 2.0f * reach;
		} else {
			band.min.y -= reach;
			band.size.y += 2.0f * reach;
		}
		if (band_holds(band, at))
			return i;
	}
	if (band_holds((voe_ui_rect){ .min = { 0.0f, high - reach },
				      .size = { root->size.x, 2.0f * reach } },
		       at))
		return BAR_BORDER;
	return NO_BORDER;
}

// The border's edge moved to `edge`: the split's `shown` written through
// voe_editor_dock_split_set (the second child's for HOLD_SECOND, the first's
// otherwise), or the bar's `wanted` within its least and a view's room above
// the surface's foot.
static void border_drag(voe_editor_dock_root *root, voe_editor_topbar *bar,
			const voe_editor_dock_arrangement *places,
			uint32_t border, float edge)
{
	const voe_editor_dock_place *place;
	const voe_editor_dock_node *node;
	bool row;
	float start;
	float length;

	VOE_BASE_ASSERT(root != NULL && bar != NULL, "dragging into nothing");
	VOE_BASE_ASSERT(border != NO_BORDER, "dragging no border");

	if (border == BAR_BORDER) {
		bar->wanted = fmaxf(
			fminf(fmaxf(edge, voe_editor_topbar_least(bar)),
			      root->size.y - VOE_EDITOR_DOCK_VIEW_ROOM),
			0.0f);
		return;
	}
	node = &root->tree.nodes[border];
	place = &places->nodes[border];
	row = node->axis == VOE_EDITOR_DOCK_ROW;
	start = row ? place->rect.min.x : place->rect.min.y;
	if (node->hold != VOE_EDITOR_DOCK_HOLD_SECOND)
		length = edge - start;
	else
		length = start + (row ? place->rect.size.x : place->rect.size.y) -
			 edge - (row ? place->seam.size.x : place->seam.size.y);
	voe_editor_dock_split_set(&root->tree, border, places, length);
}

// A double-click: the default tree's length and fraction for that node (the
// views back to half and half), the bar fitting.
static void border_set_back(voe_editor_dock_root *root, voe_editor_topbar *bar,
			    uint32_t border)
{
	const voe_editor_dock_tree fresh = voe_editor_dock_default();

	VOE_BASE_ASSERT(root != NULL && bar != NULL, "setting back nothing");
	VOE_BASE_ASSERT(border != NO_BORDER, "setting back no border");

	if (border == BAR_BORDER) {
		bar->wanted = 0.0f;
		return;
	}
	root->tree.nodes[border].length = fresh.nodes[border].length;
	root->tree.nodes[border].fraction = fresh.nodes[border].fraction;
}

// A press on the hovered border: the second within VOE_EDITOR_RESIZE_DOUBLE
// on the same one sets it back and ends, any other holds it.
static void border_press(voe_editor_resize *resize, voe_editor_dock_root *root,
			 voe_editor_topbar *bar, float edge, uint32_t border,
			 double now, voe_editor_resize_result *result)
{
	VOE_BASE_ASSERT(resize != NULL && result != NULL, "pressing into nothing");
	VOE_BASE_ASSERT(border != NO_BORDER, "pressing no border");

	if (resize->pressed == border &&
	    now - resize->pressed_at < VOE_EDITOR_RESIZE_DOUBLE) {
		border_set_back(root, bar, border);
		resize->pressed = NO_BORDER;
		result->ended = true;
		return;
	}
	resize->held = border;
	resize->grab = along_border(&root->tree, border, root->pointer.at) - edge;
	resize->pressed = border;
	resize->pressed_at = now;
}

voe_editor_resize_result voe_editor_resize_frame(voe_editor_resize *resize,
						 voe_editor_dock_root *root,
						 voe_editor_topbar *bar,
						 bool allowed, double now)
{
	voe_editor_resize_result result = {
		.cursor = VOE_PLATFORM_CURSOR_ARROW, .reached = NO_BORDER
	};
	voe_editor_dock_arrangement places;
	voe_ui_pointer pointer;
	float high;
	uint32_t border = NO_BORDER;

	VOE_BASE_ASSERT(resize != NULL && root != NULL && bar != NULL,
			"resizing with nothing");
	VOE_BASE_ASSERT(root->size.x > 0.0f && root->size.y > 0.0f,
			"resizing on a surface with no area");

	pointer = root->pointer;
	high = voe_editor_topbar_high(bar, root->size.y);
	voe_editor_dock_arrange(
		&root->tree,
		(voe_ui_rect){ .min = { 0.0f, high },
			       .size = { root->size.x, root->size.y - high } },
		&places);
	if (resize->held != NO_BORDER) {
		border = resize->held;
		if (pointer.down) {
			border_drag(root, bar, &places, border,
				    along_border(&root->tree, border,
						 pointer.at) -
					    resize->grab);
		} else {
			resize->held = NO_BORDER;
			result.ended = true;
		}
	} else if (allowed && pointer.over && !resize->was_down) {
		border = border_under(root, &places, high, pointer.at);
		if (border != NO_BORDER && pointer.down)
			border_press(resize, root, bar,
				     border_edge(&root->tree, &places, border,
						 high),
				     border, now, &result);
	}
	if (border != NO_BORDER) {
		result.taken = true;
		result.cursor = border_is_row(&root->tree, border) ?
					VOE_PLATFORM_CURSOR_LEFT_RIGHT :
					VOE_PLATFORM_CURSOR_UP_DOWN;
		if (border != BAR_BORDER)
			result.reached = border;
	}
	resize->was_down = pointer.down;
	return result;
}
