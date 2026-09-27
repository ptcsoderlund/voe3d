// The editor's one model store (3d/models.h, ADR-0277): every model file the
// project's things name, uploaded once, and handed to the pick read and to both
// view passes so models are drawn, picked and outlined. main.c makes it after
// the shapes are uploaded and destroys it before the device:
//
//     models = voe_editor_models_new();
//     voe_editor_pick_read(..., voe_editor_models_store(models), ...);
//     voe_editor_models_destroy(models, gpu);
//
// ONE FOR THE PROGRAM, NOT ONE PER PROJECT. What the store holds lives on the
// device, and the device outlives every project the editor opens: New and Open
// swap the project and keep the device. So the store is made once beside the
// device and emptied when a different project is in place, which is a later
// card's; a store per project would have to be torn down and made again at
// every swap for nothing.
//
// Nothing fills it here: the store starts empty and every model row draws as
// nothing until a loader reads files into it.
#pragma once

#include <3d/models.h>

#include <render/device.h>

typedef struct voe_editor_models voe_editor_models;

// An empty store. Never NULL: running out of memory is an assert.
voe_editor_models *voe_editor_models_new(void);

// Frees every entry's parts through `device`, then the store. NULL is nothing.
// Between frames only, before the device is destroyed.
void voe_editor_models_destroy(voe_editor_models *models,
			       voe_render_device *device);

// The store to read, for what draws, picks and outlines.
const voe_3d_models *voe_editor_models_store(const voe_editor_models *models);
