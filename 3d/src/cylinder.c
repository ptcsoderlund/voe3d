// The cylinder's side and caps. See cylinder.h for the shape and why the rim
// is built three times.
//
// A POINT AT ANGLE theta AROUND IS (sin theta, y, cos theta) times the radius:
// theta = 0 faces +Z and grows towards +X, which is left to right seen from
// outside, the same as capsule.c.
#include "cylinder.h"

#include <math.h>

#define SEGMENTS 32
#define COLUMNS (SEGMENTS + 1)
#define RADIUS 0.5f
#define HALF_HEIGHT 0.5f
#define PI 3.14159265358979323846f

// The side's two rows, then the top cap's centre and rim, then the bottom's.
#define SIDE_VERTICES (2 * COLUMNS)
#define CAP_VERTICES (1 + SEGMENTS)

static_assert(SIDE_VERTICES + 2 * CAP_VERTICES == VOE_3D_CYLINDER_VERTICES);
static_assert(SEGMENTS * 6 + 2 * SEGMENTS * 3 == VOE_3D_CYLINDER_INDICES);

// One cap, a fan around its centre at `first`: the normal straight up or down,
// and u, v laid on it as a picture seen from outside — +X to the right, and
// whichever of ±Z is the far side as seen from there at the top.
static uint32_t build_cap(voe_render_vertex *vertices, uint32_t *indices,
			  uint32_t n, uint32_t first, float side)
{
	voe_math_float3 normal = { 0.0f, side, 0.0f };

	vertices[first] = (voe_render_vertex){
		.position = { 0.0f, side * HALF_HEIGHT, 0.0f },
		.normal = normal,
		.uv = { 0.5f, 0.5f },
	};
	for (uint32_t column = 0; column < SEGMENTS; column++) {
		float theta = (float)column * 2.0f * PI / (float)SEGMENTS;
		float x = RADIUS * sinf(theta);
		float z = RADIUS * cosf(theta);
		uint32_t here = first + 1 + column;
		uint32_t next = first + 1 + (column + 1) % SEGMENTS;

		vertices[here] = (voe_render_vertex){
			.position = { x, side * HALF_HEIGHT, z },
			.normal = normal,
			.uv = { 0.5f + x, 0.5f + side * z },
		};
		// Around the centre with theta growing is counter-clockwise from
		// above; from below it is the other way round.
		indices[n++] = first;
		indices[n++] = side > 0.0f ? here : next;
		indices[n++] = side > 0.0f ? next : here;
	}
	return n;
}

void voe_3d_cylinder_build(voe_render_vertex *vertices, uint32_t *indices)
{
	uint32_t n = 0;

	for (uint32_t row = 0; row < 2; row++) {
		float y = row == 0 ? HALF_HEIGHT : -HALF_HEIGHT;

		for (uint32_t column = 0; column < COLUMNS; column++) {
			float theta = (float)column * 2.0f * PI / (float)SEGMENTS;
			voe_math_float3 normal = { sinf(theta), 0.0f, cosf(theta) };

			vertices[row * COLUMNS + column] = (voe_render_vertex){
				.position = { RADIUS * normal.x, y,
					      RADIUS * normal.z },
				.normal = normal,
				.uv = { (float)column / (float)SEGMENTS,
					(float)row },
			};
		}
	}

	// The same two triangles a quad as cube.c's faces: bottom-left,
	// bottom-right, top-right, then bottom-left, top-right, top-left.
	for (uint32_t column = 0; column < SEGMENTS; column++) {
		uint32_t top_left = column;
		uint32_t bottom_left = COLUMNS + column;

		indices[n++] = bottom_left;
		indices[n++] = bottom_left + 1;
		indices[n++] = top_left + 1;
		indices[n++] = bottom_left;
		indices[n++] = top_left + 1;
		indices[n++] = top_left;
	}

	n = build_cap(vertices, indices, n, SIDE_VERTICES, 1.0f);
	(void)build_cap(vertices, indices, n, SIDE_VERTICES + CAP_VERTICES,
			-1.0f);
}
