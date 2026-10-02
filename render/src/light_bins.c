// The light binning's two calls: a view distance to its slice, and a pass's
// lights to their tile and slice masks (see light_bins.h for the layout and why).
//
// A light's sphere is taken into view space by the view matrix, an affine one;
// its slices run from the slice of its near side to that of its far side, and
// its tiles from the corners of its box through the projection, divided by w.
// Nothing is allocated; the record is the caller's.
//
// CONSTRAINTS. The tile rectangle bounds the box and not the sphere, so a light
// seen off-centre marks a few tiles more than it reaches; a tighter sphere
// bound would lift that if a profile ever names fragment loops.
#include "light_bins.h"

#include <assert.h>
#include <math.h>
#include <string.h>

#define TILES_X ((uint32_t)VOE_RENDER_LIGHT_TILES_X)
#define TILES_Y ((uint32_t)VOE_RENDER_LIGHT_TILES_Y)
#define SLICES ((uint32_t)VOE_RENDER_LIGHT_SLICES)
#define NEAR VOE_RENDER_LIGHT_SLICE_NEAR
#define FAR VOE_RENDER_LIGHT_SLICE_FAR

static_assert(VOE_RENDER_POINT_LIGHTS % 32 == 0, "whole words of lights");

uint32_t voe_render_light_slice(float distance)
{
	assert(!isnan(distance));
	uint32_t slice = SLICES - 1;

	if (!(distance > NEAR))
		slice = 0;
	else if (distance < FAR) {
		const float s = logf(distance / NEAR) / logf(FAR / NEAR) *
				(float)SLICES;

		slice = s >= (float)(SLICES - 1) ? SLICES - 1 : (uint32_t)s;
	}
	assert(slice < SLICES);
	return slice;
}

static void mark(uint32_t mask[VOE_RENDER_LIGHT_WORDS], uint32_t light)
{
	assert(light < VOE_RENDER_POINT_LIGHTS);
	mask[light / 32] |= 1u << (light % 32);
}

static void mark_every_tile(struct voe_render_light_bins *out, uint32_t light)
{
	for (uint32_t t = 0; t < TILES_X * TILES_Y; t++)
		mark(out->tiles[t], light);
}

// The tiles [first, last] the NDC span [lo, hi] covers on an axis of `tiles`;
// false when it is off the screen.
static bool tile_span(float lo, float hi, uint32_t tiles, uint32_t *first,
		      uint32_t *last)
{
	if (!(hi >= -1.0f) || !(lo <= 1.0f))
		return false;
	const float a = (lo + 1.0f) * 0.5f * (float)tiles;
	const float b = (hi + 1.0f) * 0.5f * (float)tiles;

	*first = a <= 0.0f ? 0 : a >= (float)(tiles - 1) ? tiles - 1 : (uint32_t)a;
	*last = b >= (float)(tiles - 1) ? tiles - 1 : (uint32_t)b;
	assert(*first <= *last && *last < tiles);
	return true;
}

// Marks the tiles of the box about view-space centre `c`, half extent `r`, that
// lies wholly beyond the near distance.
static void bin_tiles(const voe_render_view *view, voe_math_float3 c, float r,
		      uint32_t light, struct voe_render_light_bins *out)
{
	float x0 = INFINITY, x1 = -INFINITY, y0 = INFINITY, y1 = -INFINITY;
	uint32_t tx0, tx1, ty0, ty1;

	for (uint32_t k = 0; k < 8; k++) {
		const voe_math_float4 corner = {
			c.x + ((k & 1u) ? r : -r), c.y + ((k & 2u) ? r : -r),
			c.z + ((k & 4u) ? r : -r), 1.0f,
		};
		const voe_math_float4 clip =
			voe_math_float4x4_mul_float4(view->projection, corner);

		if (!(clip.w > 0.0f)) {
			mark_every_tile(out, light);
			return;
		}
		x0 = fminf(x0, clip.x / clip.w);
		x1 = fmaxf(x1, clip.x / clip.w);
		y0 = fminf(y0, clip.y / clip.w);
		y1 = fmaxf(y1, clip.y / clip.w);
	}
	if (!tile_span(x0, x1, TILES_X, &tx0, &tx1) ||
	    !tile_span(y0, y1, TILES_Y, &ty0, &ty1))
		return;
	for (uint32_t y = ty0; y <= ty1; y++)
		for (uint32_t x = tx0; x <= tx1; x++)
			mark(out->tiles[x + TILES_X * y], light);
	assert(tx1 < TILES_X && ty1 < TILES_Y);
}

static void bin_light(const voe_render_view *view,
		      const voe_render_point_light *light, uint32_t i,
		      struct voe_render_light_bins *out)
{
	const float r = light->range;
	assert(r > 0.0f && isfinite(r));
	const voe_math_float3 c =
		voe_math_float4x4_transform_point(view->view, light->position);
	const float near = -(c.z + r);
	const float far = -(c.z - r);

	if (!(far > 0.0f))
		return;
	const uint32_t last = voe_render_light_slice(far);

	for (uint32_t s = voe_render_light_slice(near); s <= last; s++)
		mark(out->slices[s], i);
	if (near < NEAR)
		mark_every_tile(out, i);
	else
		bin_tiles(view, c, r, i, out);
	assert(last < SLICES);
}

void voe_render_light_bins_fill(const voe_render_view *view,
				const voe_render_point_light *lights,
				uint32_t count, struct voe_render_light_bins *out)
{
	assert(view != NULL && out != NULL);
	assert(count <= VOE_RENDER_POINT_LIGHTS);
	assert(lights != NULL || count == 0);

	memset(out, 0, sizeof(*out));
	for (uint32_t i = 0; i < count; i++)
		bin_light(view, &lights[i], i, out);
}
