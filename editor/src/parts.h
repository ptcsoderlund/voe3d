// The project's parts files as game/materials.h's parts table (0420 point 1,
// the editor's side): every `.parts` under `Assets/` read and parsed, each row
// named by its model's `Assets/...` path, the `.parts` ending made `.glb`, and
// handed to game/models.h's loader beside the materials, as the game hands its
// cooked table.
//
//     voe_editor_parts_read(parts, folder, scratch);
//     voe_game_parts table = voe_editor_parts_table(parts);
//
// READ WHEN THE MATERIALS ARE: on a different folder and after each Assets
// command, by models.h, which gives every loaded model its row each update.
//
// A FILE THAT WILL NOT READ OR PARSE is left out with one stderr line, and its
// model draws with its own glTF materials (0420 point 1); one bad file does not
// cost the rest.
//
// Constraints: at most VOE_EDITOR_PARTS rows, the rest left out and said once;
// a larger constant lifts it. A model path of VOE_3D_MODEL_PATH bytes or more
// is left out, the room a model row gives it. Each row's path points into the
// struct's own bytes, so the struct is never copied; it is held by pointer.
#pragma once

#include <3d/model_component.h>

#include <base/arena.h>

#include <game/materials.h>

#include <stdint.h>

// How many models' parts a project's table holds.
#define VOE_EDITOR_PARTS 128

// The table's rows and the bytes of their paths; `count` rows in use.
typedef struct {
	voe_game_model_parts rows[VOE_EDITOR_PARTS];
	char paths[VOE_EDITOR_PARTS][VOE_3D_MODEL_PATH];
	uint32_t count;
} voe_editor_parts;

// Empties `parts`, then reads every `.parts` under `<folder>/Assets/` in byte
// order of path. NULL `folder` leaves it empty. `scratch` is rewound.
void voe_editor_parts_read(voe_editor_parts *parts, const char *folder,
			   voe_base_arena *scratch);

// The rows as the table game/models.h loads, pointing into `parts`.
voe_game_parts voe_editor_parts_table(const voe_editor_parts *parts);
