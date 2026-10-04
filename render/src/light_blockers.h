// Which of a pass's light blockers hold a point (ADR-0347 point 3). Pure CPU,
// no Vulkan: a point light's mask is worked out here at pass begin.
//
//     uint32_t mask = voe_render_light_blockers_mask(blockers.blockers,
//                                                    blockers.count,
//                                                    light->position);
//
// BIT i IS BLOCKER i. A point inside two boxes has both bits; nought blockers,
// or a point in none, is mask 0.
//
// THE SPHERE FIRST, THEN THE ROWS. A point outside a blocker's bounding sphere
// is outside it with one compare; one inside is outside unless every row's
// |row.xyz·p + row.w| is at most 1, the face counted as inside.
//
// draw.slang and the relight test a point the same way, so the CPU's mask for
// a lamp and the shader's for a surface agree.
//
// CONSTRAINTS. Cost is count × one compare, plus three dot products for each
// sphere the point is in; at VOE_RENDER_LIGHT_BLOCKERS that is all it needs.
#pragma once

#include <render/device.h>

#include <stdint.h>

// The mask of the blockers in `blockers[0, count)` that hold `point`, which is
// in the pass's space. Asserts count ≤ VOE_RENDER_LIGHT_BLOCKERS.
uint32_t voe_render_light_blockers_mask(const voe_render_light_blocker *blockers,
					uint32_t count, voe_math_float3 point);
