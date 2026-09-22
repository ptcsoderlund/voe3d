// The model exhibit: a figure a real exporter wrote, embedded at build time,
// and the call that reads a `.glb` into the world and moves it aside.
//
// ITS OWN FILE BECAUSE IT IS ONE EXHIBIT. What the figure is for and what is
// wrong if it looks wrong stand above voe_dev_add_a_model in model.c. Where it
// is moved to is main.c's, which is why the offset is a parameter.
#pragma once

#include <base/arena.h>
#include <base/error.h>
#include <ecs/world.h>
#include <render/device.h>

#include <stddef.h>
#include <stdint.h>

// `textured_primitives_human.glb`, embedded because `platform` has no file API.
extern const uint8_t voe_dev_human_glb[];
extern const size_t voe_dev_human_glb_size;

// The `size` bytes of a `.glb` into the world, every entity moved `offset_x`
// metres along X, and one line on stdout naming `name` and what it made.
// `arena` is rewound before this returns, on every path.
[[nodiscard]] bool voe_dev_add_a_model(voe_ecs_world *world,
				       voe_render_device *gpu,
				       voe_base_arena *arena, const char *name,
				       const uint8_t *bytes, size_t size,
				       float offset_x, voe_base_error *error);
