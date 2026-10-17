// The foliage's groups (0442 point 5). Internal to 3d: draw_foliage.c defines
// voe_3d_draw_system_foliage, declared in 3d/draw_system.h, and finds each
// copy's group by the key below.
//
//     struct voe_3d_foliage_key key = { .kind = entry, .level = 1,
//                                       .part = 0, .rest = false };
//
// A GROUP IS ONE (kind, level, part, rest): the kind's loaded `.glb` entry, so
// two landscape rows naming one file share its groups; the level its copies are
// drawn at; the level-0 part they wear; and whether they are a band's
// level-before copies, dithered the other way (0432 point 3). Equal keys are one
// group, whichever row and copy filled it.
#pragma once

#include <3d/models.h>

#include <stdbool.h>
#include <stdint.h>

struct voe_3d_foliage_key {
	const voe_3d_model_entry *kind;
	uint32_t level;
	uint32_t part;
	bool rest;
};
