// The placement law: a point ahead is centred and full at the reference, the
// screen edges pan to ±1, a point behind pans to its side, twice the reference
// is a quarter, and the reference comes from where the view axis meets y = 0.
#include <audio/place.h>

#include <testing/test.h>

#define TOLERANCE 1e-5f

// At the origin, looking along -z with +x to the right, 90 degrees wide.
static voe_audio_listener level(void)
{
	return voe_audio_listener_make((voe_math_double3){ 0, 0, 0 },
				       (voe_math_float3){ 1, 0, 0 },
				       (voe_math_float3){ 0, 0, -1 }, 1.0f);
}

static void ahead_at_the_reference_is_centred_and_full(void)
{
	const voe_audio_listener ear = level();
	const voe_audio_gains g = voe_audio_place(&ear, (voe_math_double3){ 0, 0, -10 });

	VOE_TEST_CHECK_FLOAT(g.left, 1.0f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(g.right, 1.0f, TOLERANCE);
	const voe_audio_gains here = voe_audio_place(&ear, (voe_math_double3){ 0, 0, 0 });

	VOE_TEST_CHECK_FLOAT(here.left, 1.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(here.right, 1.0f, 0.0f);
}

static void screen_edges_pan_fully(void)
{
	const voe_audio_listener ear = level();
	const voe_audio_gains right = voe_audio_place(&ear, (voe_math_double3){ 5, 0, -5 });
	const voe_audio_gains left = voe_audio_place(&ear, (voe_math_double3){ -5, 0, -5 });

	VOE_TEST_CHECK_FLOAT(right.left, 0.0f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(right.right, 1.0f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(left.left, 1.0f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(left.right, 0.0f, TOLERANCE);
}

static void behind_on_the_left_pans_left(void)
{
	const voe_audio_listener ear = level();
	const voe_audio_gains g = voe_audio_place(&ear, (voe_math_double3){ -1, 0, 5 });

	VOE_TEST_CHECK_FLOAT(g.left, 1.0f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(g.right, 0.0f, TOLERANCE);
}

static void twice_the_reference_is_a_quarter(void)
{
	const voe_audio_listener ear = level();
	const voe_audio_gains g = voe_audio_place(&ear, (voe_math_double3){ 0, 0, -20 });

	VOE_TEST_CHECK_FLOAT(g.left, 0.25f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(g.right, 0.25f, TOLERANCE);
}

static void reference_follows_the_view_axis(void)
{
	const voe_audio_listener down = voe_audio_listener_make(
		(voe_math_double3){ 3, 20, -7 }, (voe_math_float3){ 1, 0, 0 },
		(voe_math_float3){ 0, -1, 0 }, 1.0f);

	VOE_TEST_CHECK_FLOAT(down.reference, 20.0f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(level().reference, VOE_AUDIO_REFERENCE, 0.0f);
	const voe_audio_gains none = voe_audio_unplaced();

	VOE_TEST_CHECK_FLOAT(none.left, 1.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(none.right, 1.0f, 0.0f);
}

int main(void)
{
	ahead_at_the_reference_is_centred_and_full();
	screen_edges_pan_fully();
	behind_on_the_left_pans_left();
	twice_the_reference_is_a_quarter();
	reference_follows_the_view_axis();
	return voe_test_result();
}
