// The Inspector's Brush section: what every brush shares, Radius (m) and
// Softness as sliders with sculpt.h's ranges, under a "Brush" heading as the
// Sculpt section's is (0452 point 1). One radius and one softness serve every
// brush, and the ring the view draws is the same one.
//
//     voe_editor_inspector_brush_forget(&scene->sculpt);  // frame opened
//     voe_editor_inspector_brush_draw(ui, &scene->sculpt); // inspector.c
//     ...voe_ui_frame_end...
//     voe_editor_inspector_brush_read(ui, &scene->sculpt); // interface_read.c
//
// SHOWN ONLY WHILE A BRUSH IS CHOSEN (0452 point 2): a sculpt kind, PAINT,
// SCATTER or ERASE, on a thing that wears a landscape. AFTER FOLIAGE AND
// BEFORE LOD (point 3), so choosing or unchoosing a brush moves no button
// above it and the one just pressed stays under the pointer.
//
// THE NODES LIVE IN THE SCULPT STATE (sculpt.h), for the reason the Sculpt
// section's do (inspector_sculpt.h): a widget answers only after
// voe_ui_frame_end. Each frame forgets them first, and the read comes after
// Foliage's (point 4).
//
// THE SECTION FOLDS by its fold button, named `section brush` (folds.h, 0502);
// folded, only its header is drawn and no slider is recorded.
//
// Constraints: draw, forget and read are called in one frame's window, read
// before the frame's arena is rewound; nothing here allocates.
#pragma once

#include "folds.h"
#include "sculpt.h"

#include <ui/layout.h>

// Forgets last frame's sliders. Called when the frame opens.
void voe_editor_inspector_brush_forget(voe_editor_sculpt *sculpt);

// Puts the Brush section on the panel, open as `folds` says, recording its
// nodes in `sculpt`; nothing while no brush is chosen.
void voe_editor_inspector_brush_draw(voe_ui_context *ui,
				     voe_editor_sculpt *sculpt,
				     voe_editor_folds *folds);

// After voe_ui_frame_end: each drawn slider's value taken into `radius` or
// `softness`, clamped into its range by `ui`.
void voe_editor_inspector_brush_read(const voe_ui_context *ui,
				     voe_editor_sculpt *sculpt);
