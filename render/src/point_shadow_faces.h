// Which of a point light's six cube faces a caster reaches (ADR-0325 point 2).
// Pure CPU, no Vulkan: a mesh's bounding sphere is taken once from its vertices,
// moved under each draw's world matrix, and tested against each slotted light.
//
//     voe_math_float4 local = voe_render_point_shadow_sphere(vertices, count);
//     voe_math_float4 world = voe_render_point_shadow_sphere_moved(local, m);
//     uint32_t faces = voe_render_point_shadow_faces(world, light, range);
//
// WHY A MASK. A caster is one instanced draw over every face it reaches, the
// vertex stage writing the layer: the mask says how many instances and which
// face the n-th one is (its n-th set bit). A caster no face reaches draws nothing.
//
// BIT f IS FACE f, IN THE ORDER +X −X +Y −Y +Z −Z (ADR-0325 point 1). The
// lookup in `shaders/point_shadow.slangh` picks a face by the same order and
// must keep it, or a caster lands in a layer nobody reads.
//
// CONSERVATIVE. A face's region is its 90° pyramid about the light, each of
// its four side planes moved out by the radius, so a sphere touching the
// pyramid always sets its bit; extras are allowed. A sphere containing the
// light reaches all six. A sphere wholly past the range reaches none.
//
// CONSTRAINTS. A sphere is a float4: xyz centre, w radius. The light and the
// sphere are in the same space (eye-relative for a pass). The moved radius uses
// the longest basis column, exact for uniform scale, loose under shear.
#pragma once

#include <render/device.h>

#include <stdint.h>

// The six faces' bits together.
#define VOE_RENDER_POINT_SHADOW_ALL_FACES 0x3fu

// The sphere about `count` positions: centre of their box, radius the farthest
// position from it. Asserts count ≥ 1.
voe_math_float4 voe_render_point_shadow_sphere(const voe_render_vertex *vertices,
					       uint32_t count);

// `sphere` under `world` (row-major, column vectors): centre transformed,
// radius times the longest of the three basis columns.
voe_math_float4 voe_render_point_shadow_sphere_moved(voe_math_float4 sphere,
						     voe_math_float4x4 world);

// The faces of the light at `light` with `range` that `sphere` reaches, bit f
// for face f; nought when the sphere is past the range.
uint32_t voe_render_point_shadow_faces(voe_math_float4 sphere,
				       voe_math_float3 light, float range);
