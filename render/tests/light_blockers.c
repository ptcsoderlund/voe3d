// Which light blockers hold a point (ADR-0347 point 3), with no graphics card:
// the mask is plain arithmetic over boxes.
//
// A box is built here as a pass would be given it: centre c, unit axes a_i and
// half sizes h_i to rows (a_i / h_i, −a_i·c / h_i), and a sphere of radius |h|.
//
// AN UNROTATED BOX OF HALF SIZE 1 ABOUT (0, 0, −5) HOLDS ITS CENTRE AND A POINT
// ON ITS FACE, NOT ONE 1.01 m OUT. A BOX TURNED 45° ABOUT Y HOLDS A POINT ALONG
// ITS DIAGONAL AND NOT ONE JUST PAST ITS EDGE. A POINT IN TWO BOXES HAS BOTH
// BITS. BLOCKER 31 SETS BIT 31. NOUGHT BLOCKERS IS MASK 0. A POINT INSIDE THE
// SPHERE BUT OUTSIDE THE BOX IS 0. THE CALL IS REACHED THROUGH render/device.h.
#include <render/device.h>

#include <testing/test.h>

#include <math.h>

static const voe_math_float3 centre = { 0.0f, 0.0f, -5.0f };

static voe_render_light_blocker box(voe_math_float3 c, float turn,
				    voe_math_float3 h)
{
	const voe_math_float3 axes[3] = {
		{ cosf(turn), 0.0f, -sinf(turn) },
		{ 0.0f, 1.0f, 0.0f },
		{ sinf(turn), 0.0f, cosf(turn) },
	};
	const float half[3] = { h.x, h.y, h.z };
	voe_render_light_blocker b = { .sphere = {
		c.x, c.y, c.z, sqrtf(voe_math_float3_dot(h, h)) } };

	for (uint32_t i = 0; i < 3; i++) {
		const voe_math_float3 a = axes[i];

		b.rows[i] = (voe_math_float4){ a.x / half[i], a.y / half[i],
					       a.z / half[i],
					       -voe_math_float3_dot(a, c) / half[i] };
	}
	return b;
}

static uint32_t mask_of(const voe_render_light_blocker *b, uint32_t count,
			float x, float y, float z)
{
	return voe_render_light_blockers_mask(b, count,
					      (voe_math_float3){ x, y, z });
}

static void an_unrotated_box_holds_its_centre_and_face(void)
{
	const voe_render_light_blocker b = box(centre, 0.0f, (voe_math_float3){ 1, 1, 1 });

	VOE_TEST_CHECK_INT(mask_of(&b, 1, 0.0f, 0.0f, -5.0f), 1u);
	VOE_TEST_CHECK_INT(mask_of(&b, 1, 1.0f, 0.0f, -5.0f), 1u);
	VOE_TEST_CHECK_INT(mask_of(&b, 1, 0.0f, 0.0f, -4.0f), 1u);
	VOE_TEST_CHECK_INT(mask_of(&b, 1, 2.01f, 0.0f, -5.0f), 0u);
	VOE_TEST_CHECK_INT(mask_of(&b, 1, 0.0f, 0.0f, -3.99f), 0u);
}

static void a_turned_box_holds_its_diagonal(void)
{
	const voe_render_light_blocker b =
		box(centre, 0.78539816f, (voe_math_float3){ 1, 1, 1 });

	VOE_TEST_CHECK_INT(mask_of(&b, 1, 1.4f, 0.0f, -5.0f), 1u);
	VOE_TEST_CHECK_INT(mask_of(&b, 1, 1.43f, 0.0f, -5.0f), 0u);
}

static void a_point_in_two_boxes_has_both_bits(void)
{
	const voe_render_light_blocker b[2] = {
		box(centre, 0.0f, (voe_math_float3){ 1, 1, 1 }),
		box((voe_math_float3){ 1.5f, 0.0f, -5.0f }, 0.0f,
		    (voe_math_float3){ 1, 1, 1 }),
	};

	VOE_TEST_CHECK_INT(mask_of(b, 2, 0.75f, 0.0f, -5.0f), 3u);
	VOE_TEST_CHECK_INT(mask_of(b, 2, -0.75f, 0.0f, -5.0f), 1u);
	VOE_TEST_CHECK_INT(mask_of(b, 2, 2.25f, 0.0f, -5.0f), 2u);
}

static void blocker_31_sets_bit_31(void)
{
	voe_render_light_blocker b[VOE_RENDER_LIGHT_BLOCKERS];

	for (uint32_t i = 0; i < VOE_RENDER_LIGHT_BLOCKERS; i++)
		b[i] = box((voe_math_float3){ 0.0f, 0.0f, 50.0f }, 0.0f,
			   (voe_math_float3){ 1, 1, 1 });
	b[31] = box(centre, 0.0f, (voe_math_float3){ 1, 1, 1 });
	VOE_TEST_CHECK_INT(mask_of(b, VOE_RENDER_LIGHT_BLOCKERS, 0.0f, 0.0f, -5.0f),
			   1u << 31);
}

static void nought_blockers_is_mask_0(void)
{
	VOE_TEST_CHECK_INT(mask_of(NULL, 0, 0.0f, 0.0f, -5.0f), 0u);
}

static void inside_the_sphere_outside_the_box_is_0(void)
{
	const voe_render_light_blocker b = box(centre, 0.0f, (voe_math_float3){ 1, 1, 1 });

	VOE_TEST_CHECK_INT(mask_of(&b, 1, 1.2f, 1.2f, -5.0f), 0u);
}

// The public declaration is the one this file calls: its address taken
// through render/device.h alone, as 3d does for the sun's mask.
static void the_call_is_reached_through_device_h(void)
{
	uint32_t (*call)(const voe_render_light_blocker *, uint32_t,
			 voe_math_float3) = voe_render_light_blockers_mask;

	VOE_TEST_CHECK_INT(call(NULL, 0, centre), 0u);
}

int main(void)
{
	the_call_is_reached_through_device_h();
	an_unrotated_box_holds_its_centre_and_face();
	a_turned_box_holds_its_diagonal();
	a_point_in_two_boxes_has_both_bits();
	blocker_31_sets_bit_31();
	nought_blockers_is_mask_0();
	inside_the_sphere_outside_the_box_is_0();
	return voe_test_result();
}
