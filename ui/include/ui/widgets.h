// Widgets: a panel, a label, a button, a number box you drag, an image and a
// scroll area, laid out by layout.h and handed back as element records for
// somebody else to draw.
//
//     voe_ui_font_set(ui, font);                   // once, at startup
//
//     voe_ui_frame_begin(ui, frame_arena);
//     voe_ui_pointer_set(ui, (voe_ui_pointer){
//             .at = { mouse_mm_x, mouse_mm_y }, .over = true, .down = held });
//
//     voe_ui_panel_begin(ui, "settings", 0, (voe_math_float4){ 0, 0, 0, 0.7f },
//                        (voe_ui_container){ .gap = 2.0f,
//                                            .pad = { 4, 4, 4, 4 } });
//     voe_ui_label(ui, "Settings");
//     voe_ui_node apply = voe_ui_button_begin(ui, "apply", 0);
//     voe_ui_label(ui, "Apply");
//     voe_ui_end(ui);
//     voe_ui_end(ui);
//
//     if (!voe_ui_frame_end(ui))
//             ...                    // the frame wanted more than it was given
//
//     if (voe_ui_button_action(ui, apply).fired)
//             apply_the_settings();
//
//     for (uint32_t i = 0; i < voe_ui_element_count(ui); i++)
//             voe_render_frame_submit_element(gpu, voe_ui_element(ui, i));
//
// IT PRODUCES RECORDS AND IT DOES NOT DRAW. Nothing here opens a frame, issues a
// draw or knows what a graphics card is doing; the records come out in paint
// order and the caller submits them. Which is the same trade layout.h makes with
// rectangles and it is what keeps this folder a leaf: where the interface hangs
// is one matrix and it is not this folder's.
//
// AND IT IS GIVEN THE POINTER RATHER THAN ASKING FOR ONE (ADR-0093). A point in
// the surface's own millimetres and a button that is either down or not, as
// plain values, every frame. This folder does not name `platform`, cannot ask a
// window anything, and its tests run with no window system at all — the pointer
// being a value is what makes every case below testable, drag included.
//
// ---- WHY THE ANSWER ARRIVES AFTER voe_ui_frame_end AND NOT AT THE CALL ----
//
// A widget call hands back a voe_ui_node and says nothing about what the pointer
// is doing, because at the moment of the call NOTHING HAS A RECTANGLE YET — see
// layout.h on why nothing is laid out until the frame ends. So the hit test
// happens inside voe_ui_frame_end, against the rectangles arrange has just
// worked out, and voe_ui_button_action reads the result out afterwards.
//
// The alternative is what most immediate-mode interfaces do: answer at the call
// site from last frame's geometry. That is a click tested against where the
// button WAS, which is wrong exactly when it matters — the frame a panel opened,
// a row reflowed, or a label grew. This way a click is always tested against the
// arrangement the person was looking at.
//
// ---- IDENTITY ----
//
// A WIDGET IS NAMED BY ITS PATH AND NOT BY ITS PLACE. Every panel and every
// button takes a `name` and an `index`, and its key is the enclosing panel's key
// mixed with both. A named widget passes its name and nought; a widget in a loop
// passes one name and the loop counter. The key is stable while the shape of the
// tree is, which is all immediate mode needs: the same calls in the same order
// name the same widgets next frame, and a button that moves because a row above
// it grew is still the same button.
//
// IT IS NOT __LINE__, AND THAT IS THE WHOLE REASON `index` EXISTS. A line number
// is identical for every iteration of a loop, so every button in a list would be
// one button — which reads as the list sharing a highlight, not as a bug in
// naming.
//
// TWO WIDGETS WITH THE SAME KEY REFUSE THE FRAME. They do not quietly share
// state, which is the failure this scheme exists to prevent and the one that is
// almost impossible to see: the second button lights up when the first is
// hovered and nothing anywhere says why. So the duplicate is caught at the call
// that made it, named on stderr with its name and index, and voe_ui_frame_end
// comes back false — the same refusal channel a frame that wanted too many nodes
// uses, for the same reason: it is the caller's mistake, it is reported once,
// and the next frame lays out normally.
//
// ---- THE TEXT SCALE ----
//
// A LABEL'S NATURAL SIZE IS WHAT THE FONT MEASURES, TIMES `text_scale`, AND THE
// MULTIPLICATION HAPPENS WHERE THE STRING IS MEASURED. So the number layout sees
// is already scaled and everything downstream is ordinary layout: a label at 1.5
// takes half again as much room and the row grows around it, while gaps and
// padding — which are millimetres the caller wrote — do not move. Nothing clips,
// because nothing was laid out against the unscaled size.
//
// IT IS NOT A SECOND SCALE AND THERE IS NO SECOND SPACE. The surface has one
// scale, and it is the caller's (ADR-0104): pixels per millimetre, applied to
// the whole surface, outside this folder entirely. `text_scale` is a multiplier
// INSIDE those millimetres, on one kind of content. The two compose by
// multiplication and in that order — the surface scale decides how big a
// millimetre is, `text_scale` decides how many millimetres a letter is worth —
// and a subtree scale, when ADR-0091's push and pop arrives, composes the same
// way and for the same reason: they are all multipliers on one unit, which is
// why there is never a conversion between them to get wrong.
//
// ---- THE SCROLL AREA ----
//
// THE OFFSET IS REMEMBERED HERE AND NOT IN LAYOUT, because remembering needs an
// identity and layout has none (layout.h, OVERFLOW). A scroll area is keyed like
// a panel; its offset lives in a table in the context under that key,
// `capacities.scrolls` long, and is handed to layout as the container's `scroll`
// every frame. What is stored after layout is the offset layout USED, clamped, so
// a remembered offset is always one the content allowed.
//
// AN AREA THAT IS NOT CALLED IN A FRAME IS FORGOTTEN AT THAT FRAME'S END, and
// comes back at nought. That is transient state on purpose (ADR-0153): an
// inspector switched to another entity and back starts at the top, and nothing
// here is saved anywhere.
//
// A SCROLL ARRIVES AS A LENGTH IN MILLIMETRES, NEVER AS WHEEL NOTCHES. How long a
// notch is belongs to the program; a thumbstick or a hand produces a length just
// as well, so nothing in this folder is shaped like a mouse. The pointer's
// `scroll` starts at the innermost scroll area under the pointer. THE AREA TAKES
// WHAT ITS CLAMP ALLOWS, ON EACH AXIS IT SCROLLS, AND PASSES THE REST OUTWARD to
// the next scroll area around it, and so on; what nobody can take is dropped. An
// area at the end of a nested list therefore hands the rest of a gesture to the
// panel it sits in, and an axis an area does not scroll passes through it whole.
//
// A SCROLL LANDS IN THE NEXT FRAME'S LAYOUT, not this one's. It is worked out
// inside voe_ui_frame_end, after the hit test and against this frame's
// rectangles, because those are the rectangles the person saw: moving content
// mid-frame would hit test a wheel, a thumb or a click against an arrangement
// nobody was looking at. The price is one frame between the gesture and the
// movement.
//
// THE SCROLLBAR IS DRAWN OVER THE CONTENT AND TAKES NO LAYOUT SPACE, so a bar
// appearing never re-wraps what is under it. It sits inside the area's rectangle
// along the far edge — right for Y, bottom for X — only on an axis that scrolls
// and has something to scroll, after the area's children in paint order and in
// front of them for the pointer. Content that must stay clear of it is padded:
// give the area a right or bottom padding of the bar's thickness.
//
// ITS THUMB IS DRAGGED AND ITS TRACK IS PAGED. Pressing the thumb holds it and the
// offset follows the pointer along the track — a gesture that carries on off the
// bar and off the surface, as a number box's does. A press on the track outside
// the thumb moves one arranged length towards the pointer, once per press, and
// holds nothing.
//
// ---- WHAT IS NOT HERE ----
//
// No theme: the colours below are constants and card 036 replaces them. No text
// input, no caret and no selection — a number box is dragged and not typed into,
// and the click that would begin typing is reserved rather than free. No
// scrolling by a program and no scrolling to a node yet: both arrive with focus,
// which is their first caller. No dragging the content itself, no focus, no smooth
// scrolling, and no offset saved beyond the context. No checkbox and no slider: a
// number box has no track, no ends and no range, which is what makes it the one
// that fits a field of unknown extent.
//
// WHAT IS HERE INSTEAD OF A CLIP OF ITS OWN: every record is clipped to what its
// node's clipping ancestors leave — a panel, a button or an image to its
// voe_ui_node_visible — and a record with nothing left is not emitted and takes
// no capacity. A widget clipped out of sight is not hit: the pointer is tested
// against its visible rectangle, so it cannot be hovered, armed or pressed there.
// A gesture already under way carries on when its widget is clipped or scrolled
// away, exactly as it carries on past the surface's edge.
#pragma once

#include <ui/layout.h>

#include <math/float2.h>
#include <math/float4.h>
#include <render/device.h>
#include <text/font.h>

#include <stdint.h>

// The font every label is measured from and every letter is drawn out of. Set
// once, before the first frame that has a label in it; a label with no font is
// the caller's bug and asserts.
//
// The font is not this folder's to make or to destroy — it belongs to whoever
// created it, and it must outlive the context.
void voe_ui_font_set(voe_ui_context *ui, const voe_text_font *font);

// What every label's measured size is multiplied by. One, until somebody says
// otherwise, and greater than nought — nought is the caller's bug and asserts.
// See the header on how it composes with the surface's own scale.
void voe_ui_text_scale_set(voe_ui_context *ui, float scale);

// What the interface is told about the pointer. Given, never asked for.
typedef struct {
	// Where it is, in the surface's own millimetres: X right, Y down,
	// origin at the surface's top-left corner. That is the space layout
	// works in and the space an element record is in (ADR-0099), so a hit
	// test is a rectangle comparison and there is no arithmetic anywhere
	// between a window's pixels and this beyond the caller's one division.
	voe_math_float2 at;
	// Whether there is a pointer at all. False ends a hover — a pointer
	// that has left the window, or one the caller has locked for looking
	// around, is not pointing at anything, so nothing is hovered and
	// nothing can be armed.
	//
	// A GESTURE ALREADY UNDER WAY IS NOT ENDED BY IT, and `at` still counts
	// for that gesture. A press survives, and a number box being dragged
	// goes on reading the pointer — which is what lets a drag carry on past
	// the surface's edge, where `platform` keeps reporting the pointer
	// because a button is down. `over` governs hovering and arming; `down`
	// governs the gesture.
	bool over;
	// Whether the primary button is held NOW. Level and not an edge: this
	// folder works out the press and the release from one frame to the
	// next, because it is the only thing that knows which widget was under
	// the pointer when the button went down.
	bool down;
	// Whether the caller's fine modifier is held, which slows a drag to
	// VOE_UI_NUMBER_FINE of its usual rate. WHICH KEY THAT IS IS NOT THIS
	// FOLDER'S TO KNOW: it arrives as a value, exactly as the pointer does
	// and for the same reason (ADR-0093), so a fine drag is testable with no
	// window system and the choice of key stays where the keyboard is.
	//
	// Nought is NOT fine, which is what lets every designated initialiser
	// that predates this field go on meaning what it meant.
	bool fine;
	// Millimetres scrolled this frame, positive showing content further
	// right or further down. The caller turns its wheel's notches into a
	// length; this folder never sees a notch. Taken by the innermost scroll
	// area under `at`, and only while `over` — see THE SCROLL AREA at the top
	// of this header. Nought is no scroll.
	voe_math_float2 scroll;
} voe_ui_pointer;

// Hands this frame's pointer over. Between voe_ui_frame_begin and
// voe_ui_frame_end, once.
//
// A FRAME THAT NEVER CALLS THIS HAS NO POINTER, which is what a zeroed
// voe_ui_pointer means and is the right answer rather than a convenience: a
// frame built with no input said nothing was pointing at anything. Nothing is
// hovered, a held button is let go, and a test that only cares about layout need
// not mention input at all.
void voe_ui_pointer_set(voe_ui_context *ui, voe_ui_pointer pointer);

// A container with a background behind its children. Everything else about it —
// its size, how its children sit in it, its gap and its padding — is
// voe_ui_column_begin's `container`, unchanged, because a panel IS a column with
// something drawn behind it.
//
// `colour` is straight linear RGBA and IS NOT PREMULTIPLIED. The shader
// multiplies by alpha once, at output (ADR-0069); doing it here as well gives a
// panel that is too faint, which reads as a badly chosen colour rather than as a
// bug.
//
// AN ALPHA OF NOUGHT EMITS NOTHING AT ALL. Not a transparent rectangle — no
// record, no instance and no blend, because all three of those cost something to
// draw nothing. A fully transparent panel with padding in it is a real thing to
// want: a full-screen one is how a television safe area is expressed, where an
// older set cuts the edges off and the interface has to stay inside them.
//
// AND SUCH A CALLER MAY NOT WANT A PANEL AT ALL. voe_ui_column_begin with the
// same padding emits nothing already and is the same thing with fewer words.
// What this widget is for is the background; if there is no background there is
// nothing here that a column does not do.
//
// Closed by voe_ui_end, like any other container.
voe_ui_node voe_ui_panel_begin(voe_ui_context *ui, const char *name,
			       uint32_t index, voe_math_float4 colour,
			       voe_ui_container container);

// A string. Its natural size is the font's measurement of it times the text
// scale, so a row containing one grows to fit it; it takes no sizing of its own
// and has no identity, having nothing to remember.
//
// `text` is read at voe_ui_frame_end and not copied, so it must still be there
// then. A literal is; a buffer the caller rewinds with the frame's arena is not.
voe_ui_node voe_ui_label(voe_ui_context *ui, const char *text);

// A rectangle with whatever is called between here and voe_ui_end centred in
// it, in one of three states: normal, hovered, and held. What comes back is a
// node whose rectangle is the whole button, and its size is what is inside it
// plus the padding a button has.
//
//     voe_ui_node ok = voe_ui_button_begin(ui, "ok", 0);
//     voe_ui_label(ui, "OK");
//     voe_ui_end(ui);
//
// THE LABEL IS COMPOSED IN RATHER THAN PASSED, and the three lines are the point
// rather than a cost. A button is a container like every other container here:
// put a label in it and it is a labelled button, put a box in it and it is a
// swatch, put a label beside an icon in it and it is that, and none of those is
// a second widget or an argument nobody uses. It also means a button costs
// nothing that a label costs — a caller with no font can still build, arrange
// and click one, which is what lets every case below be tested with no graphics
// card anywhere near it.
//
// IT FIRES ON RELEASE INSIDE ITSELF AND A DRAG OUT CANCELS, which is what
// Windows 10 does and ADR-0088 makes that binding. Press it, drag off it and let
// go: nothing happens. Press it, drag off it, drag BACK on and let go: it fires,
// because cancelling is what leaving does and coming back undoes it.
//
// Closed by voe_ui_end, like any other container.
voe_ui_node voe_ui_button_begin(voe_ui_context *ui, const char *name,
				uint32_t index);

// What the pointer did to one button. Read after voe_ui_frame_end, through the
// node the button call handed back; reading it before, or through a node that is
// not a button, is the caller's bug and asserts.
typedef struct {
	// The pointer is over it and nothing in front of it took the pointer
	// first. False while the pointer is elsewhere, even mid-drag.
	bool hovered;
	// It is the button the pointer went down on and has not yet let go of.
	// True while a drag is off it, which is how "come back and it still
	// fires" works.
	bool held;
	// It fired this frame. True for exactly one frame, on the frame the
	// release happened.
	bool fired;
} voe_ui_action;

voe_ui_action voe_ui_button_action(const voe_ui_context *ui,
				   voe_ui_node button);

// ---------------------------------------------------------- the number box

// How far the pointer moves before a drag begins, in millimetres, and what the
// fine modifier multiplies the change by once it has.
//
// THE DEAD ZONE IS WHAT KEEPS A CLICK A CLICK. Without it every press nudges the
// value by whatever the hand did between two frames, so a number could not be
// clicked at all without changing it — and clicking one is reserved for typing
// into it. The drag starts from the far edge of the dead zone and not from the
// press, so the value does not jump by a millimetre the moment it begins.
#define VOE_UI_NUMBER_DEAD_ZONE 1.0f
#define VOE_UI_NUMBER_FINE 0.1

// A rectangle whose value changes when you drag across it, with whatever is
// called between here and voe_ui_end centred in it. It looks like a button and
// is built like one.
//
//     voe_ui_node x = voe_ui_number_begin(ui, "x", 0, position.x, 0.5);
//     voe_ui_label(ui, "0.50");
//     voe_ui_end(ui);
//
//     voe_ui_number_result r = voe_ui_number_action(ui, x);
//     if (r.changed)
//             position.x = (float)r.value;
//
// THIS FOLDER KNOWS NO FIELD KINDS AND IT IS NOT GOING TO. The caller hands in
// the value and what one millimetre of horizontal drag is worth, and takes the
// new value back; a whole number is the caller rounding what comes back, and a
// limit is the caller clamping it. A number box that knew about integers,
// ranges or units would be this folder holding opinions about data it cannot
// see — which is the editor's business and, for a component's fields, `base`'s
// description.
//
// THE LABEL IS COMPOSED IN, AS WITH A BUTTON. The widget does not format the
// number: how many decimal places a value deserves is not something `ui` can
// know, and a caller that wants a name beside the figure puts both in.
//
// WHAT COMES BACK IS A VALUE AND NOT A DISTANCE, and that is a contract rather
// than a convenience. The caller writes back what it is given and never
// accumulates a delta of its own — which is what makes typing into one, when it
// arrives, the same call answering the same way: a typed entry has no distance
// to report and every caller is already written to take a value.
//
// A PRESS AND A RELEASE WITHOUT MOVEMENT DOES NOTHING, AND NOTHING MAY BE BOUND
// TO IT. It is not an event this widget has declined to expose — it is reserved,
// for the typing that card 057 does not build. A caller that gave a click a
// meaning of its own would have to take it away again when a caret appears here.
// So a number box NEVER FIRES: there is no `fired` below and
// voe_ui_button_action refuses one.
//
// Closed by voe_ui_end, like any other container.
voe_ui_node voe_ui_number_begin(voe_ui_context *ui, const char *name,
				uint32_t index, double value,
				double per_millimetre);

// What the pointer did to one number box. Read after voe_ui_frame_end, through
// the node the number call handed back; reading it before, or through a node
// that is not a number box, is the caller's bug and asserts.
typedef struct {
	// The pointer is over it and nothing in front of it took the pointer
	// first. False while the pointer is elsewhere, even mid-drag.
	bool hovered;
	// It is the number box the pointer went down on and has not yet let go
	// of. True for the whole drag, wherever the pointer has got to.
	bool held;
	// This frame's drag moved the value. False on the frames inside the
	// dead zone, and false on a frame the pointer did not move.
	bool changed;
	// The value handed in, plus this frame's drag. Equal to what was handed
	// in whenever `changed` is false, so a caller may write it back every
	// frame or only when it changed and get the same answer.
	double value;
} voe_ui_number_result;

voe_ui_number_result voe_ui_number_action(const voe_ui_context *ui,
					  voe_ui_node number);

// --------------------------------------------------------- the scroll area

// Which axes a scroll area scrolls. Absolute, as overflow is.
typedef struct {
	bool x;
	bool y;
} voe_ui_scroll_axes;

// A column that clips on both axes and scrolls on the axes named, with its
// offset remembered under its key and a scrollbar drawn over its content.
//
//     voe_ui_scroll_begin(ui, "inspector", 0,
//                         (voe_ui_container){
//                                 .size = { { VOE_UI_SIZE_GROW, 1 },
//                                           { VOE_UI_SIZE_FIXED, 60 } },
//                                 .gap = 2, .pad = { 2, 2, 3.5f, 3.5f } },
//                         (voe_ui_scroll_axes){ .y = true });
//     ...                                   // content, laid out as in a column
//     voe_ui_end(ui);
//
// `container` IS A COLUMN'S, EXCEPT FOR ITS `overflow` AND ITS `scroll`: those
// are the area's own, and a caller who set either asserts. An axis not in `axes`
// is still clipped, and its offset is always nought.
//
// The key is claimed as a panel's is, so the name and index follow the same rules
// and a duplicate refuses the frame. A frame with more scroll areas than
// `capacities.scrolls` is refused too, named on stderr, and voe_ui_frame_end
// comes back false.
//
// Closed by voe_ui_end, like any other container.
voe_ui_node voe_ui_scroll_begin(voe_ui_context *ui, const char *name,
				uint32_t index, voe_ui_container container,
				voe_ui_scroll_axes axes);

// --------------------------------------------------------------- the image

// A picture as a node: part of a texture stretched over the node's rectangle. It
// is for a view drawn into a target of one's own, an icon, a thumbnail —
// anything that is already a colour texture on the device (ADR-0148).
//
//     // `picture` is the texture voe_render_target_create handed back
//     voe_ui_node view = voe_ui_image(ui, picture,
//                                     (voe_math_float4){ 0, 0, 1, 1 },
//                                     (voe_math_float2){ 0, 0 },
//                                     (voe_ui_sizing){
//                                             .along = { VOE_UI_SIZE_GROW, 1 },
//                                             .across = { VOE_UI_SIZE_FIXED, 60 } });
//
// IT IS SIZED EXACTLY AS A BOX IS, because it is one: `content` and `sizing` are
// voe_ui_box's, unchanged. This folder does not know how big a picture is — it
// holds an id and not the pixels — so there is no natural size to measure; the
// caller writes one into `content`, or sizes the node FIXED or GROW. NO SCALING
// MODE, NO ASPECT FITTING AND NO TINT: a picture fills its rectangle whatever
// shape that is, and keeping the aspect is the caller choosing the rectangle.
//
// `sheet` is which part of the texture is shown, in texture coordinates, `xy` its
// top-left corner and `zw` its size — the element record's own shape. The whole
// picture is { 0, 0, 1, 1 }.
//
// IT IS ONE ELEMENT RECORD, against the frame's budget like a panel's background:
// kind IMAGE, clipped to its visible rectangle, coloured opaque white so the
// picture shows as it is. It paints where a leaf paints — after its parent, in
// call order among its siblings.
//
// THE TEXTURE IS THE CALLER'S AND SO IS ITS LIFETIME. Only its index half goes
// into the record, and nothing here checks that it still names a live texture:
// it must outlive the frame the record is submitted in. No texture is loaded,
// created or destroyed in this folder.
//
// IT IS A LEAF, WITH NO IDENTITY AND NO ANSWER TO THE POINTER. It takes no name,
// having nothing to remember, and it neither hovers nor clicks. Whether the
// pointer is over it is the caller comparing the pointer with voe_ui_node_rect,
// as it would be for a panel.
//
// VOE_UI_NODE_NONE when the frame has no room for another node, and a picture
// outside a container is the caller's bug and asserts, as a box is.
voe_ui_node voe_ui_image(voe_ui_context *ui, voe_render_texture texture,
			 voe_math_float4 sheet, voe_math_float2 content,
			 voe_ui_sizing sizing);

// This frame's element records, in paint order, built by voe_ui_frame_end.
// Readable until the next voe_ui_frame_begin or until the caller rewinds the
// frame's arena; reading before the frame has ended, or past the count, is the
// caller's bug and asserts.
//
// PAINT ORDER IS SUBMISSION ORDER AND IT IS THE ORDER LAYOUT ARRANGED (ADR-0092).
// A node's own records come before every one of its children's, so a panel's
// background is behind what is in it and a button's is behind its label;
// siblings paint in call order. There is no depth on this path and nothing sorts
// anything — submitting these out of order is a background drawn over its own
// text, and it will look like a colour mistake.
//
// A record is handed back by copy rather than by pointer, so that nothing here
// leaks a pointer into the arena the caller is about to rewind.
uint32_t voe_ui_element_count(const voe_ui_context *ui);
voe_render_element voe_ui_element(const voe_ui_context *ui, uint32_t index);
