// The material a shape's `material` path names in the frame's model store
// (0399 point 6), for the record a drawn shape is built with. Internal to this
// folder: draw_group.c asks it once per drawn shaped entity per pass.
//
//     const voe_3d_material *worn =
//             voe_3d_draw_material_named(frame.models, shape->material);
//     // NULL: draw the shape's own material and colour
//
// Constraints: a linear find over the store's entries per call, which is the
// world pass's CPU (0388); a store indexed by path would lift it.
#pragma once

#include <3d/material_component.h>
#include <3d/models.h>

// The loaded material entry's part material for a non-empty `path`; NULL for no
// store, an empty path, a path with no end within VOE_3D_MODEL_PATH, or one the
// store holds no loaded material for.
const voe_3d_material *voe_3d_draw_material_named(const voe_3d_models *models,
						  const char *path);
