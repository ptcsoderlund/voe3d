// The widgets: what a node means and what element records come out of it. See
// include/ui/widgets.h for the promises; this file is where the decisions
// behind them are written down.
//
// WHAT IS HERE IS WHAT EVERY WIDGET SHARES: the hashed keys, the theme in
// force, the pointer and the keyboard as they are handed in, the frame's two
// boundaries, the walk that emits records in paint order, and the widgets that
// answer nothing — the panel, the label and the image. A widget that has a
// gesture of its own has a file of its own beside this one: button.c the button
// and the number box and what the pointer comes to on any of them, field.c the
// field and the one keyboard focus, scroll.c the scroll area. context.h says
// what each of them offers the others.
//
// ---- EMISSION IS A COPY AND THERE IS NOT ONE SUBTRACTION IN IT ----
//
// A node's rectangle goes into an element's `bounds` as it stands: `min` is the
// xy and `size` is the zw. That is not luck and it is not a coincidence worth
// re-deriving — ADR-0099 put layout's space and the element record's space in
// the same place, millimetres running right and down from a top-left corner, so
// that this step would have no arithmetic in it. If a minus sign ever appears
// between voe_ui_node_rect and voe_render_element.bounds, something upstream has
// drifted and the fix is upstream, not here. There is exactly one Y flip in this
// engine, it is the viewport's, and the element path's sign lives in
// voe_render_element_transform. Neither is in this file.
//
// The one place a sign does appear is inside a label, and it is not a flip: a
// font measures up from a baseline and a surface measures down from a corner, so
// the top of a glyph's box is `baseline - high.y` and its bottom is
// `baseline - low.y`. That is the same expression dev/src/elements.c uses and
// text/include/text/font.h states the convention it comes from.
//
// ---- PAINT ORDER IS SUBMISSION ORDER AND IT COMES FROM LAYOUT ----
//
// Under ADR-0092 there is no depth on the element path and nothing sorts
// anything: what is submitted later is drawn on top. So the order records come
// out in IS the interface's stacking, and this file takes that order from
// voe_ui_paint_order rather than working it out again. Today that is the node
// array's own order, which is call order, which puts a parent before every one
// of its children — a panel's background behind what is in it, a button's behind
// its label, siblings in the order they were called. Card 041 made anchored
// children come after their in-flow siblings, so that order is no longer the
// array's; taking it from the accessor is why not one line of this file had to
// move for it.
//
// ---- EVERY RECORD IS CLIPPED BY LAYOUT'S RULE, AND AN EMPTY ONE IS NOT SENT ----
//
// A record's clip is its own rectangle narrowed by voe_ui_limit — what the
// clipping ancestors leave — so a panel, a button and an image are clipped to
// their visible rectangle, and in a tree that clips nothing the clip is still the
// record's own bounds, bit for bit. A record with nothing left on either axis is
// not pushed at all and costs no capacity: a long list scrolled away is not
// hundreds of records drawing nothing.
//
// ---- IDENTITY, AND WHY IT IS A HASHED PATH ----
//
// A key is the enclosing keyed widget's key mixed with the name and the index at
// the call site, hashed with FNV-1a. Not a call-site line number: a line number
// is the same for every iteration of a loop, so a list of buttons would be one
// button, which shows up as the whole list highlighting together and reads as a
// drawing bug rather than a naming one. The parent is found by walking the open
// containers outwards, so a plain row or column between two panels changes
// nothing — only widgets that can hold state are on the path, which is what
// makes a key stable while the SHAPE of the tree is, rather than while its every
// node is.
//
// FNV-1a AND NOT SOMETHING STRONGER, because the failure this guards against is
// two call sites written with the same name, not an adversary choosing strings.
// A duplicate is caught outright by the set below rather than tolerated, so the
// hash only has to spread; and a chance collision between two DIFFERENT paths is
// one in two to the sixty-four, which would be reported as a duplicate and
// looked at, not silently shared.
#include "context.h"

#include <base/assert.h>
#include <base/report.h>
#include <math/float2.h>
#include <math/float4.h>
#include <text/font.h>
#include <text/utf8.h>

#include <string.h>

// EVERY COLOUR BELOW IS A THEME ROLE NOW (ADR-0170, ADR-0171), READ FROM
// WHICHEVER WIDGET RECORD ASKS FOR ONE — there is no constant left in this file
// for a button's, a panel's, a field's, a caret's or a scrollbar's colour. See
// surface_colour here, and button.c's state_colour, field.c's field_colour and
// scroll.c's push_scrollbar, for where each widget picks its role, and
// ui/theme.h for what the roles are and how they are derived.
//
// NOT A THEME COLOUR, AND IT STAYS A CONSTANT: an image record's colour
// multiplies the picture, and opaque white is the one value that shows the
// picture as it is. There is no tint argument, so there is nothing else it could
// be, and no theme could name one either.
static const voe_math_float4 IMAGE_AS_IT_IS = { 1.0f, 1.0f, 1.0f, 1.0f };

// A panel's and a button's hairline border, in millimetres on every side
// (ADR-0171) — the one width this folder draws a border at, so a change to how
// thick "hairline" is is one number. Not in ui/theme.h: the border's COLOUR is
// a role and the theme's to say, but how wide the rectangle it is drawn as is
// a layout fact about this folder's widgets and not a colour at all.
#define HAIRLINE_WIDE 0.3f

#define FNV_BASIS 0xcbf29ce484222325u
#define FNV_PRIME 0x00000100000001b3u

static uint64_t mix(uint64_t hash, const void *bytes, size_t count)
{
	const unsigned char *at = bytes;

	for (size_t i = 0; i < count; i++) {
		hash ^= at[i];
		hash *= FNV_PRIME;
	}
	return hash;
}

// The key of the innermost enclosing widget that has one, or the basis at the
// top. Plain rows and columns are stepped over: they hold no state, so putting
// them on the path would only make a key change when somebody wrapped a button
// in a row.
static uint64_t parent_key(const voe_ui_context *ui)
{
	uint32_t depth = ui->depth;

	while (depth-- > 0) {
		uint32_t node = ui->open[depth];

		if (ui->widgets[node].keyed)
			return ui->widgets[node].key;
	}
	return FNV_BASIS;
}

static uint64_t key_of(const voe_ui_context *ui, const char *name,
		       uint32_t index)
{
	uint64_t key = mix(parent_key(ui), name, strlen(name));

	return mix(key, &index, sizeof index);
}

static uint32_t slot_of(const voe_ui_context *ui, uint64_t key)
{
	return (uint32_t)((key >> 32) ^ key) & ui->seen_mask;
}

// True when this frame has already made this key, which is a duplicate and is
// reported. The set has at least twice as many slots as the frame has nodes, and
// a key comes from a node, so the probe always finds an empty one.
bool voe_ui_key_taken(voe_ui_context *ui, uint64_t key)
{
	uint32_t slot = slot_of(ui, key);

	while (ui->seen_used[slot]) {
		if (ui->seen[slot] == key)
			return true;
		slot = (slot + 1) & ui->seen_mask;
	}
	return false;
}

static void key_take(voe_ui_context *ui, uint64_t key)
{
	uint32_t slot = slot_of(ui, key);

	while (ui->seen_used[slot])
		slot = (slot + 1) & ui->seen_mask;
	ui->seen_used[slot] = true;
	ui->seen[slot] = key;
}

// The key for this call site, taken so that the next call to make the same one
// is a duplicate. A duplicate refuses the frame and is named on stderr once,
// because one inside a loop would otherwise print a page of it.
//
// IT IS CALLED BEFORE THE CONTAINER IS OPENED, always: the key is the ENCLOSING
// widget's mixed with this call site's, and a panel that was already open would
// find itself.
static uint64_t claim(voe_ui_context *ui, const char *name, uint32_t index)
{
	uint64_t key = key_of(ui, name, index);

	if (voe_ui_key_taken(ui, key)) {
		if (!ui->collision)
			VOE_BASE_ERROR("ui",
				       "two widgets share the key for "
				       "\"%s\"/%u under the same parent; give one of "
				       "them another name or another index",
				       name, index);
		ui->collision = true;
	} else {
		key_take(ui, key);
	}

	// Handed back even when it was a duplicate, so that the tree this
	// frame builds is still coherent for whatever else reads it. The frame
	// is refused either way.
	return key;
}

uint64_t voe_ui_widget_claim(voe_ui_context *ui, const char *name,
			     uint32_t index)
{
	return claim(ui, name, index);
}

bool voe_ui_inside(voe_ui_rect rect, voe_math_float2 at)
{
	return at.x >= rect.min.x && at.x < rect.min.x + rect.size.x &&
	       at.y >= rect.min.y && at.y < rect.min.y + rect.size.y;
}

void voe_ui_font_set(voe_ui_context *ui, const voe_text_font *font)
{
	VOE_BASE_ASSERT(ui != NULL, "giving a font to no context");
	VOE_BASE_ASSERT(font != NULL, "giving a context no font");

	ui->font = font;
}

// The theme in force right now: the top of the push stack, or the base one
// voe_ui_theme_set gave the context when nothing is pushed. NULL when
// neither has happened, which every caller of this asserts against before
// using what it returns — see voe_ui_theme_push's and voe_ui_theme_pop's own
// asserts for the stack's bookkeeping.
static const voe_ui_theme *current_theme(const voe_ui_context *ui)
{
	return ui->theme_depth > 0
		       ? ui->theme_stack[ui->theme_depth - 1].theme
		       : ui->theme;
}

const voe_ui_theme *voe_ui_theme_current(const voe_ui_context *ui)
{
	return current_theme(ui);
}

void voe_ui_theme_set(voe_ui_context *ui, const voe_ui_theme *theme)
{
	VOE_BASE_ASSERT(ui != NULL, "setting a theme on no context");
	VOE_BASE_ASSERT(theme != NULL, "setting a context no theme");

	ui->theme = theme;
}

void voe_ui_theme_push(voe_ui_context *ui, const voe_ui_theme *theme)
{
	VOE_BASE_ASSERT(ui != NULL, "pushing a theme on no context");
	VOE_BASE_ASSERT(ui->state == VOE_UI_BUILDING,
			"pushing a theme outside a frame");
	VOE_BASE_ASSERT(theme != NULL, "pushing no theme");

	// Refused as a node past capacity is: named once, the frame carries on
	// with `theme_depth` still counting this push through `theme_refused`
	// instead, so the matching pop always finds something to undo. See
	// context.h.
	if (ui->theme_depth == ui->capacities.nodes) {
		if (!ui->theme_overrun)
			VOE_BASE_ERROR("ui",
				       "theme %u refused, this context was "
				       "created with room for %u pushed at "
				       "once",
				       ui->theme_depth + 1,
				       ui->capacities.nodes);
		ui->theme_overrun = true;
		ui->theme_refused++;
		return;
	}

	ui->theme_stack[ui->theme_depth++] = (struct voe_ui_theme_slot){
		.theme = theme,
	};
}

void voe_ui_theme_pop(voe_ui_context *ui)
{
	VOE_BASE_ASSERT(ui != NULL, "popping a theme on no context");
	VOE_BASE_ASSERT(ui->state == VOE_UI_BUILDING,
			"popping a theme outside a frame");
	VOE_BASE_ASSERT(ui->theme_depth > 0 || ui->theme_refused > 0,
			"popping a theme that was never pushed");

	if (ui->theme_refused > 0)
		ui->theme_refused--;
	else
		ui->theme_depth--;
}

void voe_ui_pointer_set(voe_ui_context *ui, voe_ui_pointer pointer)
{
	VOE_BASE_ASSERT(ui != NULL, "giving a pointer to no context");
	VOE_BASE_ASSERT(ui->state == VOE_UI_BUILDING,
			"giving a pointer outside a frame");

	ui->pointer = pointer;
}

void voe_ui_keyboard_set(voe_ui_context *ui, voe_ui_keyboard keyboard)
{
	VOE_BASE_ASSERT(ui != NULL, "giving a keyboard to no context");
	VOE_BASE_ASSERT(ui->state == VOE_UI_BUILDING,
			"giving a keyboard outside a frame");

	ui->keyboard = keyboard;
}

voe_ui_node voe_ui_panel_begin(voe_ui_context *ui, const char *name,
			       uint32_t index, voe_ui_surface surface,
			       voe_ui_container container)
{
	voe_ui_node node;
	uint64_t key;
	const voe_ui_theme *theme = current_theme(ui);

	VOE_BASE_ASSERT(ui != NULL, "opening a panel on no context");
	VOE_BASE_ASSERT(name != NULL, "a panel with no name has no identity");
	VOE_BASE_ASSERT(surface == VOE_UI_SURFACE_NONE || theme != NULL,
			"a panel with a surface needs a theme: see "
			"voe_ui_theme_set/voe_ui_theme_push");

	key = claim(ui, name, index);

	node = voe_ui_column_begin(ui, container);
	if (node != VOE_UI_NODE_NONE) {
		ui->widgets[node].kind = VOE_UI_WIDGET_PANEL;
		ui->widgets[node].surface = surface;
		ui->widgets[node].theme = theme;
		ui->widgets[node].key = key;
		ui->widgets[node].keyed = true;
	}

	return node;
}

voe_ui_node voe_ui_label_role(voe_ui_context *ui, const char *text,
			      voe_ui_text_role role)
{
	voe_text_measure measured;
	const voe_ui_theme *theme = current_theme(ui);
	float em;
	voe_ui_node node;

	VOE_BASE_ASSERT(ui != NULL, "adding a label to no context");
	VOE_BASE_ASSERT(text != NULL, "a label with no string");
	VOE_BASE_ASSERT(ui->font != NULL,
			"a label needs a font: measuring a string is the one "
			"thing it cannot do for itself — see voe_ui_font_set");
	VOE_BASE_ASSERT(theme != NULL,
			"a label needs a theme: its size and its ink both "
			"come from one — see voe_ui_theme_set/voe_ui_theme_push");

	// ADR-0090: the size is the font's measurement of this string, and
	// there is no table of advances and no assumed line height anywhere
	// above this call. The theme's own text_size is applied HERE, before
	// the number becomes a natural size, so that everything layout does
	// afterwards is ordinary layout in ordinary millimetres.
	em = theme->text_size;
	measured = voe_text_font_measure(ui->font, text);

	node = voe_ui_box(ui,
			  (voe_math_float2){ measured.size.x * em,
					     measured.size.y * em },
			  (voe_ui_sizing){ 0 });
	if (node != VOE_UI_NODE_NONE) {
		ui->widgets[node].kind = VOE_UI_WIDGET_LABEL;
		ui->widgets[node].text = text;
		ui->widgets[node].baseline = measured.baseline * em;
		ui->widgets[node].theme = theme;
		ui->widgets[node].text_role = role;
	}

	return node;
}

voe_ui_node voe_ui_label(voe_ui_context *ui, const char *text)
{
	return voe_ui_label_role(ui, text, VOE_UI_TEXT_ROLE_NORMAL);
}

voe_ui_node voe_ui_image(voe_ui_context *ui, voe_render_texture texture,
			 voe_math_float4 sheet, voe_math_float2 content,
			 voe_ui_sizing sizing)
{
	voe_ui_node node;

	VOE_BASE_ASSERT(ui != NULL, "adding an image to no context");

	// A box and nothing more as far as layout is concerned: every rule
	// about its size, and the assert for one outside a container, is
	// voe_ui_box's. No key is claimed, having nothing to remember.
	node = voe_ui_box(ui, content, sizing);
	if (node != VOE_UI_NODE_NONE) {
		ui->widgets[node].kind = VOE_UI_WIDGET_IMAGE;
		ui->widgets[node].sheet = sheet;
		ui->widgets[node].texture = texture.index;
	}

	return node;
}

// ------------------------------------------------------------- the frame

void voe_ui_widgets_init(voe_ui_context *ui, voe_base_arena *arena)
{
	voe_ui_scrolls_init(ui, arena);
}

void voe_ui_widgets_frame_begin(voe_ui_context *ui, voe_base_arena *arena)
{
	uint32_t slots = 16;

	// Twice the nodes, rounded up to a power of two, so that the set is at
	// most half full and the linear probe in voe_ui_key_taken always ends.
	while ((uint64_t)slots < 2u * (uint64_t)ui->capacities.nodes)
		slots <<= 1;

	// One push per array and never one per node: two pushes are not
	// guaranteed to be adjacent (base/arena.h) and each of these is indexed
	// as an array. Everything here is zeroed by push, which is what makes a
	// node no widget call touched a VOE_UI_WIDGET_NONE.
	ui->widgets = voe_base_arena_push(
		arena, (size_t)ui->capacities.nodes * sizeof(*ui->widgets));
	ui->seen = voe_base_arena_push(arena,
				       (size_t)slots * sizeof(*ui->seen));
	ui->seen_used = voe_base_arena_push(
		arena, (size_t)slots * sizeof(*ui->seen_used));
	ui->seen_mask = slots - 1;
	ui->collision = false;

	// A caller that asked for no elements is one that only wants
	// rectangles, and an arena push of nothing has no caller.
	ui->elements = NULL;
	if (ui->capacities.elements > 0)
		ui->elements = voe_base_arena_push(
			arena, (size_t)ui->capacities.elements *
				       sizeof(*ui->elements));
	ui->element_count = 0;
	ui->element_overrun = false;

	ui->scroll_areas = NULL;
	if (ui->capacities.scrolls > 0)
		ui->scroll_areas = voe_base_arena_push(
			arena, (size_t)ui->capacities.scrolls *
				       sizeof(*ui->scroll_areas));
	ui->scroll_count = 0;
	ui->scroll_overrun = false;

	// Bounded by the node capacity, exactly as `open` is — see context.h.
	// `ui->theme` itself is not reset here: it survives a frame as `font`
	// does.
	ui->theme_stack = voe_base_arena_push(
		arena, (size_t)ui->capacities.nodes * sizeof(*ui->theme_stack));
	ui->theme_depth = 0;
	ui->theme_refused = 0;
	ui->theme_overrun = false;

	// A frame that says nothing about the pointer has none. See widgets.h.
	ui->pointer = (voe_ui_pointer){ 0 };
	// And a frame that says nothing about typing has none, for the same
	// reason.
	ui->keyboard = (voe_ui_keyboard){ 0 };

	// No number box is built open until one is.
	ui->number_open_node = VOE_UI_NODE_NONE;
	ui->number_open_label = VOE_UI_NODE_NONE;
	ui->number_open_end = 0;

	voe_ui_colour_frame_begin(ui);
}

void voe_ui_push_element(voe_ui_context *ui, voe_render_element element)
{
	if (ui->element_count == ui->capacities.elements) {
		if (!ui->element_overrun)
			VOE_BASE_ERROR("ui",
				       "element %u refused, this context was "
				       "created with room for %u",
				       ui->element_count + 1,
				       ui->capacities.elements);
		ui->element_overrun = true;
		return;
	}

	ui->elements[ui->element_count++] = element;
}

// A record's `bounds` and its `clip`: `bounds` narrowed to what the clipping
// ancestors of `node` leave of it. False when nothing is left on either axis, and
// then the record is not pushed.
//
// A ZEROED CLIP CLIPS EVERYTHING AWAY (see voe_render_element.clip), which is why
// a record that clips nothing carries its own bounds and not nought, and why an
// empty one is skipped rather than sent to draw nothing.
static bool clip_of(const voe_ui_context *ui, uint32_t node,
		    voe_math_float4 bounds, voe_math_float4 *clip)
{
	voe_ui_rect rect = voe_ui_limit(
		ui, node,
		(voe_ui_rect){ .min = { bounds.x, bounds.y },
			       .size = { bounds.z, bounds.w } });

	*clip = (voe_math_float4){ rect.min.x, rect.min.y, rect.size.x,
				   rect.size.y };
	return rect.size.x > 0.0f && rect.size.y > 0.0f;
}

voe_math_float4 voe_ui_bounds_of(voe_ui_rect rect)
{
	return (voe_math_float4){ rect.min.x, rect.min.y, rect.size.x,
				  rect.size.y };
}

// A record over `at`, clipped by `node`'s own clipping ancestors — the same
// rule a node's own rectangle goes through, for a rectangle that need not be
// it (the fill inset inside a border, for one).
static void push_rect_at(voe_ui_context *ui, uint32_t node, voe_ui_rect at,
			 voe_math_float4 colour)
{
	voe_math_float4 bounds = voe_ui_bounds_of(at);
	voe_math_float4 clip;

	if (!clip_of(ui, node, bounds, &clip))
		return;

	voe_ui_push_element(ui, (voe_render_element){
				 .bounds = bounds,
				 .clip = clip,
				 .colour = colour,
				 .kind = VOE_RENDER_ELEMENT_SOLID,
			 });
}

void voe_ui_push_solid(voe_ui_context *ui, uint32_t node, voe_ui_rect at,
		       voe_math_float4 colour)
{
	push_rect_at(ui, node, at, colour);
}

// A node's rectangle straight across, clipped to its visible rectangle.
void voe_ui_push_rect(voe_ui_context *ui, uint32_t node,
		      voe_math_float4 colour)
{
	push_rect_at(ui, node, ui->nodes[node].rect, colour);
}

// `rect` moved in by `by` on every side, size nought rather than negative
// where the inset would cross itself — a node smaller than twice the
// hairline draws its fill at nothing rather than turned inside out.
static voe_ui_rect inset(voe_ui_rect rect, float by)
{
	voe_ui_rect out = { .min = { rect.min.x + by, rect.min.y + by },
			    .size = { rect.size.x - 2.0f * by,
				      rect.size.y - 2.0f * by } };

	if (out.size.x < 0.0f)
		out.size.x = 0.0f;
	if (out.size.y < 0.0f)
		out.size.y = 0.0f;
	return out;
}

// The hairline border, ADR-0171: the border role at the node's own bounds,
// then `fill` inset from every edge by HAIRLINE_WIDE, painted over the
// border's middle and leaving a rim of it showing all round. Two element
// records for what a plain fill costs one of.
void voe_ui_push_bordered(voe_ui_context *ui, uint32_t node,
			  voe_math_float4 fill, voe_math_float4 border)
{
	voe_ui_push_rect(ui, node, border);
	push_rect_at(ui, node, inset(ui->nodes[node].rect, HAIRLINE_WIDE),
		    fill);
}

// A picture over the node's rectangle, clipped as voe_ui_push_rect's is. The
// sheet and the texture index go across as the caller handed them in.
static void push_image(voe_ui_context *ui, uint32_t node)
{
	const struct voe_ui_widget_record *w = &ui->widgets[node];
	voe_math_float4 bounds = voe_ui_bounds_of(ui->nodes[node].rect);
	voe_math_float4 clip;

	if (!clip_of(ui, node, bounds, &clip))
		return;

	voe_ui_push_element(ui, (voe_render_element){
				 .bounds = bounds,
				 .clip = clip,
				 .colour = IMAGE_AS_IT_IS,
				 .kind = VOE_RENDER_ELEMENT_IMAGE,
				 .sheet_texture = w->texture,
				 .sheet = w->sheet,
			 });
}

// One letter of a label.
//
// DEVIATION: task 2 of spec 001 says a record's clip is its rectangle intersected
// with the node's visible rectangle. A glyph's box reaches past its label's own
// rectangle — the field's margin, the half slack added above the baseline — so
// that reading would trim letters in a tree that clips nothing, and a zeroed
// container must behave as today. The narrowest reading that keeps both: a glyph
// is narrowed by what the label's clipping ancestors leave, which is the visible
// rectangle's own rule, and not by the label's rectangle.
static void push_glyph(voe_ui_context *ui, uint32_t node,
		       voe_math_float4 bounds, voe_math_float4 sheet,
		       voe_math_float4 ink)
{
	voe_math_float4 clip;

	if (!clip_of(ui, node, bounds, &clip))
		return;

	voe_ui_push_element(ui, (voe_render_element){
				 .bounds = bounds,
				 .clip = clip,
				 .colour = ink,
				 .kind = VOE_RENDER_ELEMENT_GLYPH,
				 .sheet_texture =
					 voe_text_font_atlas(ui->font).index,
				 // Straight across: voe_text_glyph already
				 // hands the sheet rectangle over in the
				 // record's own shape, its corner paired to
				 // the box's top-left.
				 .sheet = sheet,
			 });
}

// One record per character that draws, in reading order. A space advances the
// pen and costs nothing, which on this path is a whole element of the frame's
// capacity saved for every one of them.
static void push_label(voe_ui_context *ui, uint32_t node)
{
	const struct voe_ui_widget_record *w = &ui->widgets[node];
	voe_ui_rect rect = ui->nodes[node].rect;
	// The SAME theme this label read at the call that made it, so what is
	// drawn here can never disagree with what was measured then — see
	// context.h on why the theme is copied onto the node rather than
	// looked up again.
	float em = w->theme->text_size;
	voe_math_float4 ink = w->text_role == VOE_UI_TEXT_ROLE_ACCENT
				      ? w->theme->accent
			      : w->text_role == VOE_UI_TEXT_ROLE_SECONDARY
				      ? w->theme->text_secondary
				      : w->theme->text_primary;
	float line = voe_text_font_line_height(ui->font) * em;
	float pen = rect.min.x;
	// The slack under the last line: the descender and the line gap, which
	// is everything of the line box the letters of an ordinary word do not
	// reach into. It is `line` and not `rect.size.y` because a label of
	// three lines has three line heights in its box and only the last one's
	// slack is under the text.
	float slack = line - w->baseline;
	// The font measures up from a baseline and this surface measures down
	// from a corner, so the baseline is BELOW the top edge by the ascender
	// the measurement handed back. Adding it is the whole of the conversion
	// and it is not a flip — see this file's header.
	//
	// AND HALF THE SLACK IS ADDED WITH IT, WHICH IS WHY A WORD SITS IN THE
	// MIDDLE OF A BUTTON RATHER THAN HIGH IN IT. A label's box is a whole
	// line — ascender, descender and line gap — and `click: 0` inks only the
	// part above the baseline, so a container centring that box centres the
	// space and not the writing: every button came out with more room under
	// its word than over it. Halving the slack puts as much of it above the
	// letters as below.
	//
	// IT MOVES THE TEXT AND NOT THE BOX. The rectangle layout measured is
	// untouched, so nothing about sizes, hit rectangles or where the next
	// widget goes changes — this is the last step before the letters become
	// records, and it is the only place in this folder that adjusts one.
	//
	// IT IS METRIC CENTRING AND NOT OPTICAL CENTRING. A capital is shorter
	// than the ascender, so a word of capitals still sits a little high; the
	// ink's own extent is a question for `text` and this folder cannot ask
	// it. The descender is the large half of the error and this is the half
	// that can be fixed from here.
	//
	// AND IT IS A DEFAULT AND NOT A POLICY. When styling arrives it decides
	// where a label sits in its box and this line goes with it.
	float baseline = rect.min.y + w->baseline + slack * 0.5f;

	for (const char *at = w->text; *at != '\0';) {
		uint32_t codepoint;
		voe_text_glyph g;

		at += voe_text_utf8_next(at, &codepoint);

		if (codepoint == '\n') {
			pen = rect.min.x;
			baseline += line;
			continue;
		}

		g = voe_text_font_glyph(ui->font, codepoint);
		if (g.drawn) {
			// The box whole and not trimmed: the field carries on
			// past the outline and a rectangle cut back to what the
			// letter looks like clips its own stems.
			voe_math_float4 bounds = {
				pen + g.low.x * em, baseline - g.high.y * em,
				(g.high.x - g.low.x) * em,
				(g.high.y - g.low.y) * em
			};

			push_glyph(ui, node, bounds, g.sheet, ink);
		}

		pen += g.advance * em;
	}
}

// A panel's own surface, read off its theme. Never called for NONE: emit
// skips such a panel before this, there being no colour a NONE panel draws.
static voe_math_float4 surface_colour(const voe_ui_theme *theme,
				      voe_ui_surface surface)
{
	switch (surface) {
	case VOE_UI_SURFACE_GROUND:
		return theme->ground;
	case VOE_UI_SURFACE_SURFACE:
		return theme->surface;
	case VOE_UI_SURFACE_RAISED:
		return theme->surface_raised;
	case VOE_UI_SURFACE_NONE:
		break;
	}
	VOE_BASE_ASSERT(false,
			"a NONE panel has no surface colour; emit skips it "
			"before this is called");
	return theme->ground;
}

static void emit(voe_ui_context *ui)
{
	for (uint32_t at = 0; at < ui->count; at++) {
		uint32_t node = voe_ui_paint_order(ui, at);

		// An open number box's own content is not drawn: nothing
		// emitted, as a plain row emits nothing.
		switch (voe_ui_number_hidden(ui, node)
				? VOE_UI_WIDGET_NONE
				: ui->widgets[node].kind) {
		case VOE_UI_WIDGET_COLOUR_PICKER:
			// A picker is a panel: see colour.c.
		case VOE_UI_WIDGET_PANEL:
			// A NONE surface emits nothing at all: not a
			// transparent rectangle — no record, no instance and
			// no blend to draw nothing — and a transparent panel
			// with padding in it is a real thing to want. See
			// widgets.h. Every other surface draws the hairline
			// border ADR-0171 asks for, two records where one
			// used to do.
			if (ui->widgets[node].surface != VOE_UI_SURFACE_NONE)
				voe_ui_push_bordered(ui, node,
					     surface_colour(ui->widgets[node].theme,
							    ui->widgets[node].surface),
					     ui->widgets[node].theme->border);
			break;
		case VOE_UI_WIDGET_BUTTON:
			voe_ui_button_emit(ui, node);
			break;
		case VOE_UI_WIDGET_NUMBER:
			voe_ui_number_emit(ui, node);
			break;
		case VOE_UI_WIDGET_FIELD:
			voe_ui_field_emit(ui, node);
			break;
		case VOE_UI_WIDGET_LABEL:
			push_label(ui, node);
			// THE CARET COMES AFTER THE LABEL'S OWN GLYPHS, so it
			// paints in front of them rather than under them, which
			// is why it is asked for here and not beside the
			// field's own background above — see field.c, which
			// knows which labels are a field's or an open number
			// box's.
			voe_ui_caret_emit(ui, node);
			break;
		case VOE_UI_WIDGET_IMAGE:
			push_image(ui, node);
			break;
		case VOE_UI_WIDGET_SWATCH:
		case VOE_UI_WIDGET_COLOUR_SQUARE:
		case VOE_UI_WIDGET_COLOUR_HUE:
			voe_ui_colour_emit(ui, node);
			break;
		case VOE_UI_WIDGET_SCROLL:
			// Nothing behind its content: its bars come after it.
		case VOE_UI_WIDGET_NONE:
			// A plain row, column or box draws nothing. Layout is
			// what they are for.
			break;
		}

		// After this node's own records, and so after the last of an
		// area's subtree when this is it. See scroll.c.
		voe_ui_scroll_emit(ui, at);
	}
}

void voe_ui_widgets_frame_end(voe_ui_context *ui, bool laid_out)
{
	// Asked before the press is resolved, which moves `was_down` on.
	bool pressed = ui->pointer.down && !ui->was_down;

	voe_ui_pointer_resolve(ui, laid_out);
	voe_ui_scrolls_remember(ui, laid_out);
	if (laid_out) {
		// After remembering, so every move is on top of the offset this
		// frame was laid out at and lands in the next frame's layout.
		voe_ui_scrolls_move(ui);
		// After the press has settled which field, if any, is focused —
		// see field.c — and before emission, which is what draws the
		// text the edit just wrote.
		voe_ui_field_edit(ui);
	}
	// After the edit, so a hex field's commit is settled, and before
	// emission; a refused frame's pickers answer with what they were given.
	voe_ui_colour_frame_end(ui, laid_out, pressed);
	if (laid_out)
		emit(ui);
	ui->typing = ui->focus_set;
}

// ------------------------------------------------------------- read back

uint32_t voe_ui_element_count(const voe_ui_context *ui)
{
	VOE_BASE_ASSERT(ui != NULL, "counting the elements of no context");
	VOE_BASE_ASSERT(ui->state == VOE_UI_LAID_OUT,
			"counting elements before the frame has ended");

	return ui->element_count;
}

voe_render_element voe_ui_element(const voe_ui_context *ui, uint32_t index)
{
	VOE_BASE_ASSERT(ui != NULL, "reading an element from no context");
	VOE_BASE_ASSERT(ui->state == VOE_UI_LAID_OUT,
			"reading an element before the frame has ended");
	VOE_BASE_ASSERT(index < ui->element_count,
			"reading an element past the ones this frame emitted");

	return ui->elements[index];
}
