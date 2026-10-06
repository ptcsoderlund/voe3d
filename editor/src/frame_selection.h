// F: the scene view under the pointer glides to the selection (0371 points 2
// and 3). frame_pointer.c calls voe_editor_frame_selection on the frame the
// shortcut flag is set (shortcuts.h), with the same views, scene, geometries,
// models and pointer the pick gets.
//
// THE FRAMING IS THE EDITOR'S AND THE SIZE IS `3d`'S. How big a tree of shapes
// and models is in the world needs the shape table, the model store and the
// transforms, all of which `3d` already holds for the pick (3d/bounds.h); what
// the editor adds is which view moves and where it stops.
//
// ONLY THE VIEW UNDER THE POINTER MOVES. A person points at the picture they
// want changed, as with the middle drag and the fly; the other views keep what
// they were looking at, and a pointer over no view frames nothing.
//
// A THING WITH NO SIZE STOPS AT A FIXED DISTANCE WITH ITS MARKER IN THE MIDDLE.
// A light, a camera or an empty entity has no triangles to frame, yet it has a
// place and a marker drawn there; VOE_EDITOR_FRAME_NEAR is close enough to see
// the marker and far enough that the orbit does not start inside it. An entity
// with neither a size nor a transform is nowhere, and nothing moves.
//
// NOTHING IS SAVED OR PUT ON UNDO: the view's pose is the editor's and never
// the scene's (view.h), so framing marks nothing unsaved.
//
// Constraints: allocates nothing; walks every shape and model once per call
// (3d/bounds.h), which is a key press and not a per-frame cost.
#pragma once

#include "scene.h"
#include "view.h"

#include <3d/models.h>
#include <3d/shape_geometry.h>

#include <math/float2.h>

// How far the eye stops from a selection with no size, in metres (0371 point 2).
#define VOE_EDITOR_FRAME_NEAR 3.0f

// Glides the view under `pointer` (the root surface's millimetres) to the live
// selection: its size's centre at the distance that frames it, or, sizeless,
// its world position at VOE_EDITOR_FRAME_NEAR. Nothing when the selection is
// not alive, the pointer is over no view, or it has neither size nor transform.
// `models` may be NULL.
void voe_editor_frame_selection(voe_editor_views *views,
				const voe_editor_scene *scene,
				const voe_3d_shape_geometries *geometries,
				const voe_3d_models *models,
				voe_math_float2 pointer);
