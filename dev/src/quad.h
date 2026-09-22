// The quads exhibit: the see-through quad's geometry by hand, the five quads
// standing in the world and the overlay, and the calls that build a quad or a
// panel. Placeholder scene data at a call site, the same kind of thing cubes.h
// is and there for the same reason: it is data, it decides nothing, and it is
// handed to `render` exactly as an importer would hand over a mesh it read from
// a file.
//
// ITS OWN FILE BECAUSE IT IS ONE EXHIBIT. The "two in the world / three in the
// overlay" paragraphs stand above voe_dev_add_the_quads in quad.c. text.c
// builds the heads-up panel with voe_dev_add_quad and main.c the two element
// panels with voe_dev_add_panel, which is why those are here and not private.
//
// A FLAT SQUARE, ONE METRE ACROSS, IN THE XY PLANE. Its normal is +Z, so it is
// lit like any other surface and a person can see the sun cross it.
//
// IT IS EIGHT VERTICES AND NOT FOUR, BECAUSE THE ENGINE CULLS BACK FACES. A
// single-sided square disappears for half of the camera's lap, which would make
// "is the sort right from every angle" impossible to look at — so this is two
// squares in the same place, wound opposite ways, with opposite normals. Exactly
// one of them survives culling from any given side, so nothing is drawn twice
// and nothing blends over itself.
//
// THAT IS A DOUBLE-SIDED *MESH* AND NOT A DOUBLE-SIDED MATERIAL. The engine
// culls back faces whatever a material says and `doubleSided` is not carried
// (assets/include/assets/model.h); geometry is a different matter and it is the
// call site's own. A file wanting a two-sided leaf is a different card.
#pragma once

#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <base/error.h>
#include <ecs/world.h>
#include <math/float2.h>
#include <math/float4.h>
#include <render/device.h>
#include <scene/transform_component.h>

#include <stdint.h>

#define VOE_DEV_QUAD_VERTEX_COUNT 8
#define VOE_DEV_QUAD_INDEX_COUNT 12

extern const voe_render_vertex voe_dev_quad_vertices[VOE_DEV_QUAD_VERTEX_COUNT];
extern const uint32_t voe_dev_quad_indices[VOE_DEV_QUAD_INDEX_COUNT];

// A fully rough, non-metallic material of this colour, alpha mode and
// lighting. The colour is not premultiplied.
voe_3d_material voe_dev_quad_material(voe_math_float4 colour,
				      voe_render_alpha_mode alpha_mode,
				      bool unlit);

// An upright square quad at `position`, `size` metres across.
voe_scene_transform voe_dev_quad_at(voe_math_float3 position, float size);

// One element panel entity: a transform and a panel component `millimetres`
// big in `layer`. Its range is written every frame by main.c.
[[nodiscard]] bool voe_dev_add_panel(voe_ecs_world *world,
				     voe_math_float3 position, float scale,
				     voe_math_float2 millimetres,
				     voe_3d_layer layer, voe_ecs_entity *out);

// One quad entity wearing `geometry` and `material`, which this uploads.
// `out` may be NULL for a quad nobody moves again.
[[nodiscard]] bool voe_dev_add_quad(voe_ecs_world *world,
				    voe_render_device *gpu,
				    voe_render_geometry geometry,
				    voe_3d_material material,
				    voe_scene_transform transform,
				    voe_3d_layer layer, voe_ecs_entity *out,
				    voe_base_error *error);

// The square into the pools and the five quads. `quad` gets the geometry, which
// every quad in the program wears, the heads-up panel included.
[[nodiscard]] bool voe_dev_add_the_quads(voe_ecs_world *world,
					 voe_render_device *gpu,
					 voe_render_geometry *quad,
					 voe_base_error *error);
