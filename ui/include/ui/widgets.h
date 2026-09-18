// Widgets: a panel, a label, a button, a number box you drag, an image and a
// scroll area, laid out by layout.h and handed back as element records for
// somebody else to draw.
//
//     voe_ui_font_set(ui, font);                   // once, at startup
//     voe_ui_theme_set(ui, &theme);                // once, or again to restyle
//
//     voe_ui_frame_begin(ui, frame_arena);
//     voe_ui_pointer_set(ui, (voe_ui_pointer){
//             .at = { mouse_mm_x, mouse_mm_y }, .over = true, .down = held });
//
//     voe_ui_panel_begin(ui, "settings", 0, VOE_UI_SURFACE_RAISED,
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
// ---- THE THEME ----
//
// EVERY WIDGET DRAWS FROM THE NEAREST THEME IN FORCE (ADR-0170). voe_ui_theme_set
// gives the context the one it falls back to, the caller's memory and outliving
// the context exactly as the font does; voe_ui_theme_push/voe_ui_theme_pop put
// another in force for a subtree, nested to any depth. A widget reads whichever
// is in force AT THE CALL THAT MAKES IT, once, and keeps it — a push and a pop
// either side of a call already made does not repaint it, because the frame is
// built forwards and a widget's colours are already decided the moment it exists.
// A widget with no theme anywhere in force is the caller's bug and asserts,
// exactly as a label with no font does.
//
// A LABEL'S NATURAL SIZE IS WHAT THE FONT MEASURES, TIMES THE THEME'S OWN
// `text_size` OVER ONE EM, AND THE MULTIPLICATION HAPPENS WHERE THE STRING IS
// MEASURED. So the number layout sees is already the right size and everything
// downstream is ordinary layout: a bigger theme takes more room and the row
// grows around it, while gaps and padding — which are millimetres the caller
// wrote — do not move. `text_size` IS THE ONE PLACE A TEXT SIZE IS SAID
// (voe_ui_text_scale_set is gone for exactly this reason): a caller wanting a
// bigger interface authors a bigger theme rather than scaling on top of one.
//
// IT IS NOT A SECOND SCALE AND THERE IS NO SECOND SPACE. The surface has one
// scale, and it is the caller's (ADR-0104): pixels per millimetre, applied to
// the whole surface, outside this folder entirely. `text_size` is millimetres
// INSIDE those millimetres, on one kind of content, and the two compose by the
// caller's own division happening before either of them is in the picture —
// there is never a conversion between them to get wrong.
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
// comes back at nought. That is transient state on purpose (ADR-0153): an area
// that stops being called at all — a panel closed, a tab switched away — comes
// back at the top, and nothing here is saved anywhere. An area that IS called
// every frame keeps its offset through anything its content does, however much
// that content changes: what brings a shorter inspector back to the top of its
// content is the clamp, which layout runs against the content's own measure,
// and not this table forgetting.
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
// TYPING EXISTS FOR THE FIELD AND FOR NOTHING ELSE. A number box is still
// dragged and not typed into, and the click that would begin typing there is
// still reserved rather than free — see voe_ui_number_begin. THE FIELD ITSELF
// HAS NO SELECTION, NO CLIPBOARD, NO MOVING THE CARET AND NO MULTIPLE LINES:
// what it does is append at the end, delete from the end, and hand back what
// came of that. A caret that can be moved, a range that can be cut, and a
// second line are later work built on top of this one.
//
// THE FIELD IS GIVEN ITS TEXT AS A VALUE AND COMPOSES ITS OWN LABEL, RATHER
// THAN TAKING ONE IN AS A BUTTON DOES, because the caret is measured from
// that label's own rectangle: a label a caller composed in could be anything,
// and the field would be guessing where its box ended. WHAT COMES BACK IS THE
// EDITED TEXT AND NOT THE CALLER'S OWN BUFFER WRITTEN INTO, for the same
// reason a number box hands back a value and not a distance — a field cannot
// see how the caller's text is owned, only what was in it and what a frame
// typed, so the honest answer is a value the caller may store however it
// likes. And WHERE THE TYPED BYTES CAME FROM IS NOT THIS FOLDER'S BUSINESS: a
// keymap read in `platform`, an IME, anything else that turns a key into
// UTF-8 all produce the same voe_ui_keyboard, and this folder decodes nothing
// about the device behind it.
//
// A FIELD COSTS TWO NODES — itself and the label it composes — and up to one
// element record per letter that draws, plus its own background and, while
// it is focused, one caret: no more than a label put inside a button already
// costs. NO BORDER, LIKE THE NUMBER BOX: its background is a theme role and it
// needs the nearest theme in force, but two records are a panel's and a
// button's, not this one's.
//
// No scrolling by a program and no scrolling to a node yet: both wait on a
// focus that reaches a scroll area, which this one does not — a field's
// focus is its own. No dragging the content itself, no smooth scrolling, and
// no offset saved beyond the context. No checkbox and no slider: a number box
// has no track, no ends and no range, which is what makes it the one that
// fits a field of unknown extent.
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
#include <ui/theme.h>

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

// The theme every widget without a nearer one draws with. The caller's memory,
// outliving the context exactly as the font does (voe_ui_font_set) — a theme
// derived on the stack and handed here would leave this pointer dangling the
// moment the calling function returns.
//
// May be called again, which is what restyles every widget the very next
// frame; a widget already emitted this frame keeps the colours it was given,
// because it read them at the call that made it and not at emission. NULL is
// the caller's bug and asserts, as a NULL font does at voe_ui_font_set.
void voe_ui_theme_set(voe_ui_context *ui, const voe_ui_theme *theme);

// Puts `theme` in force for every widget made until the matching
// voe_ui_theme_pop, nested to any depth — ADR-0170's "the nearest one wins".
// `theme` is the caller's memory and must outlive every widget made while it
// is in force, exactly as voe_ui_theme_set's must.
//
// Between voe_ui_frame_begin and voe_ui_frame_end, like every other call that
// touches the context's state. NULL is the caller's bug and asserts.
//
// UNMATCHED BY THE FRAME'S END, IT REFUSES THE FRAME the way a duplicate key
// or a scroll area past capacity does (ui/layout.h) — reported once on
// stderr, the frame carrying on and the next one laying out normally — rather
// than asserting: a push forgotten inside a branch that returns early is a
// call-site mistake worth finding from a message, not a program that stops.
// Popping with nothing pushed is the other imbalance, and that one IS an
// assert — see voe_ui_theme_pop.
void voe_ui_theme_push(voe_ui_context *ui, const voe_ui_theme *theme);

// Restores the theme in force before the matching voe_ui_theme_push.
//
// Popping with nothing pushed is the caller's bug and asserts, exactly as
// ending a container that was never begun does (ui/layout.h).
void voe_ui_theme_pop(voe_ui_context *ui);

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

// A panel's background, as one of the theme in force's own surfaces, or NONE.
//
// NONE EMITS NOTHING AT ALL, exactly as an alpha of nought used to before a
// theme existed to name one. Not a transparent rectangle — no record, no
// instance and no blend, because all three of those cost something to draw
// nothing. A fully transparent panel with padding in it is a real thing to
// want: a full-screen one is how a television safe area is expressed, where an
// older set cuts the edges off and the interface has to stay inside them. And
// such a caller may not want a panel at all — voe_ui_column_begin with the
// same padding emits nothing already and is the same thing with fewer words.
typedef enum {
	VOE_UI_SURFACE_NONE = 0,
	VOE_UI_SURFACE_GROUND,
	VOE_UI_SURFACE_SURFACE,
	VOE_UI_SURFACE_RAISED,
} voe_ui_surface;

// A container with a background behind its children. Everything else about it —
// its size, how its children sit in it, its gap and its padding — is
// voe_ui_column_begin's `container`, unchanged, because a panel IS a column with
// something drawn behind it.
//
// A SURFACE OTHER THAN NONE DRAWS A HAIRLINE BORDER, TWO ELEMENT RECORDS AND
// NOT ONE (ADR-0171): the border role at the panel's own bounds, then its
// surface inset from every edge by this folder's hairline width, painted over
// the border's middle and leaving a rim of it showing all round. That is the
// widget's own doing and not the shader's, so nothing in `render` changes for
// it, and it is why a panel costs one more element record than it used to.
//
// NEEDS A THEME WHEN `surface` IS NOT NONE — the nearest one in force,
// voe_ui_theme_set's or a voe_ui_theme_push's — and opening one with nowhere
// to find one is the caller's bug and asserts, exactly as a label with no font
// does.
//
// Closed by voe_ui_end, like any other container.
voe_ui_node voe_ui_panel_begin(voe_ui_context *ui, const char *name,
			       uint32_t index, voe_ui_surface surface,
			       voe_ui_container container);

// Which of the theme's two text colours a label draws in.
typedef enum {
	VOE_UI_TEXT_ROLE_NORMAL = 0,
	VOE_UI_TEXT_ROLE_ACCENT,
} voe_ui_text_role;

// A string. Its natural size is the font's measurement of it times the theme in
// force's own `text_size`, so a row containing one grows to fit it; it takes no
// sizing of its own and has no identity, having nothing to remember.
//
// `text` is read at voe_ui_frame_end and not copied, so it must still be there
// then. A literal is; a buffer the caller rewinds with the frame's arena is not.
//
// NEEDS A FONT AND A THEME, the nearest of each in force: the font to measure
// and draw the string, the theme for `text_size` and for `role`'s colour.
// Either missing is the caller's bug and asserts.
voe_ui_node voe_ui_label_role(voe_ui_context *ui, const char *text,
			      voe_ui_text_role role);

// Exactly voe_ui_label_role with VOE_UI_TEXT_ROLE_NORMAL.
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
// a second widget or an argument nobody uses.
//
// IT FIRES ON RELEASE INSIDE ITSELF AND A DRAG OUT CANCELS, which is what
// Windows 10 does and ADR-0088 makes that binding. Press it, drag off it and let
// go: nothing happens. Press it, drag off it, drag BACK on and let go: it fires,
// because cancelling is what leaving does and coming back undoes it.
//
// ITS THREE STATES ARE THEME ROLES, the pressed one on the accent
// (ADR-0171): control at rest, control_hovered under the pointer, and accent
// while held — there is no fourth role for "pressed", the accent standing in
// for it. LIKE A PANEL IT DRAWS A HAIRLINE BORDER, two element records and not
// one, and needs the nearest theme in force exactly as a panel with a surface
// does; opening one with nowhere to find a theme is the caller's bug and
// asserts.
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
// ITS COLOURS ARE THE SAME THEME ROLES A BUTTON'S ARE — control, control_hovered
// and, while held, the accent — read from the nearest theme in force, and it
// needs one for the same reason a button does. UNLIKE A PANEL AND A BUTTON IT
// DRAWS NO BORDER: one element record, as before this task.
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

// ------------------------------------------------------------- the field

// How many bytes of text one field holds, the NUL this folder adds not
// counted. Chosen once for the whole engine rather than being a parameter of
// every field, because the one caller today — a typed folder name — needs
// nothing longer and a second number per field is one more thing every caller
// would have to decide.
#define VOE_UI_FIELD_CAPACITY 256

// What was typed since the previous frame, and the two keys a field answers
// to beyond ordinary letters. Given, never asked for, exactly as the pointer
// is (ADR-0093) — WHERE THE BYTES CAME FROM IS NOT THIS FOLDER'S BUSINESS. A
// keymap read in `platform`, an IME, or anything else that turns a key into
// UTF-8 all produce the same value, and this folder decodes nothing about the
// device behind it.
typedef struct {
	// UTF-8 typed since the previous frame, read for `size` bytes and not
	// assumed to carry a NUL: a frame that typed nothing may hand in
	// nothing at all, which is what `size` of nought means.
	const char *text;
	uint32_t size;
	// The two keys editing answers to, already decided by whoever handed
	// this over — this folder never compares `text` against a control
	// character to find them.
	bool backspace;
	bool enter;
} voe_ui_keyboard;

// Hands this frame's typing over. Between voe_ui_frame_begin and
// voe_ui_frame_end, once, exactly as voe_ui_pointer_set is.
//
// A FRAME THAT NEVER CALLS THIS HAS NO TYPING, which is what a zeroed
// voe_ui_keyboard means: nothing is appended, nothing is deleted, and no
// field reports `entered`. A caller that gives no field the keyboard this
// frame need not mention it at all.
void voe_ui_keyboard_set(voe_ui_context *ui, voe_ui_keyboard keyboard);

// A single line of editable text: a keyed container built as a button is —
// BUTTON_PAD round it and the three state colours a button has, plus a
// fourth for focused — except that its run sits along START rather than
// centred, so its text begins at the left edge and grows rightward.
//
//     voe_ui_node name = voe_ui_field(ui, "name", 0, folder_name,
//                                     (voe_ui_sizing){
//                                             .along = { VOE_UI_SIZE_GROW, 1 },
//                                             .across = { VOE_UI_SIZE_FIXED, 8 } });
//     ...
//     voe_ui_field_result r = voe_ui_field_action(ui, name);
//     if (r.changed)
//             folder_name = r.text;
//
// IT TAKES A voe_ui_sizing AND NOT A voe_ui_container, unlike a panel, a
// button or a scroll area: the padding, the run and the clipping are the
// field's own and not the caller's to set, so only how big it is is left
// open.
//
// THE CALL MAKES AND ENDS ITS OWN LABEL OF `text`, rather than composing one
// in as a button does. That is not this widget being less flexible for no
// reason: a field is always exactly one string, never an icon beside a word,
// and building the label here rather than being handed one back is what lets
// the caret be measured from that label's own rectangle at emission, instead
// of guessed at from the field's. So a field costs two nodes — itself and
// that one label — however it is called, and there is nothing to put between
// this call and a matching voe_ui_end.
voe_ui_node voe_ui_field(voe_ui_context *ui, const char *name, uint32_t index,
			 const char *text, voe_ui_sizing sizing);

// Takes the keyboard to this field, as if a press had just landed inside it.
// Called after the field's own call and before the frame ends — for the
// frame a panel holding one first opens, so a field a person is about to type
// into is not one they have to click first.
//
// A node that is not a field is the caller's bug and asserts.
void voe_ui_field_focus(voe_ui_context *ui, voe_ui_node field);

// What a frame did to one field. Read after voe_ui_frame_end, through the
// node the field call handed back; reading it before, or through a node that
// is not a field, is the caller's bug and asserts.
typedef struct {
	// This is the field the keyboard is going to: a press landed inside
	// it, voe_ui_field_focus named it, or it already was and nothing this
	// frame took the keyboard elsewhere. FALSE ON EVERY OTHER FIELD IN THE
	// FRAME, there being one focus and one edited buffer for it — see
	// `text` below.
	bool focused;
	// This frame edited it: Backspace removed a code point, typing added
	// one, or both did. False on a field that is not focused, whatever
	// the frame typed.
	bool changed;
	// Enter arrived this frame while this field was focused. Enter moves
	// no focus and changes no text, so a caller that wants "confirm and
	// move on" does both itself.
	bool entered;
	// The edited text when `changed`, and the pointer this call was given
	// when it is not — SO A CALLER MAY WRITE THIS BACK EVERY FRAME,
	// CHANGED OR NOT, AND GET THE SAME FIELD EITHER WAY. It is not the
	// caller's own buffer written into: a field cannot see how that
	// buffer is owned, only what was in it and what this frame typed —
	// the same honesty a number box's `value` is built on. Valid until
	// the next voe_ui_frame_begin, as any widget's `text` is.
	const char *text;
} voe_ui_field_result;

voe_ui_field_result voe_ui_field_action(const voe_ui_context *ui,
					voe_ui_node field);

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
// ITS BAR'S COLOURS ARE THEME ROLES — the track on `ground`, the thumb on
// control, control_hovered or the accent while held — read from the nearest
// theme in force at this call; needing none is not an option, since a bar may
// show later in the very frame that opened the area.
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
