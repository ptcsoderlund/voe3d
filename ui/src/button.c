// The button, the choice and the number box, and what the pointer comes to on
// any widget:
// the hit test, the press that arms one, the release that fires it, and the
// drag between them. See include/ui/widgets.h for the promises; widgets.c makes
// the keys, the frame and the records these are built out of, and context.h
// says what each of these files offers the others.
//
// THE PRESS IS HERE BECAUSE THE BUTTON IS WHAT IT IS FOR, and there is one of
// it: a field and a scrollbar are pressed by the same edge, so this asks
// field.c for what a press does to the focus and scroll.c for what it does to a
// thumb rather than either of them deciding that edge a second time.
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
// crossed out of, gives it the focus a field has — see field.c.
#include "context.h"

#include <base/assert.h>
#include <math/float4.h>

#include <math.h>

// Between an open number box's typed text and its "not a number", in
// millimetres.
#define NUMBER_REFUSED_GAP 1.5f

voe_ui_node voe_ui_button_begin(voe_ui_context *ui, const char *name,
				uint32_t index)
{
	voe_ui_node node;
	uint64_t key;
	const voe_ui_theme *theme = voe_ui_theme_current(ui);

	VOE_BASE_ASSERT(ui != NULL, "opening a button on no context");
	VOE_BASE_ASSERT(name != NULL, "a button with no name has no identity");
	VOE_BASE_ASSERT(theme != NULL,
			"a button needs a theme: see "
			"voe_ui_theme_set/voe_ui_theme_push");

	key = voe_ui_widget_claim(ui, name, index);

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

// A CHOICE IS A BUTTON WITH A FLAG, and this is the whole of it: the key, the
// press, the release and the answer are the button's, and `selected` only
// changes what is drawn — see voe_ui_control_inverted.
voe_ui_node voe_ui_choice_begin(voe_ui_context *ui, const char *name,
				uint32_t index, bool selected)
{
	voe_ui_node node = voe_ui_button_begin(ui, name, index);

	if (node != VOE_UI_NODE_NONE)
		ui->widgets[node].selected = selected;

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
	const voe_ui_theme *theme = voe_ui_theme_current(ui);

	VOE_BASE_ASSERT(ui != NULL, "opening a number box on no context");
	VOE_BASE_ASSERT(name != NULL,
			"a number box with no name has no identity");
	VOE_BASE_ASSERT(theme != NULL,
			"a number box needs a theme: see "
			"voe_ui_theme_set/voe_ui_theme_push");

	key = voe_ui_widget_claim(ui, name, index);

	// Open when it has the focus and the buffer is not another widget's.
	// Seeded here when the focus came by Tab, which has not seeded it yet,
	// so that it draws open on the very frame after the Tab.
	open = ui->focus_set && ui->focus == key &&
	       (!ui->field_holding || ui->field_owner == key);
	if (open && !ui->field_holding)
		voe_ui_number_seed(ui, key, value);

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

// Whether `node` is the caller's own content of the number box open for
// typing, which is neither drawn nor hit while it is open. It is the tail of
// that box's subtree, after what the box composed itself — see
// voe_ui_number_begin.
bool voe_ui_number_hidden(const voe_ui_context *ui, uint32_t node)
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
		    !voe_ui_number_hidden(ui, node) &&
		    voe_ui_inside(ui->nodes[node].visible, ui->pointer.at))
			hit = (struct voe_ui_hit){
				.kind = VOE_UI_HIT_WIDGET,
				.key = ui->widgets[node].key,
				.number = ui->widgets[node].kind ==
					  VOE_UI_WIDGET_NUMBER,
				.field = ui->widgets[node].kind ==
					 VOE_UI_WIDGET_FIELD,
			};

		voe_ui_scroll_hit(ui, at, &hit);
	}
	return hit;
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
static void number_drag(voe_ui_context *ui)
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

// What the pointer came to this frame, for every widget and every bar: what is
// hovered, what a press arms, what a release fires, and what a drag moved. Run
// from widgets.c's frame_end, after arrange and before anything is emitted,
// which is the only order in which a hit test is against this frame's
// rectangles — see the header above and context.h.
void voe_ui_pointer_resolve(voe_ui_context *ui, bool laid_out)
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
	// an area not called forgets its offset — see field.c, and note that it
	// runs whether or not the frame laid out.
	voe_ui_field_forget(ui);

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
	if (ui->held_set && !voe_ui_key_taken(ui, ui->held)) {
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
		// A press on a thumb or a track is the same edge and a bar's
		// own answer to it — see scroll.c.
		voe_ui_scroll_press(ui, &hit);
		// Where this gesture began. Written whatever was armed, so
		// that a press on a button leaves nothing behind for the next
		// number box to inherit.
		ui->number_press_x = ui->pointer.at.x;
		ui->number_last_x = ui->pointer.at.x;
		ui->number_crossed = false;

		// FOCUS FOLLOWS THE PRESS ITSELF AND NOT A RELEASE, unlike
		// `held`/`fired`, and where it goes is field.c's — see
		// voe_ui_field_press. What comes back is a press inside the
		// number box already open for typing, which arms no drag: the
		// box is a field until it closes.
		if (voe_ui_field_press(ui, &hit, hovered)) {
			ui->held_set = false;
			ui->held_number = false;
		}
	} else if (!ui->pointer.down && ui->was_down) {
		// A NUMBER BOX NEVER FIRES, which is why `held_number` is asked
		// here. The release that would have fired a button is the click
		// that opens one for typing instead: over the box, and never
		// out of the dead zone — not by a drag, and not by where the
		// release itself happened. See widgets.h. It is the focus
		// itself and not a kind of press, so field.c takes it from
		// here as it would a press into a field.
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
		number_drag(ui);

	ui->hovered = hovered;
	ui->hovered_set = hovered_set;
	ui->was_down = ui->pointer.down;
}

// Whether this control is drawn inverted: a button held this frame, a number
// box being dragged, or a choice its caller made selected. State is marked by
// inversion and by nothing set apart in colour (ADR-0194, ADR-0196), so this
// one question answers for the fill, for the border and for the ink of every
// label inside — which is why widgets.c asks it too, rather than each of them
// spelling the three cases out again.
//
// HELD BEATS HOVERED, because a button being pressed is what a person is doing
// and hovering is only where the pointer happens to be. A button held with the
// pointer dragged off it stays inverted, which is what says the press is still
// live and can still be completed by coming back.
//
// A NUMBER BOX IS INVERTED FOR THE WHOLE OF ITS DRAG, dead zone included: the
// gesture began at the press and the person has not let go, and a box that
// went back to rest a millimetre in would flicker under a slow hand. Open for
// typing it is held by nothing and draws as a field does — see field.c.
bool voe_ui_control_inverted(const voe_ui_context *ui, uint32_t node)
{
	const struct voe_ui_widget_record *w = &ui->widgets[node];

	if (w->kind != VOE_UI_WIDGET_BUTTON && w->kind != VOE_UI_WIDGET_NUMBER)
		return false;

	return w->selected || (ui->held_set && ui->held == w->key);
}

// The three states, for a button and for a number box alike — control at
// rest, control_hovered under the pointer, and `inverse` where the control is
// inverted. They look the same on purpose — see BUTTON_PAD.
static voe_math_float4 state_colour(const voe_ui_context *ui, uint32_t node)
{
	const struct voe_ui_widget_record *w = &ui->widgets[node];

	if (voe_ui_control_inverted(ui, node))
		return w->theme->inverse;
	if (ui->hovered_set && ui->hovered == w->key)
		return w->theme->control_hovered;
	return w->theme->control;
}

// A button is a fill in the state's colour inside the hairline border ADR-0171
// asks for, two element records where a plain fill is one — and INVERTED IT IS
// ONE BLOCK OF `inverse`, its border included, because a rim in the border
// role around an inverted fill draws a line where the eye should see the
// control turn over.
void voe_ui_button_emit(voe_ui_context *ui, uint32_t node)
{
	voe_math_float4 fill = state_colour(ui, node);
	voe_math_float4 border = voe_ui_control_inverted(ui, node)
					 ? fill
					 : ui->widgets[node].theme->border;

	voe_ui_push_bordered(ui, node, fill, border);
}

// A number box is one fill, borderless — and OPEN FOR TYPING IT DRAWS AS A
// FIELD DOES, which is field.c's record and not this one.
void voe_ui_number_emit(voe_ui_context *ui, uint32_t node)
{
	if (ui->widgets[node].open) {
		voe_ui_number_open_emit(ui, node);
		return;
	}
	voe_ui_push_rect(ui, node, state_colour(ui, node));
}

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
