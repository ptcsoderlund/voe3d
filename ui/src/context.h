// The context and the tree it holds, which is the one piece of state the two
// halves of this folder share. Internal: nothing outside `ui` sees any of it.
//
// THERE IS ONE CONTEXT AND NOT TWO, AND THAT IS WHY THIS FILE EXISTS. layout.c
// builds the tree and arranges it; widgets.c decides what a node means, what the
// pointer is doing to it and what element records come out of it. Both are the
// same frame — a widget IS a node, its rectangle IS the one layout worked out —
// so splitting the state in two would mean keeping two of them in step, which is
// the bug this avoids by not having the second one.
//
// WHICH HALF OWNS WHICH FIELD IS WRITTEN DOWN BELOW AND IS NOT A SUGGESTION. The
// tree and the frame belong to layout.c; everything from `font` down belongs to
// widgets.c. Neither writes the other's, and the two entry points widgets.c
// offers layout.c are at the bottom of this file — layout.c's voe_ui_frame_end
// calls them once, after arrange, which is the only order in which a hit test
// can be against this frame's rectangles rather than last frame's.
#pragma once

#include <ui/layout.h>
#include <ui/widgets.h>

#include <math/float4.h>
#include <render/device.h>
#include <text/font.h>

#include <stdint.h>

// What a node is to widgets.c. VOE_UI_WIDGET_NONE is nought, so a plain row,
// column or box — a node no widget call made — is one by virtue of the arena
// having zeroed it.
enum voe_ui_widget {
	VOE_UI_WIDGET_NONE = 0,
	VOE_UI_WIDGET_PANEL,
	VOE_UI_WIDGET_LABEL,
	VOE_UI_WIDGET_BUTTON,
};

// What widgets.c keeps about one node, in an array indexed by the node's own
// index. A parallel array rather than fields on `struct node`, because layout.c
// has no business reading any of it.
struct voe_ui_widget_record {
	// The hashed path to this call site. Meaningless unless `keyed`.
	uint64_t key;
	// A label's string, and NULL on everything else. It is the caller's
	// pointer and is read at frame_end, so it must still be there then —
	// which a string literal and a buffer the caller owns both are.
	const char *text;
	// A panel's background. A button's is not here: which of the three it
	// is depends on the hit test, so emission picks it and nothing stores
	// it.
	voe_math_float4 colour;
	// A label's first baseline, in millimetres below its own top edge, as
	// the measurement handed it back. Kept so that emission does not walk
	// the string a second time to ask the same question.
	float baseline;
	enum voe_ui_widget kind;
	// Whether `key` was worked out for this node. Panels and buttons are
	// keyed; labels are not, having nothing to remember.
	bool keyed;
};

struct voe_ui_node_record {
	uint32_t first_child;
	uint32_t last_child;
	uint32_t next_sibling;

	bool leaf;
	// A container's direction. Unused on a leaf.
	bool row;

	// What the caller declared.
	voe_ui_sizing size;
	voe_math_float2 content;
	voe_ui_along along;
	voe_ui_across across;
	float gap;
	voe_ui_pad pad;
	// Out of the parent's run when `anchored`, and then `size` is read as X
	// and Y rather than as along and across. Never set on the root.
	voe_ui_anchor anchor;

	// What measure came to: this node's own content, in the panel's axes.
	voe_math_float2 content_natural;
	// What its parent made of that, in the panel's axes.
	voe_math_float2 natural;
	// What arrange came to.
	voe_ui_rect rect;

	// This node and everything under it, counted by the paint-order pass,
	// which is the width of the slot its subtree occupies in that order.
	uint32_t subtree;
	// Where it lands in paint order. The inverse of ui->order.
	uint32_t paint;
};

enum voe_ui_frame_state {
	VOE_UI_NOT_IN_FRAME = 0,
	VOE_UI_BUILDING,
	VOE_UI_LAID_OUT,
};

struct voe_ui_context {
	voe_ui_capacities capacities;

	// ---- layout.c's ----

	// This frame's, all of it out of the arena frame_begin was handed.
	struct voe_ui_node_record *nodes;
	// The containers begun and not yet ended, innermost last.
	uint32_t *open;
	// Node indices in paint order — see voe_ui_paint_order below. One entry
	// per node, and it is a permutation of the array rather than a subset.
	uint32_t *order;

	uint32_t count;
	uint32_t depth;
	// Begins refused for want of a node, still to be ended. They are counted
	// rather than pushed, so that a refused frame stays balanced and the
	// caller never has to check a handle before ending it.
	uint32_t refused;

	bool overrun;
	enum voe_ui_frame_state state;

	// ---- widgets.c's ----

	// Set once, and NULL until it is. A label with no font is the caller's
	// bug and asserts, because measuring is the one thing a label cannot do
	// for itself.
	const voe_text_font *font;
	float text_scale;

	// This frame's, zeroed by frame_begin so that a frame which says nothing
	// about the pointer has none rather than yesterday's.
	voe_ui_pointer pointer;
	// Last frame's button, which is what turns a level into an edge.
	bool was_down;

	// The three keys that are the whole of this folder's memory. `held`
	// survives between frames — that is the point of it — and `hovered` and
	// `fired` are worked out afresh at every frame_end.
	uint64_t hovered;
	uint64_t held;
	uint64_t fired;
	bool hovered_set;
	bool held_set;
	bool fired_set;

	// This frame's, out of the frame's arena. `widgets` is one per node.
	struct voe_ui_widget_record *widgets;
	// Every key made this frame, in an open-addressed set, so that a
	// duplicate is found at the call that made it rather than discovered as
	// two widgets behaving as one. `seen_mask + 1` entries, a power of two,
	// and `seen_used` says which of them hold anything — because a hash may
	// legitimately be nought and an empty slot must not read as one.
	uint64_t *seen;
	bool *seen_used;
	uint32_t seen_mask;
	bool collision;

	voe_render_element *elements;
	uint32_t element_count;
	bool element_overrun;
};

// The order layout arranged the tree in, which under ADR-0092 is the order the
// interface is painted in. `position` runs from nought to the node count.
//
// IT IS AN ACCESSOR AND NOT A CONVENIENCE, AND SINCE CARD 041 IT EARNS THAT. It
// was `position` itself while every child was in the flow; now that a child may
// leave it, the order is a walk of the tree in which a parent still comes before
// all of its children but a parent's ANCHORED children come after its in-flow
// ones — so a floating panel is submitted last and paints over what it floats
// above. Emission asks layout for this order and never re-derives it, which is
// why widgets.c did not have to change when the order stopped being the array's.
uint32_t voe_ui_paint_order(const voe_ui_context *ui, uint32_t position);

// The two frame boundaries, called from layout.c's voe_ui_frame_begin and
// voe_ui_frame_end and from nowhere else.
//
// `laid_out` IS FALSE FOR A FRAME THAT WAS REFUSED, and such a frame has no
// rectangles: nothing can be hovered, nothing can fire, and a held button is let
// go. That is the honest answer rather than a convenience — carrying a hover
// across a frame the caller never got to see would be state nobody wrote.
//
// Hit testing happens inside frame_end and before emission, because what a
// button looks like depends on what the pointer is doing to it.
void voe_ui_widgets_frame_begin(voe_ui_context *ui, voe_base_arena *arena);
void voe_ui_widgets_frame_end(voe_ui_context *ui, bool laid_out);
