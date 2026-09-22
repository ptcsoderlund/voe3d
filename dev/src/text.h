// The writing exhibit: the font, the sign in the world lettered on both faces,
// the heads-up line in the overlay on its dark panel, and the readout's entity.
// Placeholder scene data at a call site, the same kind of thing cubes.h and
// quad.h are.
//
// ITS OWN FILE BECAUSE IT IS ONE EXHIBIT. What the two placements are, what a
// text material has to say and what is wrong if the writing looks wrong stand
// above add_text in text.c. Where the heads-up line, its panel and the readout
// stand each frame is facing.h's, not this file's.
#pragma once

#include <base/arena.h>
#include <base/error.h>
#include <ecs/world.h>
#include <math/float2.h>
#include <render/device.h>
#include <text/font.h>

// The font, the sign's two faces, the heads-up line, its panel and the readout.
// `quad` is the geometry voe_dev_add_the_quads made, which the panel wears.
// `hud_size` is the heads-up line's size, which places it and its panel. The
// font is the return value because rule 6 allows one level of dereference;
// NULL on failure, with `error` saying why.
[[nodiscard]] voe_text_font *voe_dev_add_the_text(voe_ecs_world *world,
						  voe_render_device *gpu,
						  voe_base_arena *arena,
						  voe_render_geometry quad,
						  voe_ecs_entity *hud,
						  voe_ecs_entity *panel,
						  voe_ecs_entity *readout,
						  voe_math_float2 *hud_size,
						  voe_base_error *error);
