// The editor's whole interface, on the screen-filling surface: the dock's rows,
// columns and panels turned into element records and drawn onto the window in
// one command per root. The same shape `dev/src/interface.c` has, written out
// here rather than shared — `dev` is a leaf and so is this, and neither includes
// the other's files.
//
// IT IS HANDED THE POINTER AND NEVER ASKS FOR ONE (ADR-0141 point 4). Each root
// carries its own pointer, already in this surface's millimetres; the division
// that turns the window's pixels into them is in `main.c` and in nothing else,
// because the day a panel is a quad standing in the world that division is a ray
// against the quad and only the call site can know which it is. THE KEYBOARD
// (dock.h) ARRIVES THE SAME WAY, beside the pointer and read by `ui` for
// whichever field is focused — task 14's browser is the one caller today.
//
// A ROOT IS A FRAME, WHICH IS WHY THE LOOP IS SHAPED THIS WAY. `ui` lays out one
// root container per frame (see ui/layout.h) and a root is its own surface with
// its own size, so it has its own element transform and its own range of the
// buffer. One root today, one frame, one draw command.
#pragma once

#include "browser.h"
#include "dock.h"
#include "preferences.h"
#include "session.h"
#include "themes.h"

#include <base/arena.h>
#include <math/float2.h>
#include <platform/window.h>
#include <render/device.h>
#include <text/font.h>
#include <ui/layout.h>
#include <ui/theme.h>

#include <stdint.h>

// THE ONLY CALIBRATION THIS PROGRAM HAS. Everything on the surface is drawn
// VOE_EDITOR_UI_SCALE times bigger, with that much less room to put it in.
// Nothing reads a display and there is no per-device logic behind it.
#define VOE_EDITOR_UI_SCALE 1.0f

// How tall the surface is in its own millimetres at a scale of one. The window's
// height divided by this is pixels per millimetre, which is ADR-0104's whole
// formula and the reason a window twice as tall shows the same thing twice as
// big rather than twice as much of it.
#define VOE_EDITOR_SURFACE_HIGH 135.0f

// What one frame of this interface may hold. All three are checked by `ui`, and
// a frame that wants more is refused with a line saying which number it was — so
// these are numbers to be honest about rather than careful with. A label is one
// element record per character that draws, which is what makes the second one
// much the larger of the three.
//
// THE SCROLL AREAS ARE ONE PER DOCK LEAF THAT IS NOT A SCENE VIEW, because that
// is what dock.c puts one round. The default tree has two — the Scene list and
// the Inspector — and a third panel is a third here.
//
// AND EACH OF THEM COSTS UP TO FOUR ELEMENT RECORDS OF ITS OWN, on top of
// everything the panel draws: a track and a thumb on each of the two axes, when
// both bars show. The 512 predates any itemized count and is left as it stands
// except for the two things named here that ADR-0171's border touches: dock.c's
// two leaf panels, Scene and Inspector, whose one background each is now a
// border and a fill (+2), and the Scene list's own rows — dock.c's
// voe_ui_button_begin(ui, "entity", i), up to VOE_EDITOR_SCENE_ROWS (scene.h,
// 32) of them — each a button and so gaining the same second record (+32).
// DEVIATION: the 512 is not decomposed beyond those two named contributors;
// whatever else it loosely covers is left as it stood. 512 + 2 + 32 = 546,
// before this feature's own 2 areas × 4 = 8 for their bars, 554.
//
// THE TOP BAR (topbar.h) AND THE COLUMN THIS FILE OPENS OVER IT ADD ELEVEN
// NODES AT MOST: the column itself, one; topbar.h's own panel and the row
// inside it, two more; New, Open and Save as a button and a composed label
// each, six; and one label each for the project's name and the notice. That
// is 128 + 11 = 139 nodes.
//
// AND THEY ADD EIGHT BACKGROUNDS, ELEVEN CHARACTERS AND ROOM FOR TWO HUNDRED
// MORE. The column and the row draw none of their own; the panel and each of
// the three buttons draw a hairline border now (ADR-0171), two records apiece
// where one used to do — eight where four used to be; the eleven characters
// "New", "Open" and "Save" draw between them are eleven, and two hundred more
// is generous for whatever a project's name and a notice's line come to —
// neither this file nor topbar.h puts a limit on how long either string is.
// That is 554 + 8 + 11 + 200 = 773 elements.
//
// THE BROWSER (browser.h), WHEN IT SHOWS, ADDS ANOTHER SCROLL AREA — ITS OWN
// LIST OF ROWS — ON TOP OF THE DOCK'S TWO, so VOE_EDITOR_INTERFACE_SCROLLS is
// three. Its own area scrolls one axis, not two, so it costs up to two more
// element records rather than the dock areas' four — a track and a thumb, on
// Y alone.
//
// AND IT ADDS AT MOST 107 NODES: the panel, one; the row above the rows and
// what is in it — the path label, the Up button and its own label — four
// more; the scroll area, one; up to VOE_EDITOR_BROWSER_ROWS (32) rows, each a
// button, a name label and — marked — a second label for " — project", three
// apiece, ninety-six; the row below the rows, one; and Confirm and Cancel as
// a button and a label each, four. That is 11 + 96 = 107, and 139 + 107 = 246
// nodes.
//
// AND 1234 MORE ELEMENTS. Exactly, on top of what is generous: the panel's
// background and each of the Up, Confirm and Cancel buttons' are eight now,
// each drawing a hairline border (ADR-0171), two records apiece where one
// used to do; "Up" draws two characters, "Cancel" six, and "Save here" — the
// longer of the two Confirm ever shows, its one space drawing nothing —
// eight; the scrollbar up to two; and every one of the 32 rows is a button
// too, so its own background is two rather than one, and it may read
// " — project", whose em dash and "project" draw eight characters and whose
// two spaces draw none — that is 8 + 2 + 6 + 8 + 2 + 32 × (2 + 8) = 346. And
// generous, exactly as a project's name and a notice's line are above: a
// hundred and twenty characters for the current path, and twenty-four
// apiece for the 32 rows' own folder names, neither of which this file nor
// browser.h puts a limit on — 120 + 32 × 24 = 888. That is 346 + 888 = 1234,
// and 773 + 1234 = 2007 elements.
//
// AND, IN SAVE MODE, THE NAME ROW (browser.h, task 14) ADDS FOUR MORE NODES:
// a field costs two — itself and the label it composes (ui/widgets.h) — and
// Make folder, a button with a label, costs two more. 246 + 4 = 250 nodes.
//
// AND 270 MORE ELEMENTS, BOUNDED BY THE FIELD'S OWN DECLARED CAPACITY RATHER
// THAN A GUESS. The field draws no border — only a panel and a button do
// (ADR-0171) — so its background and its caret while focused stay two; up to
// VOE_UI_FIELD_CAPACITY (256) characters of whatever was typed is the honest
// ceiling, there being nowhere shorter this file could point to; that is 258.
// Make folder IS a button, so its background is two now rather than one,
// plus its ten letters ("Make folder", the one space between them drawing
// nothing) — twelve more. 258 + 12 = 270, and 2007 + 270 = 2277 elements.
//
// AND INSPECTOR.C'S PER-COMPONENT PANELS, NAMED BY NEITHER NUMBER ABOVE UNTIL
// NOW. Every component type the selected entity has a row of gets its own
// RAISED panel (inspector.c's component_panel), and this editor's world never
// holds more than eight component types at once — project.c's world_new is
// the only place that decides it, and ecs/component.h asserts rather than
// lets a world grow past what it was made with — so at most eight are drawn
// a frame. Each is a panel and its heading label, two nodes, and — the
// border ADR-0171 draws — two elements for the border and the fill plus
// twenty-four generous for whatever its component's key is named, this file
// naming none of them (inspector.h's own claim). 8 × 2 = 16 nodes,
// 8 × 26 = 208 elements.
//
// AND ITS ROWS, BOUNDED BY VOE_EDITOR_INSPECTOR_CONTROLS (inspector.h)
// RATHER THAN BY WHAT ANY ONE COMPONENT DESCRIBES: at most 64 across every
// panel together, three nodes each as inspector.h already counts —
// 192 nodes. The most one costs is a boolean's button rather than a number
// box's plain rectangle — two elements for its own border and fill
// (ADR-0171) — plus sixteen generous for the field's name and the word it
// shows, neither of which this file nor inspector.c puts a limit on.
// 64 × (2 + 16) = 1152 elements. 250 + 16 + 192 = 458 nodes, and
// 2277 + 208 + 1152 = 3637 elements.
//
// THE BAR'S PREFERENCES BUTTON (topbar.h) ADDS TWO NODES, a button and its
// label, and THIRTEEN ELEMENTS: its border and fill, and the eleven letters
// of "Preferences". 458 + 2 = 460 nodes, 3637 + 13 = 3650 elements.
//
// PREFERENCES (preferences.h) ADDS NOTHING, BECAUSE IT IS NEVER DRAWN IN THE
// SAME FRAME AS THE BROWSER AND COSTS LESS. It takes the browser's place: the
// panel, one; its scroll area, one; up to VOE_EDITOR_PREFERENCES_ROWS (16)
// rows, each a panel, a row, the name label and Choose as a button and a
// label, five apiece, eighty; the one "(in force)" label; Close's row, and
// Close as a button and a label, three — 86 nodes against the browser's 111.
// Elements: the panel's border and fill, two; the scrollbar, two; each row's
// panel and Choose button, four, "Choose", six, and twenty-four generous for
// the theme's name, thirty-four apiece, 544; "(in force)", nine, its space
// drawing nothing; Close, two and five — 564 against the browser's 1504. Its
// one scroll area is the one the browser would have used.
//
// THE SCENE PANEL'S ADD MENU (scene.h) ADDS TEN NODES: Add as a button and its
// label, two; and, while it shows, Entity, Cube, Capsule and Cylinder as a
// button and a label each, eight. 460 + 10 = 470 nodes. AND THIRTY-EIGHT
// ELEMENTS: each of the five buttons' border and fill, ten; and the letters of
// "Add", "Entity", "Cube", "Capsule" and "Cylinder", 3 + 6 + 4 + 7 + 8 = 28.
// 3650 + 38 = 3688 elements.
//
// THE INSPECTOR'S DUPLICATE AND DELETE ROW (inspector.h) ADDS FIVE NODES: the
// row, and each button and its label. 470 + 5 = 475 nodes. AND NINETEEN
// ELEMENTS: each button's border and fill, four, and the letters of
// "Duplicate" and "Delete", 9 + 6 = 15. 3688 + 19 = 3707 elements.
//
// THE INSPECTOR'S REMOVE, NEEDS AND ADD COMPONENT (inspector.h) ADD FIFTY
// NODES. Each of the eight sections gains a row round its heading, a Remove
// button and its label, and a "Needs" label, four apiece, thirty-two; Add
// component is a button and a label, two; and its choices, one per type the
// entity lacks and so at most eight, a button and a label each, sixteen.
// 475 + 50 = 525 nodes. AND FIVE HUNDRED AND EIGHTEEN ELEMENTS: each section's
// Remove border and fill, two, "Remove", six, and "Needs", five, beside
// twenty-four generous for the needed heading, thirty-seven apiece, 296; Add
// component's border and fill and its twelve letters, fourteen; each choice's
// border and fill and twenty-four generous for its heading, twenty-six apiece,
// 208. 3707 + 296 + 14 + 208 = 4225 elements.
//
// THE COLOUR PICKER (interface.c) ADDS SEVEN NODES: the six ui/colour.h states
// and the anchored column this file puts round it. 525 + 7 = 532 nodes. AND
// SEVEN HUNDRED AND TEN ELEMENTS: ui/colour.h's 461 while the hex field holds
// seven characters, and one more for each typed past them up to the field's
// VOE_UI_FIELD_CAPACITY (256), 249. 4225 + 710 = 4935 elements. It is never
// drawn beside the browser or Preferences, and costs less than either, but it
// is counted on top, as the card asked. A colour field's swatch is a button and
// one solid element, inside the three nodes and eighteen elements a control
// is already counted at above.
//
// PREFERENCES' TWO SLIDERS AND RESET (preferences.h, task 14) ARE COUNTED ON
// TOP ALL THE SAME, as the picker above is: FIFTEEN NODES — the column under
// the list, one; a row per scalar, two; each scalar's name label, two; each
// slider, three of its own (ui/slider.h), six; each value label, two; and
// Reset as a button and a label, two. 532 + 15 = 547 nodes. AND THREE HUNDRED
// AND THREE ELEMENTS: "Contrast", eight, and "Surface separation", seventeen,
// its one space drawing nothing; each slider's three, the number box's fill
// and the thumb's border and surface, six; each value label's four characters,
// the whole range printing as `%.2f` of one digit, eight; Reset's border and
// fill and its five letters, seven — 46; and, while one slider is open for
// typing (ADR-0192), a caret and up to VOE_UI_FIELD_CAPACITY (256) characters
// of whatever was typed, 257, one at a time because one thing holds the
// keyboard. 4935 + 46 + 257 = 5238 elements.
//
// THE OPEN DROPDOWN (inspector.c) ADDS THIRTY-SIX NODES. It is drawn inside
// the Inspector's own scroll area and clipped by it rather than over the rest
// of the editor, an overlay belonging to the widget it opened from (ADR-0199).
// Thirty-five are the list: the anchored column round it, one; its panel, one;
// the scroll area its rows sit in, one; and up to VOE_EDITOR_DROPDOWN_ROWS
// (inspector.h, 16) rows, each a choice button and the label composed into it,
// thirty-two. The thirty-sixth is the content column inspector.c opens round
// everything that panel draws, which is what the list is anchored to.
// 547 + 35 + 1 = 583. AND FOUR HUNDRED AND TWENTY ELEMENTS: the panel's border
// and fill, two; each row's border and fill, thirty-two; twenty-four generous
// for each row's name, this file naming none of them and neither `base` nor the
// declaring folder putting a length on one, 384 — a column drawing none of its
// own; and the rows' area a track and a thumb on Y alone, two, which is what
// the browser's own area is already counted at in this file. 5238 + 420 = 5658.
// It is never drawn beside the colour picker, because opening either closes the
// other (scene.h), and it is counted on top all the same. AND ONE MORE SCROLL
// AREA, the rows', on top of the dock's two and the browser's one, counted on
// top all the same for the reason the picker's nodes are:
// VOE_EDITOR_INTERFACE_SCROLLS is four.
#define VOE_EDITOR_INTERFACE_NODES 583
#define VOE_EDITOR_INTERFACE_ELEMENTS 5658
#define VOE_EDITOR_INTERFACE_SCROLLS 4

// Makes the context the interface is built in, once, drawing in `theme` and
// with the font that theme was derived with — ui measures and draws every
// label from the context's one font, so it is the theme's (themes.h). It
// lives in `arena` and is freed with it; the theme and its font must outlive
// it.
voe_ui_context *voe_editor_interface_new(voe_base_arena *arena,
					 const voe_ui_theme *theme);

// How big the surface is on a window of this size, and what one of its
// millimetres is worth in pixels. Both answers come out of one call because they
// come out of one division, and the caller needs each for a different reason:
// the size is what a root is laid out in, and the scale is what the pointer's
// pixels are divided by.
//
// A window with no area is the caller's bug and asserts — the reciprocal of
// nothing is what would otherwise reach a matrix.
void voe_editor_interface_surface(voe_platform_size target,
				  voe_math_float2 *millimetres,
				  float *pixels_per_millimetre);

// Builds, submits and draws every root's interface into the open frame, over the
// whole render target. Called when the draw is open and before it is closed; it
// is not in any world, so it has nothing to sort against and issues its own
// draws.
//
// False when a frame was refused — more nodes, more records or more scroll areas
// than the three numbers above — or when a submit or a draw was refused. All of those are this
// program's numbers being wrong, and whichever it was has already said so on
// stderr.
//
// `scene` is handed through to the panels and is also where THIS FRAME'S CLICKS
// LAND. The read has to happen in here and cannot be the caller's: a widget
// answers only between voe_ui_frame_end and the rewind of the arena its nodes
// were pushed out of (ui/widgets.h), and both of those are this function's.
// `views` is handed through the same way, and where each scene view's picture
// came to sit is read back into it in the same window, for the same reason.
//
// EACH ROOT GETS THE TOP BAR ABOVE ITS DOCK TREE, DRAWN FROM `session`, FOR
// THE SAME REASON AGAIN. The bar takes VOE_EDITOR_TOPBAR_HIGH off the top of
// the root's own height and hands the dock tree the rest, in a column this
// function opens as the frame's actual root; the tree itself is dock.c's
// unchanged, only nested one level deeper than it used to be, which is why
// voe_editor_dock_walk is called with VOE_EDITOR_DOCK_COLUMN (dock.h) — a
// child's declared size is read against its PARENT's flow and not its own
// (ui/layout.h), and the parent is now this column rather than the frame
// itself. A button that fired is carried out on `session` before this root's
// records are submitted, through voe_editor_session_do, which is the reason
// `session` and not just its notice and its project's name are handed in.
//
// `browser` IS DRAWN OVER THE DOCK, IN THE SAME COLUMN, WHEN IT IS SHOWING —
// see browser.h. WHETHER IT WAS SHOWING IS CAPTURED BEFORE ANYTHING IS DRAWN
// AND USED FOR EVERY DECISION BELOW, rather than read again after: a click
// read this frame answers for what was actually laid out this frame, and a
// voe_editor_session_do that shows the browser for the OPEN it was just given
// must not make this same frame try to read clicks on rows nothing drew. So
// while it was showing when the frame was built, the dock's own root is
// handed a pointer with `over` false — the anchored browser already paints
// over every one of its leaves, so nothing under it can be hit first, and
// this is the honest copy of that fact — and the top bar's buttons, though
// still drawn, are never asked what the pointer did to them at all, which is
// what "ignored" means for a mouse click rather than a shortcut main.c never
// sends in the first place; voe_editor_browser_clicks_read is asked instead,
// and what it reports is carried out through voe_editor_session_browser_do.
// Otherwise — the browser was not showing when this frame was built — the
// bar's click is read and, if one fired, carried out through
// voe_editor_session_do exactly as before, and the browser is not read at
// all, there being nothing drawn on it to answer for. `escape` is this
// frame's Escape key edge, read as the browser's Cancel — main.c's to
// compute, there being no window in here to ask (ADR-0141 point 4).
//
// `preferences` IS DRAWN THE SAME WAY, WHEN IT IS SHOWING AND THE BROWSER IS
// NOT — the browser covers the same area and wins. Whether it was drawn is
// captured once per root's frame, as `browsing` is; while it was, the dock is
// handed a pointer with `over` false for the same reason, and its clicks are
// read beside the top bar's, which stays live. The bar's Preferences shows
// it; Choose puts `themes`' entry in force through voe_editor_themes_choose
// and sets that palette and its font on `ui`, saying in the session's notice
// when it could not be remembered; Close hides it.
//
// THE COLOUR PICKER IS DRAWN OVER THE DOCK TOO, beside the Inspector column,
// while scene.h's `picking` shows and neither the browser nor Preferences does
// — either of those closes it. What it changes is submitted at once as the
// row's replace intent and counted in `scene->inspector.replaced`; a press
// outside it closes it. See interface.c.
//
// AND SO IS THE OPEN DROPDOWN (scene.h), hanging from the control that opened
// it, while it shows — never beside the picker, because opening either closes
// the other. A row chosen on it is submitted at once as that row's replace
// intent and counted the same way; `escape` closes it, as does a press outside
// its rectangle. See interface.c.
[[nodiscard]] bool voe_editor_interface_draw(voe_render_device *gpu,
					     voe_ui_context *ui,
					     voe_base_arena *arena,
					     const voe_editor_dock_root *roots,
					     uint32_t count,
					     voe_editor_scene *scene,
					     voe_editor_views *views,
					     voe_editor_session *session,
					     voe_editor_browser *browser,
					     voe_editor_preferences *preferences,
					     voe_editor_themes *themes,
					     bool escape);
