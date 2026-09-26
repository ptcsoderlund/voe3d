// The coin game's screens, drawn once a frame over the level (0259), and the
// half of the game module that answers them.
//
//     if (!game_interface_run(frame))           // in voe_game_project_interface
//             return false;                     // Quit: the run ends
//
// THE SCREENS, one per phase, on one root over frame->size:
//   menu     a centred panel: Start, Quit;
//   playing  a HUD panel at the top left: the score and the coins left;
//   won      a centred panel: "You win", the final score, Restart, Quit;
//   lost     a centred panel: "Game over", Restart, Quit.
//
// THE KEYS: Enter's press presses a screen's first button, Escape's its last.
// A press is an edge, the level going down against last frame's level, which
// is kept in the row (enter_down, escape_down) so a held key presses once.
// Headless there is no window and no key is down.
//
// THE BUTTONS: Start sets playing. Restart sets playing, bumps restarts and
// zeroes dropped and banked; the coin and player systems read the new count.
// Quit returns false. With no row yet the frame is ended and nothing drawn.
//
// Constraints: this module and game_system.c are the row's one writer (0260).
// A refused ui frame presses no button, but a key still does.
#include "game_state.h"

#include <base/assert.h>

#include <ecs/component.h>

#include <game/project.h>

#include <platform/input.h>

#include <ui/widgets.h>

#include <stdio.h>

enum {
	GAME_INTERFACE_NOTHING,
	GAME_INTERFACE_START,
	GAME_INTERFACE_RESTART,
	GAME_INTERFACE_QUIT,
};

// A screen's buttons in call order and what each does; the texts its labels
// read, alive until the frame ends.
typedef struct {
	voe_ui_node nodes[2];
	uint32_t does[2];
	uint32_t count;
	char score[32];
	char coins[32];
} game_interface_screen;

// One button labelled `text` doing `does`, appended to `screen`.
static void game_interface_button(voe_ui_context *ui,
				  game_interface_screen *screen,
				  const char *text, uint32_t does)
{
	VOE_BASE_ASSERT(ui != NULL && screen != NULL, "a button on no screen");
	VOE_BASE_ASSERT(screen->count < 2, "a third button on one screen");
	screen->nodes[screen->count] = voe_ui_button_begin(ui, text, 0);
	voe_ui_label(ui, text);
	voe_ui_end(ui);
	screen->does[screen->count++] = does;
}

// The panel's contents for `phase`, the root and the panel already open.
static void game_interface_contents(const voe_game_project_frame *frame,
				    uint32_t phase,
				    game_interface_screen *screen)
{
	VOE_BASE_ASSERT(frame != NULL && screen != NULL, "a screen of nothing");
	voe_ui_context *ui = frame->ui;
	const int32_t score = game_score(frame->world);

	(void)snprintf(screen->score, sizeof(screen->score), "Score %d",
		       (int)score);
	(void)snprintf(screen->coins, sizeof(screen->coins), "%u coins left",
		       (unsigned)game_coins_left(frame->world));
	if (phase == GAME_PHASE_PLAYING) {
		voe_ui_label(ui, screen->score);
		voe_ui_label(ui, screen->coins);
		return;
	}
	if (phase == GAME_PHASE_MENU) {
		game_interface_button(ui, screen, "Start", GAME_INTERFACE_START);
	} else {
		voe_ui_label(ui, phase == GAME_PHASE_WON ? "You win" :
							   "Game over");
		if (phase == GAME_PHASE_WON)
			voe_ui_label(ui, screen->score);
		game_interface_button(ui, screen, "Restart",
				      GAME_INTERFACE_RESTART);
	}
	game_interface_button(ui, screen, "Quit", GAME_INTERFACE_QUIT);
	VOE_BASE_ASSERT(screen->count > 0, "an end screen with no button");
}

// Lays out `phase`'s screen and ends the frame. True when the frame laid out.
static bool game_interface_draw(const voe_game_project_frame *frame,
				uint32_t phase, game_interface_screen *screen)
{
	VOE_BASE_ASSERT(frame != NULL && screen != NULL, "drawing no screen");
	const bool hud = phase == GAME_PHASE_PLAYING;
	const voe_ui_along where = hud ? VOE_UI_ALONG_START :
					 VOE_UI_ALONG_CENTER;

	voe_ui_column_begin(frame->ui, (voe_ui_container){
		.size = { .along = { VOE_UI_SIZE_FIXED, frame->size.y },
			  .across = { VOE_UI_SIZE_FIXED, frame->size.x } },
		.along = where,
		.across = hud ? VOE_UI_ACROSS_START : VOE_UI_ACROSS_CENTER,
		.pad = { 4.0f, 4.0f, 4.0f, 4.0f } });
	voe_ui_panel_begin(frame->ui, "screen", phase, VOE_UI_SURFACE_RAISED,
			   (voe_ui_container){ .across = VOE_UI_ACROSS_FILL,
					       .gap = 2.0f,
					       .pad = { 4.0f, 4.0f, 4.0f,
							4.0f } });
	game_interface_contents(frame, phase, screen);
	voe_ui_end(frame->ui);
	voe_ui_end(frame->ui);
	return voe_ui_frame_end(frame->ui);
}

// Which button was pressed, by click or by key edge; count when none.
static uint32_t game_interface_pressed(const voe_game_project_frame *frame,
				       const game_interface_screen *screen,
				       bool laid, const game_state *before,
				       const game_state *now)
{
	VOE_BASE_ASSERT(screen != NULL && before != NULL && now != NULL,
			"pressing on no screen");
	if (screen->count == 0)
		return 0;
	if (now->enter_down && !before->enter_down)
		return 0;
	if (now->escape_down && !before->escape_down)
		return screen->count - 1;
	for (uint32_t i = 0; laid && i < screen->count; i++) {
		if (voe_ui_button_action(frame->ui, screen->nodes[i]).fired)
			return i;
	}
	return screen->count;
}

bool game_interface_run(const voe_game_project_frame *frame)
{
	VOE_BASE_ASSERT(frame != NULL && frame->ui != NULL &&
				frame->world != NULL,
			"drawing the coin game's screens in no frame");
	const game_state *current = game_state_get(frame->world);

	if (current == NULL) {
		(void)voe_ui_frame_end(frame->ui);
		return true;
	}
	game_state next = *current;
	game_interface_screen screen = { 0 };
	const bool laid = game_interface_draw(frame, next.phase, &screen);

	next.enter_down = frame->window != NULL &&
		voe_platform_input_key_down(frame->window, VOE_PLATFORM_KEY_ENTER);
	next.escape_down = frame->window != NULL &&
		voe_platform_input_key_down(frame->window, VOE_PLATFORM_KEY_ESCAPE);
	const uint32_t pressed =
		game_interface_pressed(frame, &screen, laid, current, &next);
	const uint32_t does = pressed < screen.count ? screen.does[pressed] :
						       GAME_INTERFACE_NOTHING;

	if (does == GAME_INTERFACE_START || does == GAME_INTERFACE_RESTART)
		next.phase = GAME_PHASE_PLAYING;
	if (does == GAME_INTERFACE_RESTART) {
		next.restarts++;
		next.dropped = 0;
		next.banked = 0.0;
	}
	const voe_ecs_type type =
		voe_ecs_component_type(frame->world, &game_state_key);
	const bool ok = voe_ecs_component_set(
		frame->world, type,
		voe_ecs_component_entities(frame->world, type)[0], &next);

	VOE_BASE_ASSERT(ok, "game_state vanished while answering its screen");
	return does != GAME_INTERFACE_QUIT;
}
