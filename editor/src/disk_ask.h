// The question a file changed on disk asks while the editor holds unsaved
// changes to it (0506 point 1): a line naming the project-relative file, then
// Load from disk and Keep mine, as an anchored panel over the dock below the
// bar, drawn and read the way textures_ask.h's question is.
//
//     voe_editor_disk_ask_open(&ask, "Assets/a.scene");
//     voe_editor_disk_ask_draw(ui, &ask, top);
//     ... voe_ui_frame_end ...
//     answer = voe_editor_disk_ask_read(ui, &ask, pointer_down, at);
//     if (answer != VOE_EDITOR_DISK_ASK_NONE)
//             ... hand the answer on, then voe_editor_disk_ask_close(&ask) ...
//
// ONLY ITS TWO BUTTONS ANSWER IT. Escape does not, and neither does a press
// outside it: Keep mine means the next Save writes over what changed on disk,
// so nothing short of a choice may stand for one. Nothing here takes a file
// in or keeps it: the caller carries out the answer (disk_take.h).
//
// Constraints. The path is cut at VOE_EDITOR_DISK_ASK_PATH bytes, the room
// disk_take.h asks a path in, which is what interface.h's element budget
// counts; a cut may split a UTF-8 character at the very end.
#pragma once

#include <math/float2.h>

#include <ui/widgets.h>

#include <stdbool.h>

// The longest path named, its NUL included: VOE_EDITOR_DISK_TAKE_PATH.
#define VOE_EDITOR_DISK_ASK_PATH 1024

// What follows the path on the question's line.
#define VOE_EDITOR_DISK_ASK_SAID \
	" changed on disk while you have unsaved changes to it."

// Zeroed is a question never asked.
typedef struct {
	bool open;
	// Kept here because a label's text is drawn after the call that made it.
	char line[VOE_EDITOR_DISK_ASK_PATH + sizeof VOE_EDITOR_DISK_ASK_SAID];
	voe_ui_node panel;
	voe_ui_node load_button;
	voe_ui_node keep_button;
} voe_editor_disk_ask;

typedef enum {
	VOE_EDITOR_DISK_ASK_NONE = 0,
	VOE_EDITOR_DISK_ASK_LOAD,
	VOE_EDITOR_DISK_ASK_KEEP,
} voe_editor_disk_ask_answer;

// Opens the question about `path`, relative to the project's folder.
void voe_editor_disk_ask_open(voe_editor_disk_ask *ask, const char *path);
void voe_editor_disk_ask_close(voe_editor_disk_ask *ask);

// Draws the question centred across the surface, `top` below its top, over the
// dock, and records its nodes. Asserts when it is not open.
void voe_editor_disk_ask_draw(voe_ui_context *ui, voe_editor_disk_ask *ask,
			      float top);

// After voe_ui_frame_end: Load from disk or Keep mine fired; NONE otherwise,
// a `pointer_down` at `at` outside the panel included.
voe_editor_disk_ask_answer voe_editor_disk_ask_read(const voe_ui_context *ui,
						    const voe_editor_disk_ask *ask,
						    bool pointer_down,
						    voe_math_float2 at);
