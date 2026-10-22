// The question Import textures asks (ADR 0466 point 4): a panel "Import
// textures of <file>" with a "Largest size" row of 512, 1024, 2048 and 4096,
// the chosen one lit, then Import and Cancel, as an anchored panel over the
// dock below the bar, drawn and read the way levels_ask.h's question is.
//
//     voe_editor_textures_ask_open(&ask, "models/a.glb");
//     voe_editor_textures_ask_draw(ui, &ask, top);
//     ... voe_ui_frame_end ...
//     answer = voe_editor_textures_ask_read(ui, &ask, pointer_down, at);
//     if (answer == VOE_EDITOR_TEXTURES_ASK_IMPORT)
//             ... import the textures of ask.path at most ask.largest ...
//     if (answer != VOE_EDITOR_TEXTURES_ASK_NONE)
//             voe_editor_textures_ask_close(&ask);
//
// EVERY OPENING CHOOSES 2048: nothing is remembered from the last one, so a
// re-run's size never depends on what was done to another model (0466 point
// 4). A press on a size only changes the chosen one. Nothing here imports a
// picture or closes the question: the caller carries out the answer.
//
// Constraints. The path is cut at VOE_EDITOR_ASSETS_PATH bytes, the room the
// Assets panel holds a path in.
#pragma once

#include "assets_panel.h"

#include <math/float2.h>

#include <ui/widgets.h>

#include <stdbool.h>
#include <stdint.h>

#define VOE_EDITOR_TEXTURES_ASK_SIZES 4

// Zeroed is a question never asked.
typedef struct {
	bool open;
	// Relative to `Assets/`, as the Assets menu hands it.
	char path[VOE_EDITOR_ASSETS_PATH];
	// The chosen largest side in pixels, one of the four sizes.
	uint32_t largest;
	// Kept here because a label's text is drawn after the call that made it.
	char title[VOE_EDITOR_ASSETS_PATH + sizeof "Import textures of "];
	voe_ui_node panel;
	voe_ui_node size_buttons[VOE_EDITOR_TEXTURES_ASK_SIZES];
	voe_ui_node import_button;
	voe_ui_node cancel_button;
} voe_editor_textures_ask;

typedef enum {
	VOE_EDITOR_TEXTURES_ASK_NONE = 0,
	VOE_EDITOR_TEXTURES_ASK_IMPORT,
	VOE_EDITOR_TEXTURES_ASK_CANCEL,
} voe_editor_textures_ask_answer;

// Opens the question about the `.glb` at `path`, `largest` 2048 and the title
// naming the file's name.
void voe_editor_textures_ask_open(voe_editor_textures_ask *ask,
				  const char *path);
void voe_editor_textures_ask_close(voe_editor_textures_ask *ask);

// Draws the question centred across the surface, `top` below its top, over the
// dock: the title, the size row with `largest`'s lit, and Import and Cancel,
// and records its nodes. Asserts when it is not open.
void voe_editor_textures_ask_draw(voe_ui_context *ui,
				  voe_editor_textures_ask *ask, float top);

// After voe_ui_frame_end: a fired size sets `ask->largest` and is NONE; Import
// or Cancel fired, or a `pointer_down` at `at` outside the panel, which is a
// Cancel; NONE otherwise.
voe_editor_textures_ask_answer
voe_editor_textures_ask_read(const voe_ui_context *ui,
			     voe_editor_textures_ask *ask, bool pointer_down,
			     voe_math_float2 at);
