// One sculpting stroke as an undo step carries it (0379 point 5): which
// landscape, the height rectangle it touched, and that rectangle's heights
// before and after, so stepping over it writes one or the other back into the
// editor's model store.
//
//     stroke = voe_editor_stroke_new(path, rect, before, after);
//     voe_editor_undo_stroke(&undo, project, scratch, stroke); // undo owns it
//     voe_editor_stroke_apply(stroke, models, false);          // back
//     voe_editor_stroke_destroy(stroke);
//
// A STROKE RIDES BESIDE THE SCENE TEXT, NOT IN IT. A state of the undo line is
// a whole-scene text of at most 64 KB (undo.h); a stroke over a 512-cell grid
// is 513 × 513 heights twice, some 2 MB, which no text state holds and which a
// scene text would only say as its unchanged model path. So a state points at
// a stroke of its own instead.
//
// ITS OWN MEMORY, MALLOCED. This is rule 11's long-lived exception: a line of
// 64 states may hold megabytes each or nothing at all, so pushing room for the
// worst once out of an arena would be over a hundred megabytes held for every
// editor that never sculpts. Each stroke is allocated when recorded and freed
// when its state is dropped, which the undo line does.
//
// Constraints: running out of memory is an assert. `path` must fit in
// VOE_3D_MODEL_PATH with its terminator, as a model row's path does.
#pragma once

#include "models.h"

#include <3d/landscape.h>
#include <3d/model_component.h>

#include <stdbool.h>

// A landscape's path, the height rectangle touched, and its heights row-major
// as they were and as the stroke left them, both the stroke's own.
typedef struct {
	char path[VOE_3D_MODEL_PATH];
	voe_3d_landscape_rect rect;
	float *before;
	float *after;
} voe_editor_stroke;

// A stroke holding copies of `path` and of `rect`'s heights in `before` and
// `after`. Never NULL; the caller destroys it or hands it to the undo line.
voe_editor_stroke *voe_editor_stroke_new(const char *path,
					 voe_3d_landscape_rect rect,
					 const float *before, const float *after);

// Frees the stroke and its heights. NULL is nothing.
void voe_editor_stroke_destroy(voe_editor_stroke *stroke);

// Writes `after` into the store's landscape when `forward`, `before`
// otherwise; nothing when the path is no loaded landscape.
void voe_editor_stroke_apply(const voe_editor_stroke *stroke,
			     voe_editor_models *models, bool forward);
