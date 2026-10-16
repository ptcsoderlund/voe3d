// The Inspector's LOD section, after Layers, on a thing whose model row names
// a loaded model (0425 point 7): a "LOD" heading as Layers' is (0433), then per
// view in use one row, "View <n>: LOD <k> of <count>, made" or "..., own", k
// from 0, the full model, or "View <n>: LOD 0 of 1" for a model without
// levels. It shows what is drawn, so it
// asks voe_3d_model_level (3d/model_levels.h), the very function the draws
// choose with, with the view's own pass camera and eye and the thing's world
// place; a picture, landscape or material gets no section.
//
//     voe_editor_inspector_level_draw(ui, &inspector->level, store, world,
//                                     selected, views);         // inspector.c
//
// THE LABELS LIVE IN THE STRUCT, one buffer per view: a label is read at
// voe_ui_frame_end and not copied (inspector.h), long after this call returned,
// so its text cannot be on this file's stack. Each draw overwrites them.
//
// Constraints: nothing here allocates; nothing is read back after the frame.
// At most VOE_EDITOR_INSPECTOR_LEVEL_NODES nodes (the panel, its heading and a
// label per view) and VOE_EDITOR_INSPECTOR_LEVEL_ELEMENTS elements (the panel's
// border and fill, "LOD"'s three and each label at most its buffer less its
// zero), counted in interface.h.
#pragma once

#include "view.h"

#include <3d/models.h>

#include <ecs/world.h>

#include <ui/layout.h>

// One view's label, its zero included.
#define VOE_EDITOR_INSPECTOR_LEVEL_LABEL 32

#define VOE_EDITOR_INSPECTOR_LEVEL_NODES (2 + VOE_EDITOR_VIEWS)
#define VOE_EDITOR_INSPECTOR_LEVEL_ELEMENTS \
	(2 + 3 + VOE_EDITOR_VIEWS * (VOE_EDITOR_INSPECTOR_LEVEL_LABEL - 1))

// Each view's label as last drawn.
typedef struct {
	char labels[VOE_EDITOR_VIEWS][VOE_EDITOR_INSPECTOR_LEVEL_LABEL];
} voe_editor_inspector_level;

// Puts the Level section for `entity` on the panel, its labels formatted into
// `level`; nothing unless its model row names a loaded entry that is no
// picture, landscape or material.
void voe_editor_inspector_level_draw(voe_ui_context *ui,
				     voe_editor_inspector_level *level,
				     const voe_3d_models *store,
				     const voe_ecs_world *world,
				     voe_ecs_entity entity,
				     const voe_editor_views *views);
