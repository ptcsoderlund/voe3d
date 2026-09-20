// The single-line text field, the one keyboard focus this folder holds, and
// typing into the number box a click has opened. See include/ui/widgets.h for
// the promises; widgets.c makes the keys, the frame and the records this is
// built out of, button.c makes the number box, and context.h says what each of
// these files offers the others.
//
// ---- FOCUS IS NOT HELD, AND EDITING WAITS FOR THE PRESS ----
//
// A FIELD TAKES THE POINTER SO THAT A PRESS CAN FIND IT, but what a press
// does to it is not the held/hovered/fired machinery of button.c: `focus` is its
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
// longer a field to be focused, checked once in voe_ui_field_forget regardless
// of whether the frame laid out, because `claim` marks a key whether its node
// was refused or not.
//
// EDITING RUNS IN voe_ui_field_edit, AFTER THE PRESS HAS BEEN RESOLVED AND
// BEFORE ANYTHING IS EMITTED, AND NOT INSIDE THAT RESOLUTION ITSELF, because
// the press is where this frame can MOVE focus to a field it had not settled
// on yet — the frame a person first clicks into one. Only once focus for this
// frame is final is there a field to edit.
//
// THE CONTEXT HOLDS THE FOCUSED FIELD'S TEXT IN ONE BUFFER AND NOT A TABLE,
// because only one field can be focused (ADR-0192). The edit seeds it from
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
#include "context.h"

#include <base/assert.h>
#include <math/float4.h>

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// A field's caret, in millimetres wide and as tall as its label's own
// rectangle.
#define FIELD_CARET_WIDE 0.3f

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
void voe_ui_number_seed(voe_ui_context *ui, uint64_t key, double value)
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

voe_ui_node voe_ui_field(voe_ui_context *ui, const char *name, uint32_t index,
			 const char *text, voe_ui_sizing sizing)
{
	voe_ui_node node;
	uint64_t key;
	const voe_ui_theme *theme = voe_ui_theme_current(ui);

	VOE_BASE_ASSERT(ui != NULL, "opening a field on no context");
	VOE_BASE_ASSERT(name != NULL, "a field with no name has no identity");
	VOE_BASE_ASSERT(text != NULL, "a field with no text");
	VOE_BASE_ASSERT(theme != NULL,
			"a field needs a theme: see "
			"voe_ui_theme_set/voe_ui_theme_push");

	key = voe_ui_widget_claim(ui, name, index);

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
	// AFTER THE FIELD'S OWN, which is what lets voe_ui_field_edit and
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

// A focus whose field was not called this frame is dropped, exactly as an area
// not called forgets its offset. It is asked whether or not the frame laid out,
// because `claim` marks every key a call took regardless of refusal, so the
// check is meaningful either way.
void voe_ui_field_forget(voe_ui_context *ui)
{
	if (ui->focus_set && !voe_ui_key_taken(ui, ui->focus))
		ui->focus_set = false;
	if (ui->field_holding && !voe_ui_key_taken(ui, ui->field_owner))
		ui->field_holding = false;
}

// Where a press takes the focus. IT IS THE PRESS ITSELF AND NOT A RELEASE,
// unlike `held`/`fired`: a press inside a field focuses it and a press ANYWHERE
// ELSE — nothing, a button, a bar — clears it, on the same edge. See widgets.h.
// A press inside the number box open for typing is not elsewhere: it keeps the
// focus, and that is the true handed back, which is button.c's answer that such
// a press arms no drag — the box is a field until it closes.
bool voe_ui_field_press(voe_ui_context *ui, const struct voe_ui_hit *hit,
			uint64_t hovered)
{
	bool on_focus = hit->kind == VOE_UI_HIT_WIDGET && ui->focus_set &&
			ui->focus == hovered;

	if (hit->kind == VOE_UI_HIT_WIDGET && (hit->field || on_focus)) {
		ui->focus = hovered;
		ui->focus_set = true;
	} else {
		ui->focus_set = false;
	}
	return on_focus && hit->number;
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
// focus is settled — which is why this runs after the press is resolved rather
// than inside it: a press this same frame can move focus to a field before its
// first keystroke, and the field found here is the one that press produced.
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
void voe_ui_field_edit(voe_ui_context *ui)
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
		voe_ui_number_seed(ui, ui->focus, ui->widgets[field].value);
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

	voe_ui_push_solid(ui, node,
			  (voe_ui_rect){ { label.min.x + label.size.x,
					   label.min.y },
					 { FIELD_CARET_WIDE, label.size.y } },
			  ui->widgets[node].theme->text_primary);
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

// THE WHOLE TEXT SELECTED IS THE ACCENT BEHIND IT: `label`'s own rectangle,
// pushed before that label's glyphs are, and only while the context holds this
// widget's text with all of it selected.
static void push_selection(voe_ui_context *ui, uint32_t node, uint32_t label)
{
	if (ui->field_holding && ui->field_selected &&
	    ui->field_owner == ui->widgets[node].key)
		voe_ui_push_rect(ui, label, ui->widgets[node].theme->accent);
}

// A field: its fill in the state's colour, and the selection over its composed
// label, which is always the node right after it — see voe_ui_field.
void voe_ui_field_emit(voe_ui_context *ui, uint32_t node)
{
	voe_ui_push_rect(ui, node, field_colour(ui, node));
	push_selection(ui, node, node + 1);
}

// A number box open for typing, which draws as a field does: the same colours
// and the same selection, behind the label the box composed of the buffer.
void voe_ui_number_open_emit(voe_ui_context *ui, uint32_t node)
{
	voe_ui_push_rect(ui, node, field_colour(ui, node));
	push_selection(ui, node, ui->number_open_label);
}

// The caret of the focused field or open number box, asked for after every
// label's own glyphs so that it paints in front of them.
void voe_ui_caret_emit(voe_ui_context *ui, uint32_t label)
{
	uint32_t owner = label_owner(ui, label);

	if (owner != VOE_UI_NODE_NONE && ui->focus_set &&
	    ui->focus == ui->widgets[owner].key)
		push_caret(ui, owner, label);
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
		// The composed label's own text: the edit pointed it at the
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
