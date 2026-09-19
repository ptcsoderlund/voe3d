// The built-in capsule's vertices and indices, worked out rather than written
// out. Internal to 3d: shape_system.c builds it once into its own arrays and
// hands the geometry to every entity with a voe_3d_shape of VOE_3D_SHAPE_CAPSULE
// (3d/shape_system.h, ADR-0191).
//
//     voe_render_vertex vertices[VOE_3D_CAPSULE_VERTICES];
//     uint32_t indices[VOE_3D_CAPSULE_INDICES];
//
//     voe_3d_capsule_build(vertices, indices);
//
// THE SHAPE IS ADR-0191's: upright along Y, radius 0.5, 2 tall from end to end,
// centred on the origin, a hemisphere at each end joined by a straight side.
// 32 segments around; each hemisphere 8 rings from its pole to its equator.
//
// EIGHTEEN ROWS OF THIRTY-THREE VERTICES: the two poles, seven rings between
// each pole and its equator, and the two equators, which the straight side
// joins without vertices of its own. Thirty-three and not thirty-two because
// the seam carries u = 0 on one side and u = 1 on the other, the same reason
// the cube has twenty-four vertices and not eight. A pole row is thirty-three
// copies of one point, each with its own u, so the band next to a pole is one
// triangle a segment rather than two, one of which would have no area.
//
// Normals point outward at unit length, smooth across the whole surface. u runs
// 0..1 around, the way that reads left to right from outside; v runs 0 at the
// top pole to 1 at the bottom one, by height. Every triangle is counter-
// clockwise seen from outside, as cube.c's are.
//
// The caller's arrays are exactly the two constants long; nothing is checked,
// because the only caller sizes them from these.
#pragma once

#include <render/device.h>

#include <stdint.h>

#define VOE_3D_CAPSULE_VERTICES 594
#define VOE_3D_CAPSULE_INDICES 3072

void voe_3d_capsule_build(voe_render_vertex *vertices, uint32_t *indices);
