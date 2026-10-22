// The Inspector's Sculpt section, shown on a thing that wears a landscape
// (sculpt.h): a "Sculpt" heading as a component's is, a row of Raise, Lower,
// Smooth and Flatten with the chosen one drawn chosen, and Strength as a
// slider (0379 point 3). Radius and Softness are the Brush section's
// (inspector_brush.h, 0452).
//
//     voe_editor_inspector_sculpt_forget(&scene->sculpt);  // frame opened
//     voe_editor_inspector_sculpt_draw(ui, &scene->sculpt); // inspector.c
//     ...voe_ui_frame_end...
//     voe_editor_inspector_sculpt_read(ui, &scene->sculpt); // interface.c
//
// THE NODES LIVE IN THE SCULPT STATE, which outlives the call that drew them,
// for the reason every Inspector control does (inspector.h): a widget answers
// only after voe_ui_frame_end. So each frame forgets them first, and a frame
// that does not draw the section reads nothing.
//
// PRESSING THE CHOSEN BUTTON AGAIN CHOOSES NONE; another chooses its kind. While
// PAINT (inspector_layers.h), SCATTER or ERASE (inspector_foliage.h) is chosen
// no button is drawn chosen and any one chooses its kind. The slider's value
// is taken every frame, clamped into its range by `ui`; PAINT strokes with
// Strength too.
//
// Constraints: draw, forget and read are called in one frame's window, read
// before the frame's arena is rewound; nothing here allocates.
#pragma once

#include "sculpt.h"

#include <ui/layout.h>

// Forgets last frame's buttons and slider. Called when the frame opens.
void voe_editor_inspector_sculpt_forget(voe_editor_sculpt *sculpt);

// Puts the Sculpt section on the panel, recording its nodes in `sculpt`.
void voe_editor_inspector_sculpt_draw(voe_ui_context *ui,
				      voe_editor_sculpt *sculpt);

// After voe_ui_frame_end: a fired button chooses its kind, or none when it
// was the chosen one; a drawn slider's value taken.
void voe_editor_inspector_sculpt_read(const voe_ui_context *ui,
				      voe_editor_sculpt *sculpt);
