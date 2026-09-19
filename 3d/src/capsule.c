// The capsule's rows and bands. See capsule.h for the shape and why it has
// eighteen rows of thirty-three vertices.
//
// A POINT AT ANGLE theta AROUND IS (sin theta, y, cos theta) times its radius:
// theta = 0 faces +Z and grows towards +X, which is left to right seen from
// outside. A row's angle phi is measured from the top pole, so its height on
// its own hemisphere is cos phi and its radius sin phi; the normal is that same
// direction, unscaled, which is what makes it unit length.
#include "capsule.h"

#include <math.h>

#define SEGMENTS 32
#define RINGS 8
#define ROWS (2 * (RINGS + 1))
#define COLUMNS (SEGMENTS + 1)
#define RADIUS 0.5f
// How far each hemisphere's centre sits from the origin: half the straight side.
#define HALF_SIDE 0.5f
#define PI 3.14159265358979323846f

static_assert(ROWS * COLUMNS == VOE_3D_CAPSULE_VERTICES);
static_assert((ROWS - 1) * SEGMENTS * 6 - 2 * SEGMENTS * 3 ==
	      VOE_3D_CAPSULE_INDICES);

void voe_3d_capsule_build(voe_render_vertex *vertices, uint32_t *indices)
{
	uint32_t n = 0;

	for (uint32_t row = 0; row < ROWS; row++) {
		// Rows 0..RINGS are the top hemisphere, pole to equator; the rest
		// the bottom one, equator to pole.
		bool top = row <= RINGS;
		// The two equators share phi = pi / 2, one row apart.
		float phi = (float)(top ? row : row - 1) * (PI / 2.0f) /
			    (float)RINGS;
		float centre = top ? HALF_SIDE : -HALF_SIDE;

		for (uint32_t column = 0; column < COLUMNS; column++) {
			float theta = (float)column * 2.0f * PI / (float)SEGMENTS;
			float ring = sinf(phi);
			voe_math_float3 normal = { ring * sinf(theta), cosf(phi),
						   ring * cosf(theta) };
			float y = centre + RADIUS * normal.y;

			vertices[row * COLUMNS + column] = (voe_render_vertex){
				.position = { RADIUS * normal.x, y,
					      RADIUS * normal.z },
				.normal = normal,
				// A pole's u is the middle of its segment, so
				// the one triangle there reads straight.
				.uv = { ((float)column +
					 (row == 0 || row == ROWS - 1 ? 0.5f
								     : 0.0f)) /
						(float)SEGMENTS,
					(1.0f - y) / 2.0f },
			};
		}
	}

	// The same two triangles a quad as cube.c's faces: bottom-left,
	// bottom-right, top-right, then bottom-left, top-right, top-left, with
	// row + 1 below row. Next to a pole, the one with no area is left out.
	for (uint32_t row = 0; row + 1 < ROWS; row++) {
		for (uint32_t column = 0; column < SEGMENTS; column++) {
			uint32_t top_left = row * COLUMNS + column;
			uint32_t top_right = top_left + 1;
			uint32_t bottom_left = top_left + COLUMNS;
			uint32_t bottom_right = bottom_left + 1;

			if (row != 0) {
				indices[n++] = bottom_left;
				indices[n++] = top_right;
				indices[n++] = top_left;
			}
			if (row + 2 != ROWS) {
				indices[n++] = bottom_left;
				indices[n++] = bottom_right;
				indices[n++] = top_right;
			}
		}
	}
}
