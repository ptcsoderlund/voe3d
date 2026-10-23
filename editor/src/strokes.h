// One sculpting, paint or foliage stroke as an undo step carries it (0379
// point 5, 0402 point 7, 0442 point 4): which landscape, the rectangle of
// points it touched, and that rectangle's heights, or for a paint stroke its
// weights, before and after, so stepping over it writes one or the other back
// into the editor's model store.
//
//     stroke = voe_editor_stroke_new(path, rect, before, after);
//     paint = voe_editor_stroke_new_paint(path, rect, bytes, bytes2);
//     trees = voe_editor_stroke_new_foliage(path, false, added, count);
//     looked = voe_editor_stroke_new_look(path, kind, before, after);
//     voe_editor_undo_stroke(&undo, project, scratch, stroke); // undo owns it
//     voe_editor_stroke_apply(stroke, models, false);          // back
//     voe_editor_stroke_destroy(stroke);
//
// A FOLIAGE STROKE carries the copies it added or removed instead, put back by
// value: a copy has no index that survives an erase moving the last one in.
//
// A KIND'S LOOK rides as a stroke too, its kind and look before and after
// (0476 points 1 and 7): strokes already carry a landscape's edits beside the text and
// write back through the store, and a landscape is saved with the project, not
// written at once as a material step's file is.
//
// A STROKE RIDES BESIDE THE SCENE TEXT, NOT IN IT. A state of the undo line is
// a whole-scene text of at most 64 KB (undo.h); a stroke over a 512-cell grid
// is 513 × 513 heights twice, some 2 MB, and its weights 8 bytes a point twice,
// some 4 MB, which no text state holds and which a scene text would only say
// as its unchanged model path. So a state points at a stroke of its own.
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

#include <assets/landscape.h>

#include <stdbool.h>
#include <stdint.h>

// A landscape's path, the rectangle touched, and as they were and as the
// stroke left them either its heights row-major, or when `paint` its weights
// as voe_editor_models_put_paint takes them; all the stroke's own, the other
// pair NULL. When `foliage`, no rectangle and no values: `copy_count`
// `copies`, its own, that the stroke removed when `erase` and added if not.
// When `look`, none of those: kind `kind`'s look `look_before` and
// `look_after`.
typedef struct {
	char path[VOE_3D_MODEL_PATH];
	voe_3d_landscape_rect rect;
	bool paint;
	float *before;
	float *after;
	uint8_t *paint_before;
	uint8_t *paint_after;
	bool foliage;
	bool erase;
	voe_assets_landscape_copy *copies;
	uint32_t copy_count;
	bool look;
	uint32_t kind;
	voe_assets_landscape_look look_before;
	voe_assets_landscape_look look_after;
} voe_editor_stroke;

// A stroke holding copies of `path` and of `rect`'s heights in `before` and
// `after`. Never NULL; the caller destroys it or hands it to the undo line.
voe_editor_stroke *voe_editor_stroke_new(const char *path,
					 voe_3d_landscape_rect rect,
					 const float *before, const float *after);

// A paint stroke holding copies of `path` and of `rect`'s weights, 8 bytes a
// point, plane 0's rect then plane 1's, in `before` and `after`. Never NULL.
voe_editor_stroke *voe_editor_stroke_new_paint(const char *path,
					       voe_3d_landscape_rect rect,
					       const uint8_t *before,
					       const uint8_t *after);

// A foliage stroke holding copies of `path` and of the `count` `copies` a
// scatter added, or an erase removed when `erase`. Never NULL.
voe_editor_stroke *
voe_editor_stroke_new_foliage(const char *path, bool erase,
			      const voe_assets_landscape_copy *copies,
			      uint32_t count);

// A look stroke holding a copy of `path` and kind `kind`'s look `before` and
// `after` the change. Never NULL.
voe_editor_stroke *voe_editor_stroke_new_look(const char *path, uint32_t kind,
					      voe_assets_landscape_look before,
					      voe_assets_landscape_look after);

// Frees the stroke and its heights, weights or copies. NULL is nothing.
void voe_editor_stroke_destroy(voe_editor_stroke *stroke);

// Writes the after values into the store's landscape when `forward`, the
// before otherwise; a foliage stroke's copies are added forward and removed
// backward for a scatter, the other way for an erase; a look stroke's look
// is set. Nothing when the path is no loaded landscape.
void voe_editor_stroke_apply(const voe_editor_stroke *stroke,
			     voe_editor_models *models, bool forward);
