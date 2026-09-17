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
// widgets.c. Neither writes the other's, and the three entry points widgets.c
// offers layout.c are at the bottom of this file — one at creation, for the
// scroll table that outlives every frame, and two at the frame's boundaries.
// voe_ui_frame_end calls its one after arrange, which is the only order in which
// a hit test can be against this frame's rectangles rather than last frame's.
//
// A SCROLLBAR NEEDS NO KEY OF ITS OWN AND NO TABLE BESIDE `held`. A held thumb is
// `held` set to its area's key with `held_thumb` saying which of the two bars, so
// every rule that lets go of a held widget lets go of a thumb too; the area
// cannot be held any other way, because a scroll area does not take the pointer.
#pragma once

#include <ui/layout.h>
#include <ui/theme.h>
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
	VOE_UI_WIDGET_NUMBER,
	VOE_UI_WIDGET_IMAGE,
	VOE_UI_WIDGET_SCROLL,
	VOE_UI_WIDGET_FIELD,
};

// One of a scroll area's two bars: the one that scrolls X, along the bottom, or
// the one that scrolls Y, along the right. NONE is nought, so a context that has
// never seen a scrollbar has none held and none hovered.
enum voe_ui_bar {
	VOE_UI_BAR_NONE = 0,
	VOE_UI_BAR_X,
	VOE_UI_BAR_Y,
};

// One slot of the theme push stack: a struct wrapping the one pointer, and
// not a `const voe_ui_theme **`, which rule 6 forbids (one level of
// dereference — see authoring/src/scene_read.c's `cursor` for the same move).
struct voe_ui_theme_slot {
	const voe_ui_theme *theme;
};

// A remembered offset, kept between frames under its area's key.
struct voe_ui_scroll_memory {
	uint64_t key;
	voe_math_float2 offset;
};

// A scroll area called this frame, in call order — so an area nested in another
// always comes after it, which is what finding the next area outward counts on.
struct voe_ui_scroll_area {
	uint64_t key;
	// VOE_UI_NODE_NONE only in a frame refused for want of a node.
	uint32_t node;
	voe_ui_scroll_axes axes;
	// The remembered offset handed to layout, before layout clamped it.
	voe_math_float2 handed;
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
	// A panel's own surface. A button's, a number box's and a field's are
	// not here: which theme role it is depends on the hit test, so
	// emission picks it and nothing stores it.
	voe_ui_surface surface;
	// The theme in force when this widget was made — voe_ui_theme_set's
	// argument or the nearest voe_ui_theme_push's — copied here once
	// rather than looked up again at emission, which is what makes
	// ADR-0168's "the nearest one wins" a property of WHEN a widget was
	// called and not of what has pushed or popped by the time the frame
	// ends. NULL on a node no widget call touched — a plain row, column or
	// box — which is never read, because emission never reaches one.
	//
	// DEVIATION: ADR-0168 says "every node recording the theme in force
	// when it was made". Read narrowly as every WIDGET node — the ones
	// emission can go on to draw — because layout.c's node_push makes
	// every node, including a plain row's, and giving it a theme to copy
	// would mean layout.c has to know what one is, which is exactly the
	// split this file draws between "layout.c's" fields and "widgets.c's".
	// A plain container's `kind` stays VOE_UI_WIDGET_NONE and its `theme`
	// is never asked for, so recording one there would say nothing new.
	const voe_ui_theme *theme;
	// A label's colour role: NORMAL or ACCENT. Meaningless on everything
	// else.
	voe_ui_text_role text_role;
	// A number box's value as the caller handed it in this frame, and what
	// one millimetre of horizontal drag is worth. Both are the caller's and
	// neither is remembered between frames — the value lives where the
	// caller keeps it, and this folder only ever adds a drag to the copy it
	// was given. Meaningless on everything else.
	double value;
	double per_millimetre;
	// An image's part of its texture, in the element record's own xy-wh
	// shape, and the texture id's index half. Copied into the record as they
	// stand. Meaningless on everything else.
	voe_math_float4 sheet;
	uint32_t texture;
	// A label's first baseline, in millimetres below its own top edge, as
	// the measurement handed it back. Kept so that emission does not walk
	// the string a second time to ask the same question.
	float baseline;
	enum voe_ui_widget kind;
	// Whether `key` was worked out for this node. Panels, buttons and number
	// boxes are keyed; labels and images are not, having nothing to remember.
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
	// Whether this container's children may break onto further lines. False
	// on a leaf.
	bool wrap;
	// Whether it clips, per axis, and the offset the caller asked for. Both
	// nought on a leaf.
	voe_ui_overflow overflow;
	voe_math_float2 scroll;

	// Which line of its parent's run this node landed on, counted from
	// nought. Written by the arrange along the parent's flow, and nought —
	// the arena's zero — until then, which is one line and today's layout.
	// Meaningless on an anchored child, which is in no line.
	uint32_t line;

	// What measure came to: this node's own content, in the panel's axes.
	voe_math_float2 content_natural;
	// What its parent made of that, in the panel's axes.
	voe_math_float2 natural;
	// What arrange came to.
	voe_ui_rect rect;
	// The offset arrange moved this container's children by, after clamping.
	// Nought on a VISIBLE axis and on a leaf.
	voe_math_float2 scrolled;
	// What every clipping ancestor leaves this node, as the interval each
	// axis is limited to — unbounded, ±FLT_MAX, where nothing clips — and
	// `rect` narrowed to it. Written after both axes are arranged; nought in
	// a refused frame, as `rect` is.
	voe_math_float2 limit_min;
	voe_math_float2 limit_max;
	voe_ui_rect visible;

	// This node and everything under it, counted by the paint-order pass,
	// which is the width of the slot its subtree occupies in that order. The
	// subtree is also this many consecutive entries of the array, which is
	// how a wrapping column moves a child and everything inside it at once.
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

	// THE THEME, IN TWO PARTS — see widgets.h for the whole mechanism and
	// ADR-0168 for why the nearest one wins.
	//
	// `theme` IS voe_ui_theme_set's ARGUMENT, and it survives a frame
	// exactly as `font` does: nothing here resets it at frame_begin.
	const voe_ui_theme *theme;
	// The stack voe_ui_theme_push/pop keep, out of the frame's arena and
	// bounded by the node capacity — pushing before every single node is
	// the worst case, exactly as `open`'s bound is (ui/layout.h).
	// `theme_depth` is how many of its slots hold a real push; a push
	// turned away for want of room is counted in `theme_refused` instead,
	// mirroring `open`/`refused`, so that a pop always has something to
	// undo even when the push it matches was the one refused.
	struct voe_ui_theme_slot *theme_stack;
	uint32_t theme_depth;
	uint32_t theme_refused;
	bool theme_overrun;

	// This frame's, zeroed by frame_begin so that a frame which says nothing
	// about the pointer has none rather than yesterday's.
	voe_ui_pointer pointer;
	// Last frame's button, which is what turns a level into an edge.
	bool was_down;

	// This frame's typing, zeroed by frame_begin exactly as the pointer is:
	// a frame that never calls voe_ui_keyboard_set has none.
	voe_ui_keyboard keyboard;

	// THE FOCUSED FIELD, WHICH SURVIVES BETWEEN FRAMES AS `held` DOES BUT
	// MEANS SOMETHING ELSE: `held` is a gesture in progress and is let go
	// on release; `focus` is which field the keyboard is going to and
	// stays that way across as many frames as nothing changes it. Set by a
	// press landing inside a field, cleared by a press landing anywhere
	// else, and dropped at frame_end when the field it names was not
	// called this frame — see widgets.c.
	uint64_t focus;
	bool focus_set;
	// Whether THIS frame's edit changed the focused field's text. Worked
	// out once, in field_edit, because only the one field the keyboard is
	// going to can be touched by it.
	bool field_changed;
	// The one buffer an edit is written into, because only one field can
	// be focused at a time. Read back by voe_ui_field_action through the
	// focused field's composed label, whose `text` this points the record
	// at only when `field_changed` — see field_edit in widgets.c.
	char field_buffer[VOE_UI_FIELD_CAPACITY + 1];

	// The three keys that are the whole of this folder's memory. `held`
	// survives between frames — that is the point of it — and `hovered` and
	// `fired` are worked out afresh at every frame_end.
	uint64_t hovered;
	uint64_t held;
	uint64_t fired;
	bool hovered_set;
	bool held_set;
	bool fired_set;

	// A DRAG IN PROGRESS, WHICH IS INTERACTION STATE BESIDE `held` AND NOT A
	// SECOND TABLE. Only one widget can be held, so only one can be being
	// dragged, and everything this needs to remember is about that one:
	// there is nothing to key and nothing to look up.
	//
	// The first three survive between frames, as `held` does, because a
	// drag is one gesture across many of them. The last two are this
	// frame's answer and are worked out afresh in resolve.
	//
	// Where the press happened, which the dead zone is measured from —
	// against the PRESS and never against last frame, so that a hand that
	// is already moving does not cross it one slow frame at a time.
	float number_press_x;
	// Last frame's pointer x, which is what the change is measured from
	// once the dead zone is behind us.
	float number_last_x;
	// Whether the dead zone has been left. Once it has, it does not come
	// back: dragging back towards the press goes on changing the value
	// rather than re-entering a zone that has served its purpose.
	bool number_crossed;
	// Whether the held widget is a number box. Settled when it is armed, so
	// that the drag does not have to find the node behind `held` again.
	bool held_number;
	// This frame's movement in millimetres, past the dead zone, and whether
	// there was any. In millimetres and NOT in the value's own unit, because
	// `per_millimetre` belongs to the node and this is worked out before any
	// node is in hand; voe_ui_number_action does that multiplication.
	float number_delta;
	bool number_moved;

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

	// THE SCROLL TABLE, THE ONE THING HERE THAT IS NOT A FRAME'S OR A GESTURE'S.
	// Out of the arena the context was made in, `capacities.scrolls` long,
	// and the first `scroll_remembered` entries hold last frame's areas.
	// Rewritten wholesale at every frame_end from this frame's areas, which
	// is how an area not called is dropped: it is simply not written back.
	struct voe_ui_scroll_memory *scroll_memory;
	uint32_t scroll_remembered;

	// This frame's scroll areas, out of the frame's arena, and whether one
	// more was called than there was room for. After frame_end, entry i of
	// this and of `scroll_memory` are the same area.
	struct voe_ui_scroll_area *scroll_areas;
	uint32_t scroll_count;
	bool scroll_overrun;

	// A THUMB BEING DRAGGED: `held` is its area's key, and this is which bar.
	// The offset follows the pointer from where it was pressed, so the press
	// position and the offset at the press are what is kept, and a thumb
	// dragged past the end and back comes back under the pointer.
	enum voe_ui_bar held_thumb;
	voe_math_float2 thumb_press_at;
	float thumb_press_offset;
	// This frame's hovered thumb, by index into `scroll_areas`, for its
	// colour. Worked out afresh in resolve.
	enum voe_ui_bar hovered_thumb;
	uint32_t hovered_thumb_area;
	// A press on a track this frame: which area, which bar, and towards
	// which end, -1 or +1. Applied once, after the offsets are remembered.
	enum voe_ui_bar page;
	uint32_t page_area;
	float page_towards;
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

// `rect` narrowed to what the clipping ancestors of `node` leave, axis by axis,
// size nought on an axis where nothing is left. A rectangle that is not narrowed
// comes back bit for bit, which is what keeps a tree that clips nothing emitting
// exactly what it did before clipping existed.
//
// LAYOUT'S RULE AND NOT A SECOND COPY OF IT. voe_ui_node_visible is this applied
// to the node's own rectangle; widgets.c applies it to a record whose rectangle
// is not the node's — a glyph — so the two cannot drift apart.
voe_ui_rect voe_ui_limit(const voe_ui_context *ui, uint32_t node,
			 voe_ui_rect rect);

// widgets.c's part of creating the context, called from voe_ui_context_new and
// from nowhere else: the scroll table, out of the context's own arena because it
// outlives every frame. There is no deinit; rewinding that arena frees it with
// the context.
void voe_ui_widgets_init(voe_ui_context *ui, voe_base_arena *arena);

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
