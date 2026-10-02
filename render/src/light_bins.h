// Which screen tiles and depth slices each point light of a pass reaches
// (ADR-0320 point 5). Pure CPU, no Vulkan: opening a pass fills one record from
// its view and its lights, and the shader reads it.
//
//     struct voe_render_light_bins bins;
//     voe_render_light_bins_fill(&camera->view, lights.lights, lights.count,
//                                &bins);
//     uint32_t slice = voe_render_light_slice(distance);
//
// TWO SEPARABLE MASKS, NOT CELLS. One bit per light in each of 16 × 9 tiles and
// in each of 32 slices is 5.6 kB a pass; a mask per tile-and-slice cell would be
// 147 kB and 4608 cells to fill per light. A fragment ANDs its tile's mask with its slice's
// and loops over the bits left (Drobot's zbin): a light marked in both may still
// miss it, which the falloff's range then answers.
//
// LIGHT i IS BIT i % 32 OF WORD i / 32, in both masks. A tile is x + 16 × y,
// with x and y from NDC as (ndc + 1) / 2 × tiles; a slice is exponential in view
// distance from VOE_RENDER_LIGHT_SLICE_NEAR to _FAR, the last open-ended.
//
// THE PROJECTION'S OWN NDC IS USED, Y AS IT COMES. Nothing here flips or
// remaps: the shader recomputes the same NDC from the same view and projection,
// so writer and reader agree whatever convention the matrices hold.
//
// CONSERVATIVE. A tile or slice a light's sphere touches is always marked; extra
// marks are allowed. The tiles are the NDC rectangle of the eight corners of the
// sphere's view-space box, every tile when that box reaches behind the near
// distance, none when it is off the screen or wholly behind the eye.
//
// CONSTRAINTS. Cost is count × the tiles each light covers, a light around the
// eye all 144 of them: at 256 lights a few tens of thousands of bit sets.
#pragma once

#include <render/device.h>

#include <stdint.h>

#define VOE_RENDER_LIGHT_TILES_X 16
#define VOE_RENDER_LIGHT_TILES_Y 9
#define VOE_RENDER_LIGHT_SLICES 32
// Words of one light mask: one bit per light a pass may carry.
#define VOE_RENDER_LIGHT_WORDS (VOE_RENDER_POINT_LIGHTS / 32)
// View distances, metres, the slices run between, exponentially.
#define VOE_RENDER_LIGHT_SLICE_NEAR 0.1f
#define VOE_RENDER_LIGHT_SLICE_FAR 1000.0f

// One pass's masks. Only voe_render_light_bins_fill writes one.
struct voe_render_light_bins {
	uint32_t tiles[VOE_RENDER_LIGHT_TILES_X * VOE_RENDER_LIGHT_TILES_Y]
		      [VOE_RENDER_LIGHT_WORDS];
	uint32_t slices[VOE_RENDER_LIGHT_SLICES][VOE_RENDER_LIGHT_WORDS];
};

// The slice of view distance `distance`: 0 below NEAR, the last at FAR and past.
uint32_t voe_render_light_slice(float distance);

// Zeroes `out`, then marks each light's tiles and slices through `view`, whose
// view matrix looks down −Z. Asserts count ≤ VOE_RENDER_POINT_LIGHTS.
void voe_render_light_bins_fill(const voe_render_view *view,
				const voe_render_point_light *lights,
				uint32_t count, struct voe_render_light_bins *out);
