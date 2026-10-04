// The light blockers' one call: a point to the mask of the boxes holding it
// (see voe_render_light_blockers_mask in render/device.h for the rule and why
// the sphere goes first).
//
// Plain arithmetic over the caller's array; nothing is allocated or kept.
//
// CONSTRAINTS. Cost is count × one compare, plus three dot products for each
// sphere the point is in; at VOE_RENDER_LIGHT_BLOCKERS that is all it needs.
#include <render/device.h>

#include <assert.h>
#include <math.h>

static_assert(VOE_RENDER_LIGHT_BLOCKERS <= 32, "a mask is one uint32_t");

static bool holds(const voe_render_light_blocker *b, voe_math_float3 p)
{
	const voe_math_float3 c = { b->sphere.x, b->sphere.y, b->sphere.z };
	const voe_math_float3 d = { p.x - c.x, p.y - c.y, p.z - c.z };

	if (voe_math_float3_dot(d, d) > b->sphere.w * b->sphere.w)
		return false;
	for (uint32_t i = 0; i < 3; i++) {
		const voe_math_float4 r = b->rows[i];

		if (!(fabsf(r.x * p.x + r.y * p.y + r.z * p.z + r.w) <= 1.0f))
			return false;
	}
	return true;
}

uint32_t voe_render_light_blockers_mask(const voe_render_light_blocker *blockers,
					uint32_t count, voe_math_float3 point)
{
	assert(count <= VOE_RENDER_LIGHT_BLOCKERS);
	assert(blockers != NULL || count == 0);
	uint32_t mask = 0;

	for (uint32_t i = 0; i < count; i++)
		if (holds(&blockers[i], point))
			mask |= 1u << i;
	return mask;
}
