// The Inspector's LOD section, after Brush or Foliage, on a thing whose model
// row names a loaded model (0425 point 7): a "LOD" heading as Layers' is
// (0433), then per
// view in use one row, "View <n>: LOD <k> of <count>, made" or "..., own", k
// counted from one, the full model LOD 1 (0434), or "View <n>: LOD 1 of 1" for
// a model without levels. It shows the LOD each view keeps, as its draws
// chose it last: the `level` voe_3d_lods_kept_find (3d/lods_kept.h) gives for
// the thing's row key in the view's `lods`, so a copy creeping back inside the
// 10% shows the coarser LOD. Not drawn yet, or no memory, it falls back to the
// distance's LOD, voe_3d_model_level (3d/model_levels.h) with the view's own
// pass camera and eye and the thing's world place. A picture, landscape or
// material gets no section.
//
// THE SECTION FOLDS by its fold button, named `section lod` (folds.h, 0502);
// folded, only its header is drawn.
//
//     voe_editor_inspector_level_draw(ui, &inspector->level, folds, store,
//                                     world, selected, views);  // inspector.c
//
// THE LABELS LIVE IN THE STRUCT, one buffer per view: a label is read at
// voe_ui_frame_end and not copied (inspector.h), long after this call returned,
// so its text cannot be on this file's stack. Each draw overwrites them.
//
// Constraints: nothing here allocates; nothing is read back after the frame.
// At most VOE_EDITOR_INSPECTOR_LEVEL_NODES nodes (the section's four, its
// heading and a label per view) and VOE_EDITOR_INSPECTOR_LEVEL_ELEMENTS
// elements (the section's five, "LOD"'s three and each label at most its
// buffer less its zero), counted in interface.h.
#pragma once

#include "folds.h"
#include "view.h"

#include <3d/models.h>

#include <ecs/world.h>

#include <ui/layout.h>

// One view's label, its zero included.
#define VOE_EDITOR_INSPECTOR_LEVEL_LABEL 32

#define VOE_EDITOR_INSPECTOR_LEVEL_NODES (5 + VOE_EDITOR_VIEWS)
#define VOE_EDITOR_INSPECTOR_LEVEL_ELEMENTS \
	(5 + 3 + VOE_EDITOR_VIEWS * (VOE_EDITOR_INSPECTOR_LEVEL_LABEL - 1))

// Each view's label as last drawn.
typedef struct {
	char labels[VOE_EDITOR_VIEWS][VOE_EDITOR_INSPECTOR_LEVEL_LABEL];
} voe_editor_inspector_level;

// Puts the Level section for `entity` on the panel, open as `folds` says, its
// labels formatted into `level`; nothing unless its model row names a loaded
// entry that is no picture, landscape or material.
void voe_editor_inspector_level_draw(voe_ui_context *ui,
				     voe_editor_inspector_level *level,
				     voe_editor_folds *folds,
				     const voe_3d_models *store,
				     const voe_ecs_world *world,
				     voe_ecs_entity entity,
				     const voe_editor_views *views);
