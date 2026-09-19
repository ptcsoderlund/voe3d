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
// AND IT TESTS WHAT CAN BE SEEN, NOT WHERE THE WIDGET IS. The rectangle compared
// is voe_ui_node_visible, so the part of a button a clipping container cut away
// is not there for the pointer either: a widget scrolled out of sight cannot be
// hovered, armed or pressed (ADR-0153). A gesture already under way is keyed and
// not hit tested, so it carries on when its widget is clipped away, exactly as
// it carries on past the surface's edge.
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
// rather than left to the reader. Its click opens it for typing instead
// (ADR-0192): a release inside the dead zone, over the box, that no drag has
// crossed out of, gives it the focus a field has — see below.
//
// ---- THE FIELD: FOCUS IS NOT HELD, AND EDITING WAITS FOR RESOLVE ----
//
// A FIELD TAKES THE POINTER SO THAT A PRESS CAN FIND IT, but what a press
// does to it is not the held/hovered/fired machinery above: `focus` is its
// own key in the context, set to the key a press landed on when that hit is a
// field and cleared on every other press — nothing, a button, a bar — on the
// SAME edge, not on release. That is the whole difference from `held`: a
// press arms a button for the release still to come, while a press decides a
// field's focus outright and the release means nothing to it.
//
// FOCUS SURVIVES WHILE `held` DOES NOT, because they answer different
// questions. `held` is one gesture, over when the button is let go; `focus`
// is "which field is the keyboard going to", which stays true for as many
// frames as nothing changes it — otherwise every field would need a click
// held down for every letter typed into it. It is dropped exactly as an area
// not called forgets its offset: a key `claim` did not take this frame is no
// longer a field to be focused, checked once in resolve regardless of whether
// the frame laid out, because `claim` marks a key whether its node was
// refused or not.
//
// EDITING RUNS IN field_edit, AFTER resolve AND BEFORE emit, AND NOT INSIDE
// resolve ITSELF, because resolve is where this frame's press can MOVE focus
// to a field it had not settled on yet — the frame a person first clicks into
// one. Only once focus for this frame is final is there a field to edit.
//
// THE CONTEXT HOLDS THE FOCUSED FIELD'S TEXT IN ONE BUFFER AND NOT A TABLE,
// because only one field can be focused (ADR-0192). field_edit seeds it from
// the caller's text the frame the focus arrives, all of it selected; from the
// next frame voe_ui_field hands the buffer to its label instead of the
// caller's text, so layout measures what is shown. Focus leaving by Enter,
// Tab or a press elsewhere copies the buffer to `field_final`, a second
// buffer because a press into another field seeds this one in the same
// frame; Escape points the label back at the caller's text. The outcome is
// written onto the field's own widget record, read by voe_ui_field_action.
//
// A NUMBER BOX OPEN FOR TYPING IS THE SAME FOCUS AND THE SAME BUFFER, seeded
// with `%.6g` of the caller's value rather than a string. From the frame after
// the focus arrives voe_ui_number_begin builds it open: along START, a
// composed row first — a label of the buffer and, after a refused Enter or Tab,
// "not a number" — and the caller's own content after it, which still takes its
// room in the row but is neither drawn nor hit tested while the box is open,
// because a label the caller composed is the value it was handed and not what
// is being typed. A commit parses the buffer; the frame's answer goes on the
// box's own widget record as a field's does.

// ---- A SCROLL AREA: A TABLE REWRITTEN EVERY FRAME, AND A BAR IN PAINT ORDER ----
//
// THE TABLE IS NEVER EDITED IN PLACE, IT IS REWRITTEN. During a frame each
// scroll_begin looks its key up in last frame's table and hands layout what it
// finds; at frame_end the table is written again from this frame's areas, in call
// order, with the offset layout used. An area not called is not written back, and
// that is the whole of forgetting — no sweep, no age, and no slot a stale key can
// hold while a new one wants it. The lookup is a linear walk: a frame holds a
// handful of areas, and a hash would be a second collision rule for nothing.
//
// DEVIATION: spec 001 task 3 says the table stores the offset layout used; a
// refused frame has no layout. The narrowest reading: an area called in a refused
// frame keeps the offset it was handed, and one not called is still dropped.
//
// A MOVE IS APPLIED TO THE TABLE AFTER IT IS WRITTEN, so it lands in the next
// frame's layout: a thumb drag, then a track press, then the pointer's scroll.
// The pointer's goes through scroll_by, which starts at an area and passes what
// that area cannot take to the next area outward — the one function a program's
// voe_ui_scroll_by will expose when focus gives it a caller. An area's outer
// neighbour is found by the tree's shape and not by a stored parent: areas are
// listed in call order, so the nearest earlier one whose subtree holds this one
// is the one around it.
//
// THE BAR PAINTS STRAIGHT AFTER THE LAST NODE OF ITS AREA'S SUBTREE, which is
// where "after the area's children" is in paint order: a subtree fills the
// consecutive paint positions from its root's, so its last is the root's position
// plus its size less one. Emission and the hit test both walk paint order and ask
// bar_after at every position, so the bar is in front of the content for the
// pointer exactly as far as it is in front for the eye, and a panel anchored over
// the area is in front of both. Where nested areas end at one position the inner
// bar comes first and the outer paints over it.
//
// A THUMB DRAG MOVES THE OFFSET BY THE POINTER'S TRAVEL TIMES MEASURED OVER
// ARRANGED, measured from the press and not from last frame, so a thumb dragged
// past the end and back comes back under the pointer instead of lagging behind a
// clamp it hit on the way.
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

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// EVERY COLOUR BELOW IS A THEME ROLE NOW (ADR-0170, ADR-0171), READ FROM
// WHICHEVER WIDGET RECORD ASKS FOR ONE — there is no constant left in this file
// for a button's, a panel's, a field's, a caret's or a scrollbar's colour. See
// state_colour, field_colour, surface_colour and push_scrollbar for where each
// widget picks its role, and ui/theme.h for what the roles are and how they are
// derived.
//
// NOT A THEME COLOUR, AND IT STAYS A CONSTANT: an image record's colour
// multiplies the picture, and opaque white is the one value that shows the
// picture as it is. There is no tint argument, so there is nothing else it could
// be, and no theme could name one either.
static const voe_math_float4 IMAGE_AS_IT_IS = { 1.0f, 1.0f, 1.0f, 1.0f };

// Inside every edge of a button, in millimetres. Its size is otherwise entirely
// its label's, so this is the whole of what makes a button bigger than the word
// in it. The same on all four sides, which is what a button wants and what the
// four numbers make explicit rather than assume.
//
// A NUMBER BOX AND A FIELD USE IT TOO, because both are meant to look like a
// button — a thing you put the pointer on and press.
#define BUTTON_PAD                                                             \
	((voe_ui_pad){ 2.5f, 2.5f, 2.5f, 2.5f })

// Between an open number box's typed text and its "not a number", in
// millimetres.
#define NUMBER_REFUSED_GAP 1.5f

// A field's caret, in millimetres wide and as tall as its label's own
// rectangle.
#define FIELD_CARET_WIDE 0.3f

// A panel's and a button's hairline border, in millimetres on every side
// (ADR-0171) — the one width this folder draws a border at, so a change to how
// thick "hairline" is is one number. Not in ui/theme.h: the border's COLOUR is
// a role and the theme's to say, but how wide the rectangle it is drawn as is
// a layout fact about this folder's widgets and not a colour at all.
#define HAIRLINE_WIDE 0.3f

// A scroll area's bar, in millimetres. The thumb is never shorter than the
// minimum, so a very long list still has something to take hold of; where the
// track itself is shorter, the thumb is the track.
#define SCROLL_THICKNESS 1.5f
#define SCROLL_THUMB_MIN 5.0f

// No scroll area, where an index into this frame's areas is expected.
#define NO_AREA UINT32_MAX

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

uint64_t voe_ui_widget_claim(voe_ui_context *ui, const char *name,
			     uint32_t index)
{
	return claim(ui, name, index);
}

static bool inside(voe_ui_rect rect, voe_math_float2 at)
{
	return at.x >= rect.min.x && at.x < rect.min.x + rect.size.x &&
	       at.y >= rect.min.y && at.y < rect.min.y + rect.size.y;
}

// ---------------------------------------------------- a field's own editing

// A UTF-8 continuation byte, 10xxxxxx.
static bool is_continuation(unsigned char byte)
{
	return (byte & 0xc0) == 0x80;
}

// How many bytes the sequence starting with this leading byte takes, judged
// from its own high bits and never by decoding it — appending only needs the
// count, not the meaning, and `text/utf8.h`'s decoder is for a string already
// known to be whole. A byte that cannot lead a sequence — a continuation
// byte, or one past today's four-byte forms — is one byte, so a malformed
// lead does not stall the walk; nothing typed through a real keyboard
// produces one.
static uint32_t utf8_length(unsigned char lead)
{
	if ((lead & 0x80) == 0x00)
		return 1;
	if ((lead & 0xe0) == 0xc0)
		return 2;
	if ((lead & 0xf0) == 0xe0)
		return 3;
	if ((lead & 0xf8) == 0xf0)
		return 4;
	return 1;
}

// Whether a typed byte is one field_append keeps: nothing below 0x20 and not
// 0x7F, which a keyboard hands over for Tab, Enter or Delete but which a
// field's text never holds.
static bool typeable(unsigned char byte)
{
	return byte >= 0x20 && byte != 0x7f;
}

// Removes the last code point of a NUL-terminated string in place: the
// trailing continuation bytes and the byte before them. Does nothing on an
// empty string, there being no last code point to remove.
static void field_backspace(char *text)
{
	size_t len = strlen(text);

	if (len == 0)
		return;
	len--;
	while (len > 0 && is_continuation((unsigned char)text[len]))
		len--;
	text[len] = '\0';
}

// Appends `size` bytes of `typed` to the NUL-terminated `text`, whose buffer
// holds VOE_UI_FIELD_CAPACITY bytes and the NUL, code point by code point:
// stepping by each one's own leading byte's length so nothing reads past
// `size`, dropping a trailing partial sequence whole rather than reading past
// the end of what was typed, and dropping whole any code point that would not
// fit rather than cutting it at a byte capacity does not respect. A byte that
// is not typeable is skipped.
static void field_append(char *text, const char *typed, uint32_t size)
{
	size_t len = strlen(text);
	uint32_t at = 0;

	while (at < size) {
		uint32_t bytes = utf8_length((unsigned char)typed[at]);

		if (at + bytes > size)
			break;
		if (bytes == 1 && !typeable((unsigned char)typed[at])) {
			at++;
			continue;
		}
		if (len + bytes <= VOE_UI_FIELD_CAPACITY) {
			memcpy(text + len, typed + at, bytes);
			len += bytes;
		}
		at += bytes;
	}
	text[len] = '\0';
}

// A number box opening for typing: the buffer seeded with `%.6g` of the value
// the caller handed in, all of it selected, and that text kept as what a commit
// compares against. See context.h.
static void number_seed(voe_ui_context *ui, uint64_t key, double value)
{
	snprintf(ui->number_opened, sizeof(ui->number_opened), "%.6g", value);
	memcpy(ui->field_buffer, ui->number_opened, sizeof(ui->number_opened));
	ui->field_owner = key;
	ui->field_holding = true;
	ui->field_selected = true;
	ui->number_refused = false;
}

// Whether `text` is one finite number and nothing else: what strtod reads,
// blanks allowed either side of it, the whole text consumed. "1e999" reads as
// an infinity and "nan" as a NaN, and both are refused by the finite check;
// an empty text reads nothing and is refused too.
static bool number_parse(const char *text, double *out)
{
	char *end;
	double value = strtod(text, &end);

	if (end == text)
		return false;
	while (isspace((unsigned char)*end))
		end++;
	if (*end != '\0' || !isfinite(value))
		return false;
	*out = value;
	return true;
}

// One axis of a pair, `y` saying which, as layout.c reads them. Only the scroll
// area's arithmetic is the same on both axes, so only it uses these.
static float component(voe_math_float2 v, bool y)
{
	return y ? v.y : v.x;
}

static void component_set(voe_math_float2 *v, bool y, float value)
{
	if (y)
		v->y = value;
	else
		v->x = value;
}

static bool scrolls_on(voe_ui_scroll_axes axes, bool y)
{
	return y ? axes.y : axes.x;
}

// What is left of `rect` inside `within`, size nought on an axis with nothing.
static voe_ui_rect intersect(voe_ui_rect rect, voe_ui_rect within)
{
	voe_ui_rect out = { 0 };

	for (int axis = 0; axis < 2; axis++) {
		bool y = axis == 1;
		float low = component(rect.min, y);
		float high = low + component(rect.size, y);
		float within_low = component(within.min, y);
		float within_high = within_low + component(within.size, y);

		if (low < within_low)
			low = within_low;
		if (high > within_high)
			high = within_high;
		component_set(&out.min, y, low);
		component_set(&out.size, y, high > low ? high - low : 0.0f);
	}
	return out;
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

voe_ui_node voe_ui_button_begin(voe_ui_context *ui, const char *name,
				uint32_t index)
{
	voe_ui_node node;
	uint64_t key;
	const voe_ui_theme *theme = current_theme(ui);

	VOE_BASE_ASSERT(ui != NULL, "opening a button on no context");
	VOE_BASE_ASSERT(name != NULL, "a button with no name has no identity");
	VOE_BASE_ASSERT(theme != NULL,
			"a button needs a theme: see "
			"voe_ui_theme_set/voe_ui_theme_push");

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
		ui->widgets[node].theme = theme;
	}

	return node;
}

voe_ui_node voe_ui_number_begin(voe_ui_context *ui, const char *name,
				uint32_t index, double value,
				double per_millimetre)
{
	voe_ui_node node;
	voe_ui_node label = VOE_UI_NODE_NONE;
	uint64_t key;
	bool open;
	const voe_ui_theme *theme = current_theme(ui);

	VOE_BASE_ASSERT(ui != NULL, "opening a number box on no context");
	VOE_BASE_ASSERT(name != NULL,
			"a number box with no name has no identity");
	VOE_BASE_ASSERT(theme != NULL,
			"a number box needs a theme: see "
			"voe_ui_theme_set/voe_ui_theme_push");

	key = claim(ui, name, index);

	// Open when it has the focus and the buffer is not another widget's.
	// Seeded here when the focus came by Tab, which has not seeded it yet,
	// so that it draws open on the very frame after the Tab.
	open = ui->focus_set && ui->focus == key &&
	       (!ui->field_holding || ui->field_owner == key);
	if (open && !ui->field_holding)
		number_seed(ui, key, value);

	// Built exactly as a button is, down to the padding and the centring:
	// what is in it is composed rather than passed, so a caller with no
	// font can still build one and drag it. See widgets.h. Open, its run
	// sits along START, as a field's does, so the typed text begins at the
	// left edge.
	node = voe_ui_row_begin(ui, (voe_ui_container){
					   .along = open ? VOE_UI_ALONG_START
							 : VOE_UI_ALONG_CENTER,
					   .across = VOE_UI_ACROSS_CENTER,
					   .pad = BUTTON_PAD });
	if (node != VOE_UI_NODE_NONE) {
		ui->widgets[node].kind = VOE_UI_WIDGET_NUMBER;
		ui->widgets[node].key = key;
		ui->widgets[node].keyed = true;
		ui->widgets[node].value = value;
		ui->widgets[node].per_millimetre = per_millimetre;
		ui->widgets[node].theme = theme;
		ui->widgets[node].open = open;
	}

	// The composed row comes first, so that everything the caller calls
	// before its voe_ui_end lands after it and is the part not drawn.
	if (open) {
		voe_ui_row_begin(ui, (voe_ui_container){
					     .across = VOE_UI_ACROSS_CENTER,
					     .gap = NUMBER_REFUSED_GAP });
		label = voe_ui_label(ui, ui->field_buffer);
		if (ui->number_refused)
			voe_ui_label_role(ui, "not a number",
					  VOE_UI_TEXT_ROLE_SECONDARY);
		voe_ui_end(ui);
		if (node != VOE_UI_NODE_NONE) {
			ui->number_open_node = node;
			ui->number_open_label = label;
			ui->number_open_end = ui->count;
		}
	}

	return node;
}

voe_ui_node voe_ui_field(voe_ui_context *ui, const char *name, uint32_t index,
			 const char *text, voe_ui_sizing sizing)
{
	voe_ui_node node;
	uint64_t key;
	const voe_ui_theme *theme = current_theme(ui);

	VOE_BASE_ASSERT(ui != NULL, "opening a field on no context");
	VOE_BASE_ASSERT(name != NULL, "a field with no name has no identity");
	VOE_BASE_ASSERT(text != NULL, "a field with no text");
	VOE_BASE_ASSERT(theme != NULL,
			"a field needs a theme: see "
			"voe_ui_theme_set/voe_ui_theme_push");

	key = claim(ui, name, index);

	// Along START rather than centred, unlike a button and a number box:
	// text begins at the left edge of a field and grows rightward, not
	// out from its middle. Otherwise built exactly as they are, down to
	// the padding — see widgets.h.
	node = voe_ui_row_begin(ui, (voe_ui_container){ .size = sizing,
							.along = VOE_UI_ALONG_START,
							.across = VOE_UI_ACROSS_CENTER,
							.pad = BUTTON_PAD });
	if (node != VOE_UI_NODE_NONE) {
		ui->widgets[node].kind = VOE_UI_WIDGET_FIELD;
		ui->widgets[node].key = key;
		ui->widgets[node].keyed = true;
		ui->widgets[node].theme = theme;
		ui->widgets[node].text = text;
	}

	// Made and ended here, whether or not `node` is a real one: a field
	// refused for want of a node is still a balanced begin/end pair, as
	// every other refused container is. THE LABEL IS ALWAYS THE NEXT NODE
	// AFTER THE FIELD'S OWN, which is what lets field_edit and
	// voe_ui_field_action find it from the field's index alone. WHILE THE
	// CONTEXT HOLDS THIS FIELD'S TEXT THE LABEL IS THAT TEXT, so layout
	// measures what is shown and the caller's `text` is not read.
	voe_ui_label(ui, ui->field_holding && ui->field_owner == key
				 ? ui->field_buffer
				 : text);
	voe_ui_end(ui);

	return node;
}

// Takes the keyboard to `field`, as if a press had just landed inside it.
// Called between voe_ui_frame_begin and voe_ui_frame_end, like every other
// call that touches the context's state.
void voe_ui_field_focus(voe_ui_context *ui, voe_ui_node field)
{
	VOE_BASE_ASSERT(ui != NULL, "focusing a field on no context");
	VOE_BASE_ASSERT(ui->state == VOE_UI_BUILDING,
			"focusing a field outside a frame");
	VOE_BASE_ASSERT(field != VOE_UI_NODE_NONE,
			"focusing a field the frame had no room for");
	VOE_BASE_ASSERT(field < ui->count,
			"focusing a field this frame never made");
	VOE_BASE_ASSERT(ui->widgets[field].kind == VOE_UI_WIDGET_FIELD,
			"focusing a node that is not a field");

	ui->focus = ui->widgets[field].key;
	ui->focus_set = true;
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

// Last frame's offset for this key, nought on an axis the area does not scroll
// and on both for a key last frame did not have.
static voe_math_float2 remembered(const voe_ui_context *ui, uint64_t key,
				  voe_ui_scroll_axes axes)
{
	for (uint32_t i = 0; i < ui->scroll_remembered; i++) {
		voe_math_float2 offset = ui->scroll_memory[i].offset;

		if (ui->scroll_memory[i].key != key)
			continue;
		return (voe_math_float2){ axes.x ? offset.x : 0.0f,
					  axes.y ? offset.y : 0.0f };
	}
	return (voe_math_float2){ 0.0f, 0.0f };
}

voe_ui_node voe_ui_scroll_begin(voe_ui_context *ui, const char *name,
				uint32_t index, voe_ui_container container,
				voe_ui_scroll_axes axes)
{
	voe_math_float2 handed;
	voe_ui_node node;
	uint64_t key;
	const voe_ui_theme *theme = current_theme(ui);

	VOE_BASE_ASSERT(ui != NULL, "opening a scroll area on no context");
	VOE_BASE_ASSERT(name != NULL,
			"a scroll area with no name has no identity");
	VOE_BASE_ASSERT(theme != NULL,
			"a scroll area needs a theme, its bar may be drawn "
			"later in this very frame: see "
			"voe_ui_theme_set/voe_ui_theme_push");
	VOE_BASE_ASSERT(container.overflow.x == VOE_UI_OVERFLOW_VISIBLE &&
				container.overflow.y == VOE_UI_OVERFLOW_VISIBLE,
			"a scroll area's overflow is its own; it clips on both "
			"axes, so leave `overflow` zeroed");
	VOE_BASE_ASSERT(container.scroll.x == 0.0f && container.scroll.y == 0.0f,
			"a scroll area's offset is its own and remembered under "
			"its key, so leave `scroll` zeroed");

	key = claim(ui, name, index);
	handed = remembered(ui, key, axes);

	container.overflow = (voe_ui_overflow){ VOE_UI_OVERFLOW_CLIP,
						VOE_UI_OVERFLOW_CLIP };
	container.scroll = handed;
	node = voe_ui_column_begin(ui, container);

	// Refused as a node past capacity is: named once, the frame carries on
	// balanced, and frame_end says no. The column is still open, so the
	// caller's voe_ui_end still matches it.
	if (ui->scroll_count == ui->capacities.scrolls) {
		if (!ui->scroll_overrun)
			VOE_BASE_ERROR("ui",
				       "scroll area %u refused, this context was "
				       "created with room for %u",
				       ui->scroll_count + 1,
				       ui->capacities.scrolls);
		ui->scroll_overrun = true;
	} else {
		ui->scroll_areas[ui->scroll_count++] =
			(struct voe_ui_scroll_area){ .key = key,
						     .node = node,
						     .axes = axes,
						     .handed = handed };
	}

	if (node != VOE_UI_NODE_NONE) {
		ui->widgets[node].kind = VOE_UI_WIDGET_SCROLL;
		ui->widgets[node].key = key;
		ui->widgets[node].keyed = true;
		ui->widgets[node].theme = theme;
	}

	return node;
}

// ------------------------------------------------------------- the frame

void voe_ui_widgets_init(voe_ui_context *ui, voe_base_arena *arena)
{
	// A context that asked for no scroll areas remembers none, and an arena
	// push of nothing has no caller.
	ui->scroll_memory = NULL;
	if (ui->capacities.scrolls > 0)
		ui->scroll_memory = voe_base_arena_push(
			arena, (size_t)ui->capacities.scrolls *
				       sizeof(*ui->scroll_memory));
	ui->scroll_remembered = 0;
}

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

// Which widgets answer the pointer at all. A panel, a label and an image do not:
// a panel is a background, a label is a measurement and an image is a picture,
// and none of them has ever been asked what the mouse is doing to it. A field
// does, because a press is what focuses one; so do a colour picker's square
// and strip, because a press held there is what sets the colour.
static bool takes_the_pointer(enum voe_ui_widget kind)
{
	return kind == VOE_UI_WIDGET_BUTTON || kind == VOE_UI_WIDGET_NUMBER ||
	       kind == VOE_UI_WIDGET_FIELD ||
	       kind == VOE_UI_WIDGET_COLOUR_SQUARE ||
	       kind == VOE_UI_WIDGET_COLOUR_HUE;
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

// How far an area's content reaches past its rectangle on one axis, and nought
// where it does not. The range layout clamped the offset to.
static float scroll_range(const voe_ui_context *ui, uint32_t node, bool y)
{
	const struct voe_ui_node_record *n = &ui->nodes[node];
	float range =
		component(n->content_natural, y) - component(n->rect.size, y);

	return range > 0.0f ? range : 0.0f;
}

static float clamp_offset(float offset, float range)
{
	if (offset > range)
		offset = range;
	if (offset < 0.0f)
		offset = 0.0f;
	return offset;
}

static bool bar_shows(const voe_ui_context *ui,
		      const struct voe_ui_scroll_area *area, bool y)
{
	return scrolls_on(area->axes, y) && scroll_range(ui, area->node, y) > 0.0f;
}

// Where one of an area's bars is this frame, unclipped: `y` is the bar that
// scrolls Y, along the right edge, and otherwise the one along the bottom.
struct voe_ui_scrollbar {
	bool shows;
	voe_ui_rect track;
	voe_ui_rect thumb;
};

static struct voe_ui_scrollbar scrollbar(const voe_ui_context *ui,
					 const struct voe_ui_scroll_area *area,
					 bool y)
{
	const struct voe_ui_node_record *n = &ui->nodes[area->node];
	struct voe_ui_scrollbar bar = { 0 };
	float measured = component(n->content_natural, y);
	float arranged = component(n->rect.size, y);
	float track;
	float thumb;

	if (!bar_shows(ui, area, y))
		return bar;

	// Short of the corner by the other bar's thickness when both show, so
	// the two never overlap and neither is under the other for the pointer.
	track = arranged - (bar_shows(ui, area, !y) ? SCROLL_THICKNESS : 0.0f);
	if (track < 0.0f)
		track = 0.0f;
	// Measured is greater than arranged here, and so greater than nought.
	thumb = track * arranged / measured;
	if (thumb < SCROLL_THUMB_MIN)
		thumb = SCROLL_THUMB_MIN;
	if (thumb > track)
		thumb = track;

	bar.shows = true;
	component_set(&bar.track.min, y, component(n->rect.min, y));
	component_set(&bar.track.size, y, track);
	component_set(&bar.track.min, !y,
		      component(n->rect.min, !y) + component(n->rect.size, !y) -
			      SCROLL_THICKNESS);
	component_set(&bar.track.size, !y, SCROLL_THICKNESS);

	// Proportional to the offset over the range, so the thumb's far end
	// meets the track's at the content's end whatever the minimum did.
	bar.thumb = bar.track;
	component_set(&bar.thumb.min, y,
		      component(n->rect.min, y) +
			      (track - thumb) * component(n->scrolled, y) /
				      scroll_range(ui, area->node, y));
	component_set(&bar.thumb.size, y, thumb);

	return bar;
}

// The next scroll area below index `from` whose bar paints straight after paint
// position `at`, or NO_AREA. Counting down puts an inner area before the outer
// one when both end at the same position. See this file's header.
static uint32_t bar_after(const voe_ui_context *ui, uint32_t at, uint32_t from)
{
	while (from-- > 0) {
		const struct voe_ui_node_record *n =
			&ui->nodes[ui->scroll_areas[from].node];

		if (n->paint + n->subtree - 1 == at)
			return from;
	}
	return NO_AREA;
}

// What the pointer is on, the last in paint order winning.
enum voe_ui_hit_kind {
	VOE_UI_HIT_NOTHING = 0,
	VOE_UI_HIT_WIDGET,
	VOE_UI_HIT_THUMB,
	VOE_UI_HIT_TRACK,
};

struct voe_ui_hit {
	enum voe_ui_hit_kind kind;
	// A widget's.
	uint64_t key;
	bool number;
	// Whether the widget hit is a field, which is what a press focuses —
	// see resolve.
	bool field;
	// A bar's: its area, which of the two, and for a track which way the
	// thumb is from the pointer, -1 or +1.
	uint32_t area;
	enum voe_ui_bar bar;
	float towards;
};

// One of an area's bars against the pointer, tested against what of it is seen.
static void hit_bar(const voe_ui_context *ui, uint32_t area, bool y,
		    struct voe_ui_hit *hit)
{
	const struct voe_ui_scroll_area *a = &ui->scroll_areas[area];
	voe_ui_rect visible = ui->nodes[a->node].visible;
	struct voe_ui_scrollbar bar = scrollbar(ui, a, y);
	voe_math_float2 at = ui->pointer.at;

	if (!bar.shows || !inside(intersect(bar.track, visible), at))
		return;

	*hit = (struct voe_ui_hit){
		.kind = inside(intersect(bar.thumb, visible), at)
				? VOE_UI_HIT_THUMB
				: VOE_UI_HIT_TRACK,
		.area = area,
		.bar = y ? VOE_UI_BAR_Y : VOE_UI_BAR_X,
		.towards = component(at, y) < component(bar.thumb.min, y)
				   ? -1.0f
				   : 1.0f,
	};
}

// Whether `node` is the caller's own content of the number box open for
// typing, which is neither drawn nor hit while it is open. It is the tail of
// that box's subtree, after what the box composed itself — see
// voe_ui_number_begin.
static bool hidden(const voe_ui_context *ui, uint32_t node)
{
	uint32_t open = ui->number_open_node;

	return open != VOE_UI_NODE_NONE && node >= ui->number_open_end &&
	       node < open + ui->nodes[open].subtree;
}

static struct voe_ui_hit hit_test(const voe_ui_context *ui)
{
	struct voe_ui_hit hit = { 0 };

	// Every hit, not the first: the last one in paint order is the one in
	// front, and that is the one the pointer is on.
	for (uint32_t at = 0; at < ui->count; at++) {
		uint32_t node = voe_ui_paint_order(ui, at);

		if (takes_the_pointer(ui->widgets[node].kind) &&
		    !hidden(ui, node) &&
		    inside(ui->nodes[node].visible, ui->pointer.at))
			hit = (struct voe_ui_hit){
				.kind = VOE_UI_HIT_WIDGET,
				.key = ui->widgets[node].key,
				.number = ui->widgets[node].kind ==
					  VOE_UI_WIDGET_NUMBER,
				.field = ui->widgets[node].kind ==
					 VOE_UI_WIDGET_FIELD,
			};

		for (uint32_t s = bar_after(ui, at, ui->scroll_count);
		     s != NO_AREA; s = bar_after(ui, at, s)) {
			hit_bar(ui, s, false, &hit);
			hit_bar(ui, s, true, &hit);
		}
	}
	return hit;
}

static void resolve(voe_ui_context *ui, bool laid_out)
{
	struct voe_ui_hit hit = { 0 };
	uint64_t hovered;
	bool hovered_set;
	bool hovered_number;

	ui->fired_set = false;
	ui->number_delta = 0.0f;
	ui->number_moved = false;
	ui->hovered_thumb = VOE_UI_BAR_NONE;
	ui->page = VOE_UI_BAR_NONE;

	if (laid_out && ui->pointer.over)
		hit = hit_test(ui);

	hovered_set = hit.kind == VOE_UI_HIT_WIDGET;
	hovered = hovered_set ? hit.key : 0;
	hovered_number = hovered_set && hit.number;

	// A focus whose field was not called this frame is dropped, exactly as
	// an area not called forgets its offset — and this runs whether or not
	// the frame laid out, because `claim` marks every key a call took
	// regardless of refusal, so the check is meaningful either way.
	if (ui->focus_set && !key_taken(ui, ui->focus))
		ui->focus_set = false;
	if (ui->field_holding && !key_taken(ui, ui->field_owner))
		ui->field_holding = false;

	if (!laid_out) {
		ui->hovered_set = false;
		ui->held_set = false;
		ui->held_number = false;
		ui->held_thumb = VOE_UI_BAR_NONE;
		ui->was_down = ui->pointer.down;
		return;
	}

	if (hit.kind == VOE_UI_HIT_THUMB) {
		ui->hovered_thumb = hit.bar;
		ui->hovered_thumb_area = hit.area;
	}

	// A held widget the frame did not build is no longer held, and this is
	// asked before the press and the release so that a release cannot fire
	// something that is no longer there. A thumb's key is its area's, so a
	// thumb whose area was not built is let go by the same line.
	if (ui->held_set && !key_taken(ui, ui->held)) {
		ui->held_set = false;
		ui->held_number = false;
		ui->held_thumb = VOE_UI_BAR_NONE;
	}

	if (ui->pointer.down && !ui->was_down) {
		// The edge, and only the edge: dragging onto a button with the
		// mouse already down arms nothing.
		ui->held = hovered;
		ui->held_set = hovered_set;
		ui->held_number = hovered_set && hovered_number;
		ui->held_thumb = VOE_UI_BAR_NONE;
		if (hit.kind == VOE_UI_HIT_THUMB) {
			const struct voe_ui_scroll_area *a =
				&ui->scroll_areas[hit.area];
			bool y = hit.bar == VOE_UI_BAR_Y;

			ui->held = a->key;
			ui->held_set = true;
			ui->held_thumb = hit.bar;
			ui->thumb_press_at = ui->pointer.at;
			ui->thumb_press_offset =
				component(ui->nodes[a->node].scrolled, y);
		} else if (hit.kind == VOE_UI_HIT_TRACK) {
			// Holds nothing, and pages once: the press is the edge.
			ui->page = hit.bar;
			ui->page_area = hit.area;
			ui->page_towards = hit.towards;
		}
		// Where this gesture began. Written whatever was armed, so
		// that a press on a button leaves nothing behind for the next
		// number box to inherit.
		ui->number_press_x = ui->pointer.at.x;
		ui->number_last_x = ui->pointer.at.x;
		ui->number_crossed = false;

		// FOCUS FOLLOWS THE PRESS ITSELF AND NOT A RELEASE, unlike
		// `held`/`fired`: a press inside a field focuses it and a press
		// ANYWHERE ELSE — nothing, a button, a bar — clears it, on the
		// same edge. See widgets.h. A press inside the number box open
		// for typing is not elsewhere: it keeps the focus, and arms no
		// drag, the box being a field until it closes.
		bool on_focus = hit.kind == VOE_UI_HIT_WIDGET &&
				ui->focus_set && ui->focus == hovered;

		if (on_focus && hit.number) {
			ui->held_set = false;
			ui->held_number = false;
		}
		if (hit.kind == VOE_UI_HIT_WIDGET && (hit.field || on_focus)) {
			ui->focus = hovered;
			ui->focus_set = true;
		} else {
			ui->focus_set = false;
		}
	} else if (!ui->pointer.down && ui->was_down) {
		// A NUMBER BOX NEVER FIRES, which is why `held_number` is asked
		// here. The release that would have fired a button is the click
		// that opens one for typing instead: over the box, and never
		// out of the dead zone — not by a drag, and not by where the
		// release itself happened. See widgets.h.
		if (ui->held_set && ui->held_number && !ui->number_crossed &&
		    hovered_set && hovered == ui->held &&
		    fabsf(ui->pointer.at.x - ui->number_press_x) <
			    VOE_UI_NUMBER_DEAD_ZONE) {
			ui->focus = ui->held;
			ui->focus_set = true;
		}
		if (ui->held_set && !ui->held_number && hovered_set &&
		    hovered == ui->held) {
			ui->fired = ui->held;
			ui->fired_set = true;
		}
		ui->held_set = false;
		ui->held_number = false;
		ui->held_thumb = VOE_UI_BAR_NONE;
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

// The widgets the keyboard focus can go to: a field, and a number box, which
// is typed into once it is open (ADR-0192).
static bool typeable_kind(enum voe_ui_widget kind)
{
	return kind == VOE_UI_WIDGET_FIELD || kind == VOE_UI_WIDGET_NUMBER;
}

// The node of the field or number box keyed `key` this frame, or
// VOE_UI_NODE_NONE.
static uint32_t typeable_of(const voe_ui_context *ui, uint64_t key)
{
	for (uint32_t i = 0; i < ui->count; i++)
		if (typeable_kind(ui->widgets[i].kind) &&
		    ui->widgets[i].key == key)
			return i;
	return VOE_UI_NODE_NONE;
}

// Focus leaves the field whose text the context holds, keeping that text:
// copied to `field_final`, shown by its label this frame and handed back as
// its `text`, and the field marked committed.
static void field_commit(voe_ui_context *ui, uint32_t field)
{
	memcpy(ui->field_final, ui->field_buffer, sizeof(ui->field_final));
	ui->widgets[field + 1].text = ui->field_final;
	ui->widgets[field].committed = true;
	ui->field_holding = false;
}

// The first field or number box made after `node` in this frame, wrapping to
// the first of all — `node` itself when it is the only one. Call order is node
// order, which is what Tab follows (ADR-0192).
static uint32_t typeable_after(const voe_ui_context *ui, uint32_t node)
{
	for (uint32_t step = 1; step <= ui->count; step++) {
		uint32_t i = (node + step) % ui->count;

		if (typeable_kind(ui->widgets[i].kind))
			return i;
	}
	return node;
}

// A number box's commit: true when it is taken, false when it is refused. The
// text it opened with, unchanged, is taken and changes nothing; otherwise the
// text must be one finite number, which becomes the box's `value` with
// `changed` set.
static bool number_commit(voe_ui_context *ui, uint32_t number)
{
	double typed;

	if (strcmp(ui->field_buffer, ui->number_opened) == 0)
		return true;
	if (!number_parse(ui->field_buffer, &typed))
		return false;
	ui->widgets[number].changed = true;
	ui->widgets[number].value = typed;
	return true;
}

// A number box closing: the context stops holding its text. When it was built
// open this frame its label is pointed at a copy, because a press into a field
// may seed the buffer again in this same frame.
static void number_close(voe_ui_context *ui, uint32_t number)
{
	memcpy(ui->field_final, ui->field_buffer, sizeof(ui->field_final));
	if (ui->number_open_node == number)
		ui->widgets[ui->number_open_label].text = ui->field_final;
	ui->field_holding = false;
	ui->number_refused = false;
}

// What this frame's keyboard does to the focused field or number box, once
// focus is settled — which is why this runs after resolve rather than inside it: a
// press this same frame can move focus to a field before its first
// keystroke, and the field found here is the one that press produced.
//
// IN THIS ORDER: a field that lost the focus since the buffer was seeded
// commits; the newly focused one is seeded from its caller's text with all
// of it selected; Escape cancels and nothing else is read; Backspace, then
// typing, edit; Enter or Tab commits, and Tab hands the focus to the next
// field, which is seeded the next frame. The label is pointed at whichever
// text this leaves the field showing, so a letter typed shows in the frame
// that read it.
//
// A NUMBER BOX GOES THROUGH THE SAME STEPS with its own commit: seeded with
// `%.6g`, edited as a field is, and on Enter or Tab parsed — a refused parse
// keeps it open and Tab where it is. Losing the focus to a press elsewhere
// commits it as Enter would, and a refused one then closes changing nothing,
// the press having already gone to something else. Escape closes it.
//
// THE LABEL IS ALWAYS THE NODE RIGHT AFTER THE FIELD'S OWN — see
// voe_ui_field — so once the field is found there is no second table to
// consult for its text.
static void field_edit(voe_ui_context *ui)
{
	char before[VOE_UI_FIELD_CAPACITY + 1];
	uint32_t field;
	bool number;
	bool typed = false;

	if (ui->field_holding &&
	    (!ui->focus_set || ui->focus != ui->field_owner)) {
		field = typeable_of(ui, ui->field_owner);
		if (field != VOE_UI_NODE_NONE &&
		    ui->widgets[field].kind == VOE_UI_WIDGET_FIELD) {
			field_commit(ui, field);
		} else if (field != VOE_UI_NODE_NONE) {
			(void)number_commit(ui, field);
			number_close(ui, field);
		}
		ui->field_holding = false;
	}
	if (!ui->focus_set)
		return;
	field = typeable_of(ui, ui->focus);
	if (field == VOE_UI_NODE_NONE)
		return;
	number = ui->widgets[field].kind == VOE_UI_WIDGET_NUMBER;

	if (!ui->field_holding && number) {
		number_seed(ui, ui->focus, ui->widgets[field].value);
	} else if (!ui->field_holding) {
		// Copied defensively rather than trusted: the buffer is fixed
		// and a caller handing in more than VOE_UI_FIELD_CAPACITY
		// bytes is a bug this folder does not crash over.
		size_t len = strlen(ui->widgets[field].text);

		if (len > VOE_UI_FIELD_CAPACITY)
			len = VOE_UI_FIELD_CAPACITY;
		memcpy(ui->field_buffer, ui->widgets[field].text, len);
		ui->field_buffer[len] = '\0';
		ui->field_owner = ui->focus;
		ui->field_holding = true;
		ui->field_selected = true;
	}
	if (!number)
		ui->widgets[field + 1].text = ui->field_buffer;

	if (ui->keyboard.escape && number) {
		number_close(ui, field);
		ui->focus_set = false;
		return;
	}
	if (ui->keyboard.escape) {
		ui->widgets[field].cancelled = true;
		ui->widgets[field + 1].text = ui->widgets[field].text;
		ui->field_holding = false;
		ui->focus_set = false;
		return;
	}

	memcpy(before, ui->field_buffer, sizeof(before));
	for (uint32_t i = 0; i < ui->keyboard.size; i++)
		typed = typed || typeable((unsigned char)ui->keyboard.text[i]);
	// THE WHOLE TEXT SELECTED IS WHAT THE FIRST EDIT REPLACES: Backspace
	// empties it and typing starts it again, and either ends the selection.
	if (ui->field_selected && (ui->keyboard.backspace || typed)) {
		ui->field_buffer[0] = '\0';
		ui->field_selected = false;
	} else if (ui->keyboard.backspace) {
		field_backspace(ui->field_buffer);
	}
	if (typed)
		field_append(ui->field_buffer, ui->keyboard.text,
			     ui->keyboard.size);
	// A number box's `changed` is a commit taken and not an edit; what an
	// edit does to one is take "not a number" away until the next refusal.
	if (number && strcmp(before, ui->field_buffer) != 0)
		ui->number_refused = false;
	else if (!number)
		ui->widgets[field].changed =
			strcmp(before, ui->field_buffer) != 0;

	if (!ui->keyboard.enter && !ui->keyboard.tab)
		return;
	if (number && !number_commit(ui, field)) {
		ui->widgets[field].refused = true;
		ui->number_refused = true;
		return;
	}
	if (number) {
		number_close(ui, field);
	} else {
		ui->widgets[field].entered = ui->keyboard.enter;
		field_commit(ui, field);
	}
	ui->focus_set = ui->keyboard.tab;
	if (ui->keyboard.tab)
		ui->focus = ui->widgets[typeable_after(ui, field)].key;
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

static voe_math_float4 bounds_of(voe_ui_rect rect)
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
	voe_math_float4 bounds = bounds_of(at);
	voe_math_float4 clip;

	if (!clip_of(ui, node, bounds, &clip))
		return;

	push_element(ui, (voe_render_element){
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
static void push_rect(voe_ui_context *ui, uint32_t node,
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
static void push_bordered(voe_ui_context *ui, uint32_t node,
			  voe_math_float4 fill, voe_math_float4 border)
{
	push_rect(ui, node, border);
	push_rect_at(ui, node, inset(ui->nodes[node].rect, HAIRLINE_WIDE),
		    fill);
}

// A picture over the node's rectangle, clipped as push_rect's record is. The
// sheet and the texture index go across as the caller handed them in.
static void push_image(voe_ui_context *ui, uint32_t node)
{
	const struct voe_ui_widget_record *w = &ui->widgets[node];
	voe_math_float4 bounds = bounds_of(ui->nodes[node].rect);
	voe_math_float4 clip;

	if (!clip_of(ui, node, bounds, &clip))
		return;

	push_element(ui, (voe_render_element){
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

	push_element(ui, (voe_render_element){
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

// The three states, for a button and for a number box alike — control at
// rest, control_hovered under the pointer, accent while held (ADR-0171: a
// pressed control is the accent and there is no fourth role for it). They
// look the same on purpose — see BUTTON_PAD.
static voe_math_float4 state_colour(const voe_ui_context *ui, uint32_t node)
{
	const struct voe_ui_widget_record *w = &ui->widgets[node];

	// Held beats hovered, because a button being pressed is what a person
	// is doing and hovering is only where the pointer happens to be. A
	// button held with the pointer dragged off it stays in its held colour,
	// which is what says the press is still live and can still be
	// completed by coming back.
	if (ui->held_set && ui->held == w->key)
		return w->theme->accent;
	if (ui->hovered_set && ui->hovered == w->key)
		return w->theme->control_hovered;
	return w->theme->control;
}

// A field's own four states. Held still beats everything, a press in
// progress being what a person is doing right now; focused beats hovered and
// normal, a field being focused being worth seeing whether or not the
// pointer still happens to be over it.
//
// DEVIATION: ADR-0171 names control, control_hovered and the accent, and no
// fourth role for a field's own "focused" state — nothing before this theme
// existed asked for one. Read narrowly as surface_raised, a surface already
// meant to read as sitting above its neighbour, because the alternative of
// reusing control_hovered would make a focused, unhovered field look exactly
// like one the pointer merely sits over, losing the distinction the field's
// four states had before this task.
static voe_math_float4 field_colour(const voe_ui_context *ui, uint32_t node)
{
	const struct voe_ui_widget_record *w = &ui->widgets[node];

	if (ui->held_set && ui->held == w->key)
		return w->theme->accent;
	if (ui->focus_set && ui->focus == w->key)
		return w->theme->surface_raised;
	if (ui->hovered_set && ui->hovered == w->key)
		return w->theme->control_hovered;
	return w->theme->control;
}

// The caret: FIELD_CARET_WIDE at the right edge of the focused field's — or
// open number box's — composed label, as tall as that label's own rectangle,
// in the widget's own theme's text_primary. An empty label measures to
// nothing, so its rectangle has no width and the caret sits at the left of the
// content box, exactly where the next letter typed will begin.
static void push_caret(voe_ui_context *ui, uint32_t node, uint32_t text)
{
	voe_ui_rect label = ui->nodes[text].rect;
	voe_math_float4 bounds = { label.min.x + label.size.x, label.min.y,
				   FIELD_CARET_WIDE, label.size.y };
	voe_math_float4 clip;

	if (!clip_of(ui, node, bounds, &clip))
		return;

	push_element(ui, (voe_render_element){
				 .bounds = bounds,
				 .clip = clip,
				 .colour = ui->widgets[node].theme->text_primary,
				 .kind = VOE_RENDER_ELEMENT_SOLID,
			 });
}

// A solid record over `bounds`, clipped to `within`, and not pushed when nothing
// of it is left.
static void push_solid_within(voe_ui_context *ui, voe_ui_rect bounds,
			      voe_ui_rect within, voe_math_float4 colour)
{
	voe_ui_rect clip = intersect(bounds, within);

	if (clip.size.x <= 0.0f || clip.size.y <= 0.0f)
		return;

	push_element(ui, (voe_render_element){
				 .bounds = bounds_of(bounds),
				 .clip = bounds_of(clip),
				 .colour = colour,
				 .kind = VOE_RENDER_ELEMENT_SOLID,
			 });
}

// One of an area's bars, track and then thumb, clipped to the area's visible
// rectangle. The track is `ground`, the thumb a button's own three states —
// control, control_hovered, and the accent while held. Held beats hovered, as
// it does on a button.
static void push_scrollbar(voe_ui_context *ui, uint32_t area, bool y)
{
	const struct voe_ui_scroll_area *a = &ui->scroll_areas[area];
	const voe_ui_theme *theme = ui->widgets[a->node].theme;
	struct voe_ui_scrollbar bar = scrollbar(ui, a, y);
	enum voe_ui_bar which = y ? VOE_UI_BAR_Y : VOE_UI_BAR_X;
	voe_ui_rect visible = ui->nodes[a->node].visible;
	voe_math_float4 thumb = theme->control;

	if (!bar.shows)
		return;

	if (ui->held_set && ui->held_thumb == which && ui->held == a->key)
		thumb = theme->accent;
	else if (ui->hovered_thumb == which && ui->hovered_thumb_area == area)
		thumb = theme->control_hovered;

	push_solid_within(ui, bar.track, visible, theme->ground);
	push_solid_within(ui, bar.thumb, visible, thumb);
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

// The field or open number box whose composed label `label` is, or
// VOE_UI_NODE_NONE. A field's is the node right after it — see voe_ui_field —
// and an open number box's is recorded when it is built.
static uint32_t label_owner(const voe_ui_context *ui, uint32_t label)
{
	if (label > 0 && ui->widgets[label - 1].kind == VOE_UI_WIDGET_FIELD)
		return label - 1;
	if (label == ui->number_open_label)
		return ui->number_open_node;
	return VOE_UI_NODE_NONE;
}

static void emit(voe_ui_context *ui)
{
	for (uint32_t at = 0; at < ui->count; at++) {
		uint32_t node = voe_ui_paint_order(ui, at);
		uint32_t owner;

		// An open number box's own content is not drawn: nothing
		// emitted, as a plain row emits nothing.
		switch (hidden(ui, node) ? VOE_UI_WIDGET_NONE
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
				push_bordered(ui, node,
					     surface_colour(ui->widgets[node].theme,
							    ui->widgets[node].surface),
					     ui->widgets[node].theme->border);
			break;
		case VOE_UI_WIDGET_BUTTON:
			push_bordered(ui, node, state_colour(ui, node),
				     ui->widgets[node].theme->border);
			break;
		case VOE_UI_WIDGET_NUMBER:
			// OPEN FOR TYPING IT DRAWS AS A FIELD DOES: a field's
			// colours, and the whole text selected as the accent
			// behind its composed label.
			if (!ui->widgets[node].open) {
				push_rect(ui, node, state_colour(ui, node));
				break;
			}
			push_rect(ui, node, field_colour(ui, node));
			if (ui->field_holding && ui->field_selected &&
			    ui->field_owner == ui->widgets[node].key)
				push_rect(ui, ui->number_open_label,
					  ui->widgets[node].theme->accent);
			break;
		case VOE_UI_WIDGET_FIELD:
			push_rect(ui, node, field_colour(ui, node));
			// THE WHOLE TEXT SELECTED IS THE ACCENT BEHIND IT: the
			// label's own rectangle, before the label's glyphs.
			if (ui->field_holding && ui->field_selected &&
			    ui->field_owner == ui->widgets[node].key)
				push_rect(ui, node + 1,
					  ui->widgets[node].theme->accent);
			break;
		case VOE_UI_WIDGET_LABEL:
			push_label(ui, node);
			// THE CARET COMES AFTER THE LABEL'S OWN GLYPHS, so it
			// paints in front of them rather than under them, which
			// is why it is pushed here and not beside the field's
			// own background above. See label_owner for which
			// labels are a field's or an open number box's.
			owner = label_owner(ui, node);
			if (owner != VOE_UI_NODE_NONE && ui->focus_set &&
			    ui->focus == ui->widgets[owner].key)
				push_caret(ui, owner, node);
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
		// area's subtree when this is it. See this file's header.
		for (uint32_t s = bar_after(ui, at, ui->scroll_count);
		     s != NO_AREA; s = bar_after(ui, at, s)) {
			push_scrollbar(ui, s, false);
			push_scrollbar(ui, s, true);
		}
	}
}

// ------------------------------------------------------ moving a scroll area

// This frame's areas written back as the table, in call order, so that entry i
// of both is one area from here to the next frame_begin. See this file's header,
// and its DEVIATION, for what a refused frame writes.
static void scrolls_remember(voe_ui_context *ui, bool laid_out)
{
	for (uint32_t i = 0; i < ui->scroll_count; i++) {
		const struct voe_ui_scroll_area *a = &ui->scroll_areas[i];

		ui->scroll_memory[i] = (struct voe_ui_scroll_memory){
			.key = a->key,
			.offset = laid_out ? ui->nodes[a->node].scrolled
					   : a->handed,
		};
	}
	ui->scroll_remembered = ui->scroll_count;
}

// The nearest area around `area`, or NO_AREA. Areas are in call order, so one
// that holds this one comes before it, and holding is a subtree range.
static uint32_t outward(const voe_ui_context *ui, uint32_t area)
{
	uint32_t inner = ui->scroll_areas[area].node;

	while (area-- > 0) {
		uint32_t outer = ui->scroll_areas[area].node;

		if (outer < inner && inner < outer + ui->nodes[outer].subtree)
			return area;
	}
	return NO_AREA;
}

// Scrolls by a length, starting at `area`: each area takes what its clamp allows
// on each axis it scrolls and passes the rest outward, and what nobody takes is
// dropped. Into the table, so it lands in the next frame's layout.
//
// THE ONE PATH A LENGTH TAKES. The pointer's scroll comes through here today, and
// voe_ui_scroll_by is this with a node for `area` once focus gives it a caller.
static void scroll_by(voe_ui_context *ui, uint32_t area, voe_math_float2 length)
{
	for (int axis = 0; axis < 2; axis++) {
		bool y = axis == 1;
		float left = component(length, y);

		for (uint32_t s = area; s != NO_AREA && left != 0.0f;
		     s = outward(ui, s)) {
			const struct voe_ui_scroll_area *a = &ui->scroll_areas[s];
			float wanted;
			float kept;

			if (!scrolls_on(a->axes, y))
				continue;

			wanted = component(ui->scroll_memory[s].offset, y) + left;
			kept = clamp_offset(wanted, scroll_range(ui, a->node, y));
			component_set(&ui->scroll_memory[s].offset, y, kept);
			// Nought exactly when nothing was clamped away.
			left = wanted - kept;
		}
	}
}

// The innermost area whose visible rectangle holds the pointer — the last in
// paint order, which for nested areas is the inner — or NO_AREA.
static uint32_t area_under_pointer(const voe_ui_context *ui)
{
	uint32_t best = NO_AREA;

	for (uint32_t i = 0; i < ui->scroll_count; i++) {
		const struct voe_ui_node_record *n =
			&ui->nodes[ui->scroll_areas[i].node];

		if (!inside(n->visible, ui->pointer.at))
			continue;
		if (best == NO_AREA ||
		    n->paint > ui->nodes[ui->scroll_areas[best].node].paint)
			best = i;
	}
	return best;
}

// The held thumb's area follows the pointer's travel since the press, times
// measured over arranged.
static void thumb_drag(voe_ui_context *ui)
{
	bool y = ui->held_thumb == VOE_UI_BAR_Y;

	for (uint32_t i = 0; i < ui->scroll_count; i++) {
		const struct voe_ui_scroll_area *a = &ui->scroll_areas[i];
		const struct voe_ui_node_record *n = &ui->nodes[a->node];
		float arranged = component(n->rect.size, y);
		float measured = component(n->content_natural, y);
		float travel;
		float offset;

		if (a->key != ui->held)
			continue;
		// Nothing arranged has nothing to be a proportion of.
		if (arranged <= 0.0f)
			return;

		travel = component(ui->pointer.at, y) -
			 component(ui->thumb_press_at, y);
		offset = ui->thumb_press_offset + travel * measured / arranged;
		component_set(&ui->scroll_memory[i].offset, y,
			      clamp_offset(offset, scroll_range(ui, a->node, y)));
		return;
	}
}

// A track pressed this frame: one arranged length towards the pointer.
static void page(voe_ui_context *ui)
{
	const struct voe_ui_scroll_area *a = &ui->scroll_areas[ui->page_area];
	bool y = ui->page == VOE_UI_BAR_Y;
	float offset = component(ui->scroll_memory[ui->page_area].offset, y) +
		       ui->page_towards *
			       component(ui->nodes[a->node].rect.size, y);

	component_set(&ui->scroll_memory[ui->page_area].offset, y,
		      clamp_offset(offset, scroll_range(ui, a->node, y)));
}

static void scrolls_move(voe_ui_context *ui)
{
	uint32_t under;

	if (ui->held_set && ui->held_thumb != VOE_UI_BAR_NONE)
		thumb_drag(ui);
	if (ui->page != VOE_UI_BAR_NONE)
		page(ui);

	if (!ui->pointer.over ||
	    (ui->pointer.scroll.x == 0.0f && ui->pointer.scroll.y == 0.0f))
		return;
	under = area_under_pointer(ui);
	if (under != NO_AREA)
		scroll_by(ui, under, ui->pointer.scroll);
}

void voe_ui_widgets_frame_end(voe_ui_context *ui, bool laid_out)
{
	// Asked before resolve, which is what moves `was_down` on.
	bool pressed = ui->pointer.down && !ui->was_down;

	resolve(ui, laid_out);
	scrolls_remember(ui, laid_out);
	if (laid_out) {
		// After remembering, so every move is on top of the offset this
		// frame was laid out at and lands in the next frame's layout.
		scrolls_move(ui);
		// After resolve has settled which field, if any, is focused —
		// see field_edit — and before emission, which is what draws
		// the text field_edit just wrote.
		field_edit(ui);
	}
	// After field_edit, so a hex field's commit is settled, and before
	// emission; a refused frame's pickers answer with what they were given.
	voe_ui_colour_frame_end(ui, laid_out, pressed);
	if (laid_out)
		emit(ui);
	ui->typing = ui->focus_set;
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
		// A typed commit taken this frame, when `value` is the number
		// typed — see number_commit.
		.changed = w->changed,
		// The value handed in, unchanged, which is the answer on every
		// frame but the ones a drag or a typed commit moved it.
		.value = w->value,
		.typing = ui->focus_set && ui->focus == w->key,
		.refused = w->refused,
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

voe_ui_field_result voe_ui_field_action(const voe_ui_context *ui,
					voe_ui_node field)
{
	uint64_t key;
	bool focused;

	VOE_BASE_ASSERT(ui != NULL, "reading a field on no context");
	VOE_BASE_ASSERT(ui->state == VOE_UI_LAID_OUT,
			"reading a field before the frame has ended; nothing "
			"has a rectangle until then and so nothing has been "
			"hit tested");
	VOE_BASE_ASSERT(field != VOE_UI_NODE_NONE,
			"reading a field the frame had no room for");
	VOE_BASE_ASSERT(field < ui->count,
			"reading a field this frame never made");
	VOE_BASE_ASSERT(ui->widgets[field].kind == VOE_UI_WIDGET_FIELD,
			"reading a field action from a node that is not a "
			"field");

	key = ui->widgets[field].key;
	focused = ui->focus_set && ui->focus == key;

	return (voe_ui_field_result){
		.focused = focused,
		.changed = ui->widgets[field].changed,
		.entered = ui->widgets[field].entered,
		.committed = ui->widgets[field].committed,
		.cancelled = ui->widgets[field].cancelled,
		// The composed label's own text: field_edit pointed it at the
		// context's buffer while focused, at the committed text on the
		// frame focus left, and left it as the caller's otherwise.
		.text = ui->widgets[field + 1].text,
	};
}

bool voe_ui_typing(const voe_ui_context *ui)
{
	VOE_BASE_ASSERT(ui != NULL, "asking whether no context is typing");

	return ui->typing;
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
