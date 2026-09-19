// The built-in cylinder's vertices and indices, worked out rather than written
// out. Internal to 3d: shape_system.c builds it once into its own arrays and
// hands the geometry to every entity with a voe_3d_shape of VOE_3D_SHAPE_CYLINDER
// (3d/shape_system.h, ADR-0191).
//
//     voe_render_vertex vertices[VOE_3D_CYLINDER_VERTICES];
//     uint32_t indices[VOE_3D_CYLINDER_INDICES];
//
//     voe_3d_cylinder_build(vertices, indices);
//
// THE SHAPE IS ADR-0191's: upright along Y, radius 0.5, 1 tall, flat caps,
// centred on the origin, 32 segments around.
//
// THE SIDE AND THE CAPS DO NOT SHARE A VERTEX, because the rim is one point with
// two normals — outward on the side, straight up or down on a cap — and a vertex
// carries one of each attribute, the same reason the cube has twenty-four
// vertices and not eight. The side is two rows of thirty-three vertices, the
// seam carrying u = 0 on one side and u = 1 on the other; u runs 0..1 around,
// left to right from outside, and v 0 at the top to 1 at the bottom. Each cap is
// a centre and thirty-two rim vertices in a fan, u and v running 0..1 across it
// as if the cap were a picture laid on it seen from outside.
//
// Normals point outward at unit length. Every triangle is counter-clockwise
// seen from outside, as cube.c's are.
//
// The caller's arrays are exactly the two constants long; nothing is checked,
// because the only caller sizes them from these.
#pragma once

#include <render/device.h>

#include <stdint.h>

#define VOE_3D_CYLINDER_VERTICES 132
#define VOE_3D_CYLINDER_INDICES 384

void voe_3d_cylinder_build(voe_render_vertex *vertices, uint32_t *indices);
