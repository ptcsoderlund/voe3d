// The tank game's screens, drawn once a frame over the level (0334 points 7
// and 8), and the half of the state module that answers them.
//
//     return tank_menu_run(frame);          // in voe_game_project_interface
//
// THE SCREENS, one per phase, on one root over frame->size, each item a
// choice drawn inverted while it is the chosen one:
//   menu     a centred panel: Start, Quit;
//   playing  a HUD panel at the top left: `Score N` and `Lives N`;
//   paused   a centred panel: Resume, Menu, Quit;
//   won      a centred panel: "You win", the score, Menu, Quit;
//   lost     a centred panel: "Game over", the score, Menu, Quit.
// With no row yet the frame is ended, nothing drawn, and the run not paused.
//
// THE KEYS, the pad being the lowest connected slot: W, S, the d-pad and the
// left stick past 0.5 move the choice, wrapping; Enter, Space or the pad's
// south button press it, and a click presses the item clicked. Escape or
// Start pauses while playing, at choice 0; paused, Escape, Start or the east
// button resumes.
//
// EACH IS AN EDGE, the level going down against last frame's, kept a bit each
// in the row's `held` and written every frame: platform reports levels only
// (input.h), and a key held down must press once, not once a frame. Headless
// there is no window and no key is down.
//
// THE ITEMS: Start and Resume set playing; Menu asks the run to restart
// (0333), which builds a fresh world that starts at the menu; Quit returns
// false. A phase changed here starts at choice 0. Then asks->paused is
// whether the phase is paused, won or lost, so no step runs behind them.
//
// Constraints: this module and tank_state_system.c are the row's one writer,
// the screens once a frame, the step once a step; the row is written whole.
// A refused ui frame presses no item by click, but a key still does. The
// lives' label waits for the lives row, made on the first step playing.
#include "tank_state.h"
#include "tank_lives.h"

#include <base/assert.h>

#include <ecs/component.h>

#include <game/project.h>

#include <platform/input.h>

#include <ui/widgets.h>

#include <stdio.h>

#define TANK_MENU_STICK 0.5f
#define TANK_MENU_ITEMS 3

// The bits of tank_state's `held`, one per input the screens read.
enum {
	TANK_MENU_HELD_UP = 1u << 0,
	TANK_MENU_HELD_DOWN = 1u << 1,
	TANK_MENU_HELD_PRESS = 1u << 2,
	TANK_MENU_HELD_PAUSE = 1u << 3,
	TANK_MENU_HELD_BACK = 1u << 4,
};

// What an item, or a key beside the items, does.
enum {
	TANK_MENU_DOES_NOTHING,
	TANK_MENU_DOES_START,
	TANK_MENU_DOES_RESUME,
	TANK_MENU_DOES_MENU,
	TANK_MENU_DOES_QUIT,
	TANK_MENU_DOES_PAUSE,
};

// A screen's items in call order and what each does; the texts its labels
// read, alive until the frame ends.
typedef struct {
	voe_ui_node nodes[TANK_MENU_ITEMS];
	uint32_t does[TANK_MENU_ITEMS];
	uint32_t count;
	char score[32];
	char lives[32];
} tank_menu_screen;

// One item labelled `text` doing `does`, appended to `screen`, inverted when
// it is the `selected` one.
static void tank_menu_item(voe_ui_context *ui, tank_menu_screen *screen,
			   uint32_t selected, const char *text, uint32_t does)
{
	VOE_BASE_ASSERT(ui != NULL && screen != NULL, "an item on no screen");
	VOE_BASE_ASSERT(screen->count < TANK_MENU_ITEMS, "too many items");
	screen->nodes[screen->count] = voe_ui_choice_begin(
		ui, "item", screen->count, screen->count == selected);
	voe_ui_label(ui, text);
	voe_ui_end(ui);
	screen->does[screen->count++] = does;
}

// The HUD's labels: the score, and the lives once their row exists.
static void tank_menu_hud(const voe_game_project_frame *frame,
			  tank_menu_screen *screen)
{
	VOE_BASE_ASSERT(frame != NULL && screen != NULL, "a HUD of nothing");
	const voe_ecs_type type =
		voe_ecs_component_type(frame->world, &tank_lives_key);

	voe_ui_label(frame->ui, screen->score);
	if (voe_ecs_component_count(frame->world, type) == 0)
		return;
	const tank_lives *lives = voe_ecs_component_rows(frame->world, type);

	(void)snprintf(screen->lives, sizeof(screen->lives), "Lives %d",
		       (int)lives->lives);
	voe_ui_label(frame->ui, screen->lives);
}

// The panel's contents for the row's phase, the root and the panel open.
static void tank_menu_contents(const voe_game_project_frame *frame,
			       const tank_state *row, tank_menu_screen *screen)
{
	VOE_BASE_ASSERT(frame != NULL && row != NULL && screen != NULL,
			"a screen of nothing");
	voe_ui_context *ui = frame->ui;
	const uint32_t at = row->selected;

	(void)snprintf(screen->score, sizeof(screen->score), "Score %d",
		       (int)row->score);
	if (row->phase == TANK_PHASE_PLAYING) {
		tank_menu_hud(frame, screen);
		return;
	}
	if (row->phase == TANK_PHASE_MENU) {
		tank_menu_item(ui, screen, at, "Start", TANK_MENU_DOES_START);
	} else if (row->phase == TANK_PHASE_PAUSED) {
		tank_menu_item(ui, screen, at, "Resume", TANK_MENU_DOES_RESUME);
		tank_menu_item(ui, screen, at, "Menu", TANK_MENU_DOES_MENU);
	} else {
		voe_ui_label(ui, row->phase == TANK_PHASE_WON ? "You win" :
								"Game over");
		voe_ui_label(ui, screen->score);
		tank_menu_item(ui, screen, at, "Menu", TANK_MENU_DOES_MENU);
	}
	tank_menu_item(ui, screen, at, "Quit", TANK_MENU_DOES_QUIT);
	VOE_BASE_ASSERT(screen->count > 0, "a screen with no item");
}

// Lays out the row's screen and ends the frame. True when the frame laid out.
static bool tank_menu_draw(const voe_game_project_frame *frame,
			   const tank_state *row, tank_menu_screen *screen)
{
	VOE_BASE_ASSERT(frame != NULL && row != NULL && screen != NULL,
			"drawing no screen");
	const bool hud = row->phase == TANK_PHASE_PLAYING;

	voe_ui_column_begin(frame->ui, (voe_ui_container){
		.size = { .along = { VOE_UI_SIZE_FIXED, frame->size.y },
			  .across = { VOE_UI_SIZE_FIXED, frame->size.x } },
		.along = hud ? VOE_UI_ALONG_START : VOE_UI_ALONG_CENTER,
		.across = hud ? VOE_UI_ACROSS_START : VOE_UI_ACROSS_CENTER,
		.pad = { 4.0f, 4.0f, 4.0f, 4.0f } });
	voe_ui_panel_begin(frame->ui, "screen", row->phase,
			   VOE_UI_SURFACE_RAISED,
			   (voe_ui_container){ .across = VOE_UI_ACROSS_FILL,
					       .gap = 2.0f,
					       .pad = { 4.0f, 4.0f, 4.0f,
							4.0f } });
	tank_menu_contents(frame, row, screen);
	voe_ui_end(frame->ui);
	voe_ui_end(frame->ui);
	return voe_ui_frame_end(frame->ui);
}

// This frame's levels, a TANK_MENU_HELD_ bit each; 0 headless.
static uint32_t tank_menu_levels(voe_platform_window *window)
{
	if (window == NULL)
		return 0;
	voe_platform_gamepad pad = { 0 };

	for (int slot = 0; slot < VOE_PLATFORM_GAMEPAD_SLOTS && !pad.connected;
	     slot++)
		pad = voe_platform_input_gamepad(window, slot);
	const bool *b = pad.buttons;
	uint32_t levels = 0;

	if (voe_platform_input_key_down(window, VOE_PLATFORM_KEY_W) ||
	    b[VOE_PLATFORM_GAMEPAD_BUTTON_PAD_UP] || pad.left_y > TANK_MENU_STICK)
		levels |= TANK_MENU_HELD_UP;
	if (voe_platform_input_key_down(window, VOE_PLATFORM_KEY_S) ||
	    b[VOE_PLATFORM_GAMEPAD_BUTTON_PAD_DOWN] ||
	    pad.left_y < -TANK_MENU_STICK)
		levels |= TANK_MENU_HELD_DOWN;
	if (voe_platform_input_key_down(window, VOE_PLATFORM_KEY_ENTER) ||
	    voe_platform_input_key_down(window, VOE_PLATFORM_KEY_SPACE) ||
	    b[VOE_PLATFORM_GAMEPAD_BUTTON_SOUTH])
		levels |= TANK_MENU_HELD_PRESS;
	if (voe_platform_input_key_down(window, VOE_PLATFORM_KEY_ESCAPE) ||
	    b[VOE_PLATFORM_GAMEPAD_BUTTON_START])
		levels |= TANK_MENU_HELD_PAUSE;
	if (b[VOE_PLATFORM_GAMEPAD_BUTTON_EAST])
		levels |= TANK_MENU_HELD_BACK;
	return levels;
}

// What this frame does: the item clicked, else the chosen one pressed, else a
// pause or a resume key; nothing when none.
static uint32_t tank_menu_does(const voe_game_project_frame *frame,
			       const tank_menu_screen *screen, bool laid,
			       const tank_state *row, uint32_t edges)
{
	VOE_BASE_ASSERT(frame != NULL && screen != NULL && row != NULL,
			"answering no screen");
	for (uint32_t i = 0; laid && i < screen->count; i++)
		if (voe_ui_button_action(frame->ui, screen->nodes[i]).fired)
			return screen->does[i];
	if ((edges & TANK_MENU_HELD_PRESS) != 0 && row->selected < screen->count)
		return screen->does[row->selected];
	if (row->phase == TANK_PHASE_PLAYING && (edges & TANK_MENU_HELD_PAUSE))
		return TANK_MENU_DOES_PAUSE;
	if (row->phase == TANK_PHASE_PAUSED &&
	    (edges & (TANK_MENU_HELD_PAUSE | TANK_MENU_HELD_BACK)))
		return TANK_MENU_DOES_RESUME;
	return TANK_MENU_DOES_NOTHING;
}

// `selected` moved by this frame's up and down edges, wrapping in `count`.
static uint32_t tank_menu_moved(uint32_t selected, uint32_t count,
				uint32_t edges)
{
	VOE_BASE_ASSERT(count > 0 && count <= TANK_MENU_ITEMS,
			"moving among no items");
	if (edges & TANK_MENU_HELD_UP)
		selected = (selected + count - 1) % count;
	if (edges & TANK_MENU_HELD_DOWN)
		selected = (selected + 1) % count;
	return selected;
}

bool tank_menu_run(const voe_game_project_frame *frame)
{
	VOE_BASE_ASSERT(frame != NULL && frame->ui != NULL &&
				frame->world != NULL && frame->asks != NULL,
			"drawing the tank game's screens in no frame");
	const tank_state *current = tank_state_get(frame->world);

	if (current == NULL) {
		(void)voe_ui_frame_end(frame->ui);
		frame->asks->paused = false;
		return true;
	}
	tank_state row = *current;
	tank_menu_screen screen = { 0 };
	const bool laid = tank_menu_draw(frame, &row, &screen);
	const uint32_t levels = tank_menu_levels(frame->window);
	const uint32_t edges = levels & ~row.held;
	const uint32_t does = tank_menu_does(frame, &screen, laid, &row, edges);
	const uint32_t was = row.phase;

	row.held = levels;
	if (does == TANK_MENU_DOES_START || does == TANK_MENU_DOES_RESUME)
		row.phase = TANK_PHASE_PLAYING;
	if (does == TANK_MENU_DOES_PAUSE)
		row.phase = TANK_PHASE_PAUSED;
	if (does == TANK_MENU_DOES_MENU)
		frame->asks->restart = true;
	if (does == TANK_MENU_DOES_NOTHING && screen.count > 0)
		row.selected = tank_menu_moved(row.selected, screen.count, edges);
	if (row.phase != was || row.selected >= screen.count)
		row.selected = 0;
	const voe_ecs_type type =
		voe_ecs_component_type(frame->world, &tank_state_key);
	const bool ok = voe_ecs_component_set(
		frame->world, type,
		voe_ecs_component_entities(frame->world, type)[0], &row);

	VOE_BASE_ASSERT(ok, "tank_state vanished while answering its screen");
	frame->asks->paused = row.phase == TANK_PHASE_PAUSED ||
			      row.phase == TANK_PHASE_WON ||
			      row.phase == TANK_PHASE_LOST;
	return does != TANK_MENU_DOES_QUIT;
}
