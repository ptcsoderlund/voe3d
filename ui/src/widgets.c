// The widgets: what a node means, what the pointer is doing to it, and what
// element records come out of it. See include/ui/widgets.h for the promises;
// this file is where the decisions behind them are written down.
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
// ---- THE HIT TEST IS AFTER ARRANGE, AND THAT IS THE WHOLE OF WHY ----
//
// It runs inside voe_ui_frame_end, once the rectangles exist. The tempting
// alternative — answer at the widget call, from where the button was last frame
// — is wrong exactly when it matters: the frame a panel opened, a row reflowed,
// or a label grew is the frame a person clicks on something that has moved. See
// widgets.h for what that costs the caller, which is that the answer is read
// after frame_end rather than returned by the call.
//
// THE LAST WIDGET IN PAINT ORDER WINS THE POINTER, because the last one painted
// is the one in front. The loop does not stop at the first hit for that reason.
//
// ---- WHAT THE TWO IDS DO, INCLUDING THE AWKWARD CASES ----
//
// `hovered` is worked out afresh every frame and remembers nothing. `held` is
// the only thing in this folder that survives a frame, and it is what makes a
// press and a release one gesture rather than two events.
//
//   - PRESS ARMS THE HOVERED WIDGET AND ONLY ON THE EDGE. A button that is
//     already down when the pointer arrives over a widget arms nothing, so
//     dragging onto a button with the mouse held does not press it. That is what
//     the style target does and it is why `was_down` exists.
//   - A DRAG OFF THE WIDGET UN-HOVERS IT AND KEEPS IT HELD. Release then finds
//     hovered and held disagreeing and fires nothing. Drag back on and they
//     agree again, so it fires — cancelling is what leaving does and coming back
//     undoes it.
//   - RELEASE ALWAYS LETS GO, fired or not. There is no state in which the
//     button is up and something is still held.
//   - A HELD WIDGET THAT IS NOT CALLED THIS FRAME IS NO LONGER HELD. A panel
//     closed with a button down, a list that got shorter, a branch of the
//     interface that stopped being built: the key is simply not among this
//     frame's, and it is dropped. Without this, the id sits there until
//     something happens to be given the same key and inherits a press nobody
//     made — which is the immediate-mode stuck button, and it looks like the
//     interface has jammed rather than like a bug in bookkeeping.
//   - A REFUSED FRAME HOLDS NOTHING. It has no rectangles, so it cannot honestly
//     say anything is hovered; carrying yesterday's answer through would be
//     state nobody wrote.
//
// ---- AND WHAT A DRAG ADDS TO THEM, WHICH IS FOUR FIELDS AND NO TABLE ----
//
// A number box is armed and let go by every one of the rules above, unchanged —
// it is a button as far as the press is concerned. What it adds is that the
// frames BETWEEN the press and the release mean something, and that is the whole
// of the difference.
//
// ONE GESTURE AT A TIME IS WHY THERE IS NO SECOND TABLE. Only one widget can be
// held, so only one can be being dragged; the press position, last frame's
// position and whether the dead zone has been crossed are about that one widget
// and there is nothing to key them by. They sit beside `held` in the context and
// are written when it is armed.
//
// THE CHANGE IS WORKED OUT IN MILLIMETRES HERE AND TURNED INTO THE CALLER'S UNIT
// IN voe_ui_number_action, because `per_millimetre` belongs to the node and this
// pass has a key rather than a node in its hand. It is also the honest split:
// what happened is a distance, and what it is worth is the caller's business.
//
// A NUMBER BOX NEVER FIRES, and that is enforced where the release is handled
// rather than left to the reader. The click is reserved for the typing that is
// not built yet — see widgets.h.
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

// CARD 036 REPLACES EVERY COLOUR IN THIS BLOCK AND THEY ARE HERE SO THAT IT HAS
// SOMETHING TO REPLACE. Three states and an ink, written as constants in one
// place: a struct of named roles here would be the theme arriving early and
// badly, and the whole of a theme is a decision this card does not get to make.
//
// THEY ARE LINEAR, like every colour that crosses render's boundary, so they
// look further apart on screen than the numbers do. And they are STRAIGHT, not
// premultiplied — the shader multiplies rgb by a once, at output (ADR-0069), and
// doing it here as well gives an interface that reads as washed out rather than
// as wrong.
static const voe_math_float4 BUTTON_NORMAL = { 0.14f, 0.15f, 0.17f, 1.0f };
static const voe_math_float4 BUTTON_HOVERED = { 0.28f, 0.30f, 0.34f, 1.0f };
static const voe_math_float4 BUTTON_HELD = { 0.02f, 0.20f, 0.48f, 1.0f };
static const voe_math_float4 LABEL_INK = { 0.85f, 0.87f, 0.90f, 1.0f };

// How many millimetres one em is, before the text scale. Card 036 owns this one
// as well: a text size is a thing a theme says, and until there is a theme it is
// a number here with the font's own measurements multiplied by it.
#define TEXT_EM 4.0f

// Inside every edge of a button, in millimetres. Its size is otherwise entirely
// its label's, so this is the whole of what makes a button bigger than the word
// in it.
// The same on all four sides, which is what a button wants and what the four
// numbers make explicit rather than assume.
//
// A NUMBER BOX USES IT TOO, and shares the three colours above, because it is
// meant to look like a button — a thing you put the pointer on and press. One
// constant and not a copy, so that card 036 replaces it once.
#define BUTTON_PAD                                                             \
	((voe_ui_pad){ 2.5f, 2.5f, 2.5f, 2.5f })

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
static bool key_taken(voe_ui_context *ui, uint64_t key)
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

	if (key_taken(ui, key)) {
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

static bool inside(voe_ui_rect rect, voe_math_float2 at)
{
	return at.x >= rect.min.x && at.x < rect.min.x + rect.size.x &&
	       at.y >= rect.min.y && at.y < rect.min.y + rect.size.y;
}

// How many millimetres one em is worth on this frame's surface. The one
// multiplication that turns a font's own unit into the surface's, and it happens
// here so that everything downstream — a natural size, a pen position, a glyph's
// box — is already in millimetres. See widgets.h on how it composes with the
// surface's own scale.
static float em_millimetres(const voe_ui_context *ui)
{
	return TEXT_EM * ui->text_scale;
}

void voe_ui_font_set(voe_ui_context *ui, const voe_text_font *font)
{
	VOE_BASE_ASSERT(ui != NULL, "giving a font to no context");
	VOE_BASE_ASSERT(font != NULL, "giving a context no font");

	ui->font = font;
}

void voe_ui_text_scale_set(voe_ui_context *ui, float scale)
{
	VOE_BASE_ASSERT(ui != NULL, "setting a text scale on no context");
	VOE_BASE_ASSERT(scale > 0.0f, "a text scale of nought or less");

	ui->text_scale = scale;
}

void voe_ui_pointer_set(voe_ui_context *ui, voe_ui_pointer pointer)
{
	VOE_BASE_ASSERT(ui != NULL, "giving a pointer to no context");
	VOE_BASE_ASSERT(ui->state == VOE_UI_BUILDING,
			"giving a pointer outside a frame");

	ui->pointer = pointer;
}

voe_ui_node voe_ui_panel_begin(voe_ui_context *ui, const char *name,
			       uint32_t index, voe_math_float4 colour,
			       voe_ui_container container)
{
	voe_ui_node node;
	uint64_t key;

	VOE_BASE_ASSERT(ui != NULL, "opening a panel on no context");
	VOE_BASE_ASSERT(name != NULL, "a panel with no name has no identity");

	key = claim(ui, name, index);

	node = voe_ui_column_begin(ui, container);
	if (node != VOE_UI_NODE_NONE) {
		ui->widgets[node].kind = VOE_UI_WIDGET_PANEL;
		ui->widgets[node].colour = colour;
		ui->widgets[node].key = key;
		ui->widgets[node].keyed = true;
	}

	return node;
}

voe_ui_node voe_ui_label(voe_ui_context *ui, const char *text)
{
	voe_text_measure measured;
	float em;
	voe_ui_node node;

	VOE_BASE_ASSERT(ui != NULL, "adding a label to no context");
	VOE_BASE_ASSERT(text != NULL, "a label with no string");
	VOE_BASE_ASSERT(ui->font != NULL,
			"a label needs a font: measuring a string is the one "
			"thing it cannot do for itself — see voe_ui_font_set");

	// ADR-0090: the size is the font's measurement of this string, and
	// there is no table of advances and no assumed line height anywhere
	// above this call. The text scale is applied HERE, before the number
	// becomes a natural size, so that everything layout does afterwards is
	// ordinary layout in ordinary millimetres.
	em = em_millimetres(ui);
	measured = voe_text_font_measure(ui->font, text);

	node = voe_ui_box(ui,
			  (voe_math_float2){ measured.size.x * em,
					     measured.size.y * em },
			  (voe_ui_sizing){ 0 });
	if (node != VOE_UI_NODE_NONE) {
		ui->widgets[node].kind = VOE_UI_WIDGET_LABEL;
		ui->widgets[node].text = text;
		ui->widgets[node].baseline = measured.baseline * em;
	}

	return node;
}

voe_ui_node voe_ui_button_begin(voe_ui_context *ui, const char *name,
				uint32_t index)
{
	voe_ui_node node;
	uint64_t key;

	VOE_BASE_ASSERT(ui != NULL, "opening a button on no context");
	VOE_BASE_ASSERT(name != NULL, "a button with no name has no identity");

	key = claim(ui, name, index);

	// A row so that what is in it is centred on both axes with one setting
	// each, and a row rather than a column because a second thing in a
	// button — an icon beside the word — goes across and not under.
	node = voe_ui_row_begin(ui, (voe_ui_container){
					   .along = VOE_UI_ALONG_CENTER,
					   .across = VOE_UI_ACROSS_CENTER,
					   .pad = BUTTON_PAD });
	if (node != VOE_UI_NODE_NONE) {
		ui->widgets[node].kind = VOE_UI_WIDGET_BUTTON;
		ui->widgets[node].key = key;
		ui->widgets[node].keyed = true;
	}

	return node;
}

voe_ui_node voe_ui_number_begin(voe_ui_context *ui, const char *name,
				uint32_t index, double value,
				double per_millimetre)
{
	voe_ui_node node;
	uint64_t key;

	VOE_BASE_ASSERT(ui != NULL, "opening a number box on no context");
	VOE_BASE_ASSERT(name != NULL,
			"a number box with no name has no identity");

	key = claim(ui, name, index);

	// Built exactly as a button is, down to the padding and the centring:
	// what is in it is composed rather than passed, so a caller with no
	// font can still build one and drag it. See widgets.h.
	node = voe_ui_row_begin(ui, (voe_ui_container){
					   .along = VOE_UI_ALONG_CENTER,
					   .across = VOE_UI_ACROSS_CENTER,
					   .pad = BUTTON_PAD });
	if (node != VOE_UI_NODE_NONE) {
		ui->widgets[node].kind = VOE_UI_WIDGET_NUMBER;
		ui->widgets[node].key = key;
		ui->widgets[node].keyed = true;
		ui->widgets[node].value = value;
		ui->widgets[node].per_millimetre = per_millimetre;
	}

	return node;
}

// ------------------------------------------------------------- the frame

void voe_ui_widgets_frame_begin(voe_ui_context *ui, voe_base_arena *arena)
{
	uint32_t slots = 16;

	// Twice the nodes, rounded up to a power of two, so that the set is at
	// most half full and the linear probe in key_taken always ends.
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

	// A frame that says nothing about the pointer has none. See widgets.h.
	ui->pointer = (voe_ui_pointer){ 0 };
}

// Which widgets answer the pointer at all. A panel and a label do not: a panel
// is a background and a label is a measurement, and neither has ever been asked
// what the mouse is doing to it.
static bool takes_the_pointer(enum voe_ui_widget kind)
{
	return kind == VOE_UI_WIDGET_BUTTON || kind == VOE_UI_WIDGET_NUMBER;
}

// This frame's movement of the held number box, in millimetres.
//
// THE DEAD ZONE IS MEASURED FROM THE PRESS AND THE DRAG FROM LAST FRAME, and
// those being two different origins is the whole of this function. Measuring the
// dead zone from last frame would let a slow hand cross it a tenth of a
// millimetre at a time without ever having moved; measuring the drag from the
// press would make every frame's change the whole distance travelled, so the
// value would race away as the square of the gesture.
//
// AND THE FIRST CHANGE IS WHAT LIES BEYOND THE DEAD ZONE, not the whole distance
// from the press. Otherwise the value jumps by a millimetre's worth the instant
// the drag begins, which is a visible step exactly where the person expects the
// gesture to start from nothing.
static void drag(voe_ui_context *ui)
{
	float x = ui->pointer.at.x;
	float change = 0.0f;

	if (ui->number_crossed) {
		change = x - ui->number_last_x;
	} else {
		float from_press = x - ui->number_press_x;

		// Either way out of it, and the sign is kept: dragging left
		// takes the value down by as much as dragging right takes it
		// up.
		if (from_press >= VOE_UI_NUMBER_DEAD_ZONE) {
			change = from_press - VOE_UI_NUMBER_DEAD_ZONE;
			ui->number_crossed = true;
		} else if (from_press <= -VOE_UI_NUMBER_DEAD_ZONE) {
			change = from_press + VOE_UI_NUMBER_DEAD_ZONE;
			ui->number_crossed = true;
		}
	}

	ui->number_last_x = x;
	ui->number_delta = change;
	// A frame the pointer did not move changed nothing, and says so rather
	// than reporting a change of nought as a change.
	ui->number_moved = change != 0.0f;
}

static void resolve(voe_ui_context *ui, bool laid_out)
{
	uint64_t hovered = 0;
	bool hovered_set = false;
	bool hovered_number = false;

	ui->fired_set = false;
	ui->number_delta = 0.0f;
	ui->number_moved = false;

	if (laid_out && ui->pointer.over) {
		// Every hit, not the first: the last one in paint order is the
		// one in front, and that is the one the pointer is on.
		for (uint32_t at = 0; at < ui->count; at++) {
			uint32_t node = voe_ui_paint_order(ui, at);

			if (!takes_the_pointer(ui->widgets[node].kind))
				continue;
			if (inside(ui->nodes[node].rect, ui->pointer.at)) {
				hovered = ui->widgets[node].key;
				hovered_set = true;
				hovered_number = ui->widgets[node].kind ==
						 VOE_UI_WIDGET_NUMBER;
			}
		}
	}

	if (!laid_out) {
		ui->hovered_set = false;
		ui->held_set = false;
		ui->held_number = false;
		ui->was_down = ui->pointer.down;
		return;
	}

	// A held widget the frame did not build is no longer held, and this is
	// asked before the press and the release so that a release cannot fire
	// something that is no longer there.
	if (ui->held_set && !key_taken(ui, ui->held)) {
		ui->held_set = false;
		ui->held_number = false;
	}

	if (ui->pointer.down && !ui->was_down) {
		// The edge, and only the edge: dragging onto a button with the
		// mouse already down arms nothing.
		ui->held = hovered;
		ui->held_set = hovered_set;
		ui->held_number = hovered_set && hovered_number;
		// Where this gesture began. Written whatever was armed, so
		// that a press on a button leaves nothing behind for the next
		// number box to inherit.
		ui->number_press_x = ui->pointer.at.x;
		ui->number_last_x = ui->pointer.at.x;
		ui->number_crossed = false;
	} else if (!ui->pointer.down && ui->was_down) {
		// A NUMBER BOX NEVER FIRES, which is why `held_number` is asked
		// here. The release that would have fired a button is the click
		// reserved for typing into one, and a caller that could see it
		// would bind something to it that has to be taken away again
		// when the caret arrives. See widgets.h.
		if (ui->held_set && !ui->held_number && hovered_set &&
		    hovered == ui->held) {
			ui->fired = ui->held;
			ui->fired_set = true;
		}
		ui->held_set = false;
		ui->held_number = false;
	}

	// After the press, so that the frame which arms a number box has a
	// change of nought rather than one measured from wherever the pointer
	// was last. `over` is not asked: a drag carries on past the surface's
	// edge, where the caller goes on reporting a pointer that is no longer
	// over anything. See voe_ui_pointer.
	if (ui->held_set && ui->held_number && ui->pointer.down)
		drag(ui);

	ui->hovered = hovered;
	ui->hovered_set = hovered_set;
	ui->was_down = ui->pointer.down;
}

static void push_element(voe_ui_context *ui, voe_render_element element)
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

// A rectangle straight across, clipped to itself.
//
// ITS OWN BOUNDS IS A REAL CLIP RECTANGLE AND IT CLIPS NOTHING, which is the
// answer this card wants: a zeroed one clips everything away (see
// voe_render_element.clip) and would draw an empty interface, and anything
// narrower than the widget is a scroll area, which is card 035. When that lands
// it narrows these; until then every record says "all of me".
static void push_rect(voe_ui_context *ui, voe_ui_rect rect,
		      voe_math_float4 colour)
{
	voe_math_float4 bounds = { rect.min.x, rect.min.y, rect.size.x,
				   rect.size.y };

	push_element(ui, (voe_render_element){
				 .bounds = bounds,
				 .clip = bounds,
				 .colour = colour,
				 .kind = VOE_RENDER_ELEMENT_SOLID,
			 });
}

// One record per character that draws, in reading order. A space advances the
// pen and costs nothing, which on this path is a whole element of the frame's
// capacity saved for every one of them.
static void push_label(voe_ui_context *ui, uint32_t node)
{
	const struct voe_ui_widget_record *w = &ui->widgets[node];
	voe_ui_rect rect = ui->nodes[node].rect;
	float em = em_millimetres(ui);
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

			push_element(
				ui,
				(voe_render_element){
					.bounds = bounds,
					.clip = bounds,
					.colour = LABEL_INK,
					.kind = VOE_RENDER_ELEMENT_GLYPH,
					.sheet_texture =
						voe_text_font_atlas(ui->font)
							.index,
					// Straight across: voe_text_glyph
					// already hands the sheet rectangle
					// over in the record's own shape, its
					// corner paired to the box's top-left.
					.sheet = g.sheet,
				});
		}

		pen += g.advance * em;
	}
}

// The three states, for a button and for a number box alike. They look the same
// on purpose — see BUTTON_PAD.
static voe_math_float4 state_colour(const voe_ui_context *ui, uint32_t node)
{
	uint64_t key = ui->widgets[node].key;

	// Held beats hovered, because a button being pressed is what a person
	// is doing and hovering is only where the pointer happens to be. A
	// button held with the pointer dragged off it stays in its held colour,
	// which is what says the press is still live and can still be
	// completed by coming back.
	if (ui->held_set && ui->held == key)
		return BUTTON_HELD;
	if (ui->hovered_set && ui->hovered == key)
		return BUTTON_HOVERED;
	return BUTTON_NORMAL;
}

static void emit(voe_ui_context *ui)
{
	for (uint32_t at = 0; at < ui->count; at++) {
		uint32_t node = voe_ui_paint_order(ui, at);

		switch (ui->widgets[node].kind) {
		case VOE_UI_WIDGET_PANEL:
			// A fully transparent panel emits nothing at all: an
			// alpha of nought costs a record, an instance and a
			// blend to draw nothing, and a transparent panel with
			// padding in it is a real thing to want. See widgets.h.
			if (ui->widgets[node].colour.w > 0.0f)
				push_rect(ui, ui->nodes[node].rect,
					  ui->widgets[node].colour);
			break;
		case VOE_UI_WIDGET_BUTTON:
		case VOE_UI_WIDGET_NUMBER:
			push_rect(ui, ui->nodes[node].rect,
				  state_colour(ui, node));
			break;
		case VOE_UI_WIDGET_LABEL:
			push_label(ui, node);
			break;
		case VOE_UI_WIDGET_NONE:
			// A plain row, column or box draws nothing. Layout is
			// what they are for.
			break;
		}
	}
}

void voe_ui_widgets_frame_end(voe_ui_context *ui, bool laid_out)
{
	resolve(ui, laid_out);
	if (laid_out)
		emit(ui);
}

// ------------------------------------------------------------- read back

voe_ui_action voe_ui_button_action(const voe_ui_context *ui, voe_ui_node button)
{
	uint64_t key;

	VOE_BASE_ASSERT(ui != NULL, "reading a button on no context");
	VOE_BASE_ASSERT(ui->state == VOE_UI_LAID_OUT,
			"reading a button before the frame has ended; nothing "
			"has a rectangle until then and so nothing has been "
			"hit tested");
	VOE_BASE_ASSERT(button != VOE_UI_NODE_NONE,
			"reading a button the frame had no room for");
	VOE_BASE_ASSERT(button < ui->count,
			"reading a button this frame never made");
	VOE_BASE_ASSERT(ui->widgets[button].kind == VOE_UI_WIDGET_BUTTON,
			"reading a button action from a node that is not a "
			"button");

	key = ui->widgets[button].key;

	return (voe_ui_action){
		.hovered = ui->hovered_set && ui->hovered == key,
		.held = ui->held_set && ui->held == key,
		.fired = ui->fired_set && ui->fired == key,
	};
}

voe_ui_number_result voe_ui_number_action(const voe_ui_context *ui,
					  voe_ui_node number)
{
	const struct voe_ui_widget_record *w;
	voe_ui_number_result result;

	VOE_BASE_ASSERT(ui != NULL, "reading a number box on no context");
	VOE_BASE_ASSERT(ui->state == VOE_UI_LAID_OUT,
			"reading a number box before the frame has ended; "
			"nothing has a rectangle until then and so nothing has "
			"been hit tested");
	VOE_BASE_ASSERT(number != VOE_UI_NODE_NONE,
			"reading a number box the frame had no room for");
	VOE_BASE_ASSERT(number < ui->count,
			"reading a number box this frame never made");
	VOE_BASE_ASSERT(ui->widgets[number].kind == VOE_UI_WIDGET_NUMBER,
			"reading a number box action from a node that is not a "
			"number box");

	w = &ui->widgets[number];
	result = (voe_ui_number_result){
		.hovered = ui->hovered_set && ui->hovered == w->key,
		.held = ui->held_set && ui->held == w->key,
		.changed = false,
		// The value handed in, unchanged, which is the answer on every
		// frame but the ones a drag moved it.
		.value = w->value,
	};

	if (result.held && ui->number_moved) {
		// THE UNIT CONVERSION HAPPENS HERE AND NOWHERE ELSE, because
		// `per_millimetre` is this node's and resolve had no node in
		// hand. Fine is a plain multiplier on the same product: which
		// order the three are multiplied in cannot matter, and this one
		// reads as "the drag, in the caller's unit, slowed".
		double change = (double)ui->number_delta * w->per_millimetre;

		if (ui->pointer.fine)
			change *= VOE_UI_NUMBER_FINE;

		// A per_millimetre of nought is a number box that does not
		// move, and it says nothing changed rather than reporting a
		// change of nought.
		result.changed = change != 0.0;
		result.value = w->value + change;
	}

	return result;
}

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
