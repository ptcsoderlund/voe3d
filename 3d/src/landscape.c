// The landscape arithmetic of 3d/landscape.h: the bilinear height, the ray
// marched and bisected through the box, and a brush's stamp over the rect its
// radius covers.
//
// Used by the model store (the brush), the pick (the ray) and the editor (the
// brush). Heights are row-major, row r (z) starting at r·(cells + 1).
//
// Constraints: the ray takes at most box length / half a cell steps and 24
// bisections; the brush walks every height in its square once, smooth reading
// a copy one height wider on `scratch`. Nothing here allocates but on the
// arenas handed in.
#include <3d/landscape.h>

#include <base/assert.h>

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BISECTIONS 24
#define LONGEST_SECONDS 0.1f

static float cell_size(const voe_assets_landscape *landscape)
{
	return landscape->size / (float)landscape->cells;
}

// A coordinate in metres into a fractional height index, clamped to the grid.
static float grid_index(const voe_assets_landscape *landscape, float metres)
{
	const float index =
		(metres + 0.5f * landscape->size) / cell_size(landscape);

	return fminf(fmaxf(index, 0.0f), (float)landscape->cells);
}

static float at(const voe_assets_landscape *landscape, uint32_t c, uint32_t r)
{
	return landscape->heights[(size_t)r * (landscape->cells + 1) + c];
}

float voe_3d_landscape_height(const voe_assets_landscape *landscape, float x,
			      float z)
{
	VOE_BASE_ASSERT(landscape && landscape->heights, "a landscape");
	VOE_BASE_ASSERT(landscape->cells >= 1, "at least one cell");
	const float fx = grid_index(landscape, x);
	const float fz = grid_index(landscape, z);
	const uint32_t c = (uint32_t)fminf(floorf(fx), landscape->cells - 1);
	const uint32_t r = (uint32_t)fminf(floorf(fz), landscape->cells - 1);
	const float tx = fx - (float)c;
	const float tz = fz - (float)r;
	const float near = at(landscape, c, r) +
			   (at(landscape, c + 1, r) - at(landscape, c, r)) * tx;
	const float far =
		at(landscape, c, r + 1) +
		(at(landscape, c + 1, r + 1) - at(landscape, c, r + 1)) * tx;

	return near + (far - near) * tz;
}

void voe_3d_landscape_box(const voe_assets_landscape *landscape,
			  voe_math_float3 *min, voe_math_float3 *max)
{
	VOE_BASE_ASSERT(landscape && landscape->heights, "a landscape");
	VOE_BASE_ASSERT(min && max, "somewhere to put the box");
	const size_t count =
		(size_t)(landscape->cells + 1) * (landscape->cells + 1);
	float low = landscape->heights[0];
	float high = landscape->heights[0];

	for (size_t i = 1; i < count; i++) {
		low = fminf(low, landscape->heights[i]);
		high = fmaxf(high, landscape->heights[i]);
	}
	*min = (voe_math_float3){ -0.5f * landscape->size, low,
				  -0.5f * landscape->size };
	*max = (voe_math_float3){ 0.5f * landscape->size, high,
				  0.5f * landscape->size };
}

// One axis of the slab test, narrowing [*enter, *leave].
static bool slab(float origin, float direction, float low, float high,
		 float *enter, float *leave)
{
	if (fabsf(direction) < 1e-12f)
		return origin >= low && origin <= high;
	float a = (low - origin) / direction;
	float b = (high - origin) / direction;

	if (a > b) {
		const float swap = a;
		a = b;
		b = swap;
	}
	*enter = fmaxf(*enter, a);
	*leave = fminf(*leave, b);
	return *enter <= *leave;
}

// How far above the ground the ray is at t; at or below 0 is in it.
static float above(const voe_assets_landscape *landscape, voe_math_float3 o,
		   voe_math_float3 d, float t)
{
	return o.y + d.y * t -
	       voe_3d_landscape_height(landscape, o.x + d.x * t, o.z + d.z * t);
}

bool voe_3d_landscape_ray(const voe_assets_landscape *landscape,
			  voe_math_float3 origin, voe_math_float3 direction,
			  float *distance)
{
	VOE_BASE_ASSERT(landscape && landscape->heights, "a landscape");
	VOE_BASE_ASSERT(distance, "somewhere to put the distance");
	const float length = sqrtf(direction.x * direction.x +
				   direction.y * direction.y +
				   direction.z * direction.z);
	voe_math_float3 low, high;
	float enter = 0.0f, leave = INFINITY;

	if (length <= 0.0f)
		return false;
	voe_3d_landscape_box(landscape, &low, &high);
	// A flat grid's box has no thickness; pad it so the slab holds it.
	low.y -= 0.01f;
	high.y += 0.01f;
	if (!slab(origin.x, direction.x, low.x, high.x, &enter, &leave) ||
	    !slab(origin.y, direction.y, low.y, high.y, &enter, &leave) ||
	    !slab(origin.z, direction.z, low.z, high.z, &enter, &leave))
		return false;
	const float step = 0.5f * cell_size(landscape) / length;
	const uint32_t steps = (uint32_t)ceilf((leave - enter) / step) + 1;
	float before = enter;

	if (above(landscape, origin, direction, enter) <= 0.0f) {
		*distance = enter;
		return true;
	}
	for (uint32_t i = 1; i <= steps; i++) {
		const float t = fminf(enter + step * (float)i, leave);

		if (above(landscape, origin, direction, t) <= 0.0f) {
			float a = before, b = t;

			for (int k = 0; k < BISECTIONS; k++) {
				const float mid = 0.5f * (a + b);

				if (above(landscape, origin, direction, mid) <= 0.0f)
					b = mid;
				else
					a = mid;
			}
			*distance = b;
			return true;
		}
		before = t;
	}
	return false;
}

// 1 inside radius·(1 − softness), smoothstepping to 0 at the radius.
static float weight(const voe_3d_brush *brush, float distance)
{
	const float inner = brush->radius * (1.0f - brush->softness);

	if (distance <= inner)
		return 1.0f;
	if (distance >= brush->radius)
		return 0.0f;
	const float t = (brush->radius - distance) / (brush->radius - inner);

	return t * t * (3.0f - 2.0f * t);
}

// The height indices within `radius` of `centre` on one axis, end exclusive.
static void span(const voe_assets_landscape *landscape, float centre,
		 float radius, uint32_t *first, uint32_t *end)
{
	const float half = 0.5f * landscape->size;
	const float cell = cell_size(landscape);
	const float lo = ceilf((centre - radius + half) / cell);
	const float hi = floorf((centre + radius + half) / cell) + 1.0f;

	*first = (uint32_t)fminf(fmaxf(lo, 0.0f), (float)landscape->cells + 1);
	*end = (uint32_t)fminf(fmaxf(hi, 0.0f), (float)landscape->cells + 1);
}

// The mean of the up to 8 neighbours of (c, r) in `copy`, which holds the
// heights from (cx, rz) on, `width` a row.
static float neighbours_mean(const voe_assets_landscape *landscape,
			     const float *copy, uint32_t width, uint32_t cx,
			     uint32_t rz, uint32_t c, uint32_t r)
{
	float sum = 0.0f;
	int count = 0;

	for (int dr = -1; dr <= 1; dr++) {
		for (int dc = -1; dc <= 1; dc++) {
			const int64_t nc = (int64_t)c + dc;
			const int64_t nr = (int64_t)r + dr;

			if ((dc == 0 && dr == 0) || nc < 0 || nr < 0 ||
			    nc > landscape->cells || nr > landscape->cells)
				continue;
			sum += copy[(size_t)(nr - rz) * width + (size_t)(nc - cx)];
			count++;
		}
	}
	VOE_BASE_ASSERT(count >= 3, "a grid point has at least 3 neighbours");
	return sum / (float)count;
}

// The rect one wider on each side, clamped, copied onto `scratch`.
static float *copy_around(const voe_assets_landscape *landscape,
			  voe_3d_landscape_rect rect, voe_base_arena *scratch,
			  voe_3d_landscape_rect *around)
{
	const uint32_t n = landscape->cells + 1;

	around->x0 = rect.x0 > 0 ? rect.x0 - 1 : 0;
	around->z0 = rect.z0 > 0 ? rect.z0 - 1 : 0;
	around->x1 = rect.x1 < n ? rect.x1 + 1 : n;
	around->z1 = rect.z1 < n ? rect.z1 + 1 : n;
	const uint32_t width = around->x1 - around->x0;
	float *copy = voe_base_arena_push(
		scratch, sizeof(float) * width * (around->z1 - around->z0));

	for (uint32_t r = around->z0; r < around->z1; r++)
		for (uint32_t c = around->x0; c < around->x1; c++)
			copy[(size_t)(r - around->z0) * width + (c - around->x0)] =
				at(landscape, c, r);
	return copy;
}

voe_3d_landscape_rect voe_3d_landscape_brush(voe_assets_landscape *landscape,
					     const voe_3d_brush *brush, float x,
					     float z, float seconds,
					     voe_base_arena *scratch)
{
	VOE_BASE_ASSERT(landscape && landscape->heights && brush && scratch,
			"a landscape, a brush and scratch");
	VOE_BASE_ASSERT(brush->radius > 0.0f && seconds >= 0.0f,
			"a radius and time not running backwards");
	const float s = fminf(seconds, LONGEST_SECONDS);
	const float half = 0.5f * landscape->size;
	const float cell = cell_size(landscape);
	const uint32_t n = landscape->cells + 1;
	voe_3d_landscape_rect rect = { 0 };

	span(landscape, x, brush->radius, &rect.x0, &rect.x1);
	span(landscape, z, brush->radius, &rect.z0, &rect.z1);
	if (rect.x0 >= rect.x1 || rect.z0 >= rect.z1)
		return (voe_3d_landscape_rect){ 0 };
	const struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);
	voe_3d_landscape_rect around = { 0 };
	const float *copy = brush->kind == VOE_3D_BRUSH_SMOOTH ?
				    copy_around(landscape, rect, scratch, &around) :
				    NULL;

	for (uint32_t r = rect.z0; r < rect.z1; r++) {
		for (uint32_t c = rect.x0; c < rect.x1; c++) {
			const float dx = -half + (float)c * cell - x;
			const float dz = -half + (float)r * cell - z;
			const float w = weight(brush, sqrtf(dx * dx + dz * dz));
			float *h = &landscape->heights[(size_t)r * n + c];
			const float move = fminf(1.0f, 10.0f * brush->strength * w * s);

			if (w <= 0.0f)
				continue;
			if (brush->kind == VOE_3D_BRUSH_RAISE)
				*h += brush->strength * brush->radius * 0.5f * w * s;
			else if (brush->kind == VOE_3D_BRUSH_LOWER)
				*h -= brush->strength * brush->radius * 0.5f * w * s;
			else if (brush->kind == VOE_3D_BRUSH_FLATTEN)
				*h += (brush->target - *h) * move;
			else
				*h += (neighbours_mean(landscape, copy,
						       around.x1 - around.x0,
						       around.x0, around.z0, c, r) -
				       *h) * move;
		}
	}
	voe_base_arena_rewind(scratch, mark);
	return rect;
}
