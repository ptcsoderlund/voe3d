// That the fractional scale turns logical units into buffer pixels as ADR-0180
// says: 120 is the identity for both lengths and positions, 150 is 1.25 and 180
// is 1.5, a length rounds half away from zero (1 at 1.5 is 2), zero stays zero,
// and a position keeps its fraction. Plain arithmetic, so no compositor needed.
//
// IT INCLUDES platform's INTERNAL HEADER BY RELATIVE PATH, as tests/input.c
// does: the conversion is not platform's public surface.
#include "../src/scale.h"

#include <testing/test.h>

static void one_twenty_is_the_identity(void)
{
	VOE_TEST_CHECK_INT(voe_platform_scale_length(1536, 120), 1536);
	VOE_TEST_CHECK_INT(voe_platform_scale_length(1, 120), 1);
	VOE_TEST_CHECK_INT(voe_platform_scale_length(0, 120), 0);
	VOE_TEST_CHECK_FLOAT(voe_platform_scale_position(100.5, 120), 100.5, 0.0);
	VOE_TEST_CHECK_FLOAT(voe_platform_scale_position(0.25, 120), 0.25, 0.0);
}

static void one_fifty_is_one_and_a_quarter(void)
{
	VOE_TEST_CHECK_INT(voe_platform_scale_length(1536, 150), 1920);
	VOE_TEST_CHECK_INT(voe_platform_scale_length(864, 150), 1080);
	VOE_TEST_CHECK_INT(voe_platform_scale_length(0, 150), 0);
	VOE_TEST_CHECK_FLOAT(voe_platform_scale_position(100.5, 150), 125.625, 0.0);
	VOE_TEST_CHECK_FLOAT(voe_platform_scale_position(0.0, 150), 0.0, 0.0);
}

static void a_half_rounds_away_from_zero(void)
{
	VOE_TEST_CHECK_INT(voe_platform_scale_length(1, 180), 2);
	VOE_TEST_CHECK_INT(voe_platform_scale_length(-1, 180), -2);
}

int main(void)
{
	one_twenty_is_the_identity();
	one_fifty_is_one_and_a_quarter();
	a_half_rounds_away_from_zero();
	return voe_test_result();
}
