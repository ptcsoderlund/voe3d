// What base/samples.h promises, checked from outside: that an empty run reads as
// noughts rather than dividing by zero, that the average is over exactly the
// values added, that the worst is the largest of them and not the last one, and
// that a reset really does start a new period rather than leaving the worst
// behind.
//
// THE WORST SURVIVING A RESET IS THE FAILURE THIS FILE IS REALLY FOR. It is the
// one mistake here that looks right: every average would still be correct, every
// count would still be correct, and a single slow frame would go on being
// reported as the worst of every period after it for the rest of the run. A
// person watching the console would read one stutter as a permanent one.
#include <base/samples.h>

#include <testing/test.h>

int main(void)
{
	voe_base_samples samples = { 0 };

	// Nothing added: no division, and both numbers say so.
	VOE_TEST_CHECK_INT((long long)samples.count, 0);
	VOE_TEST_CHECK_FLOAT(voe_base_samples_average(&samples), 0.0, 0.0);
	VOE_TEST_CHECK_FLOAT(samples.worst, 0.0, 0.0);

	// The worst arrives in the middle, so a `worst` that were really "the
	// last one" or "the first one" fails here rather than passing by luck.
	voe_base_samples_add(&samples, 1.0);
	voe_base_samples_add(&samples, 4.0);
	voe_base_samples_add(&samples, 2.0);

	VOE_TEST_CHECK_INT((long long)samples.count, 3);
	VOE_TEST_CHECK_FLOAT(samples.sum, 7.0, 1e-12);
	VOE_TEST_CHECK_FLOAT(voe_base_samples_average(&samples), 7.0 / 3.0, 1e-12);
	VOE_TEST_CHECK_FLOAT(samples.worst, 4.0, 0.0);

	// A period that ends and one that begins. The 4.0 above is gone: the new
	// period's worst is the worst of the new period.
	voe_base_samples_reset(&samples);
	VOE_TEST_CHECK_INT((long long)samples.count, 0);
	VOE_TEST_CHECK_FLOAT(samples.sum, 0.0, 0.0);
	VOE_TEST_CHECK_FLOAT(samples.worst, 0.0, 0.0);

	voe_base_samples_add(&samples, 0.5);
	VOE_TEST_CHECK_INT((long long)samples.count, 1);
	VOE_TEST_CHECK_FLOAT(voe_base_samples_average(&samples), 0.5, 1e-12);
	VOE_TEST_CHECK_FLOAT(samples.worst, 0.5, 0.0);

	// Zero is a value and not an absence. A frame that measured as taking no
	// time at all still counts, or an average over a period with a very fast
	// frame in it is an average over one fewer frame than it claims.
	voe_base_samples_add(&samples, 0.0);
	VOE_TEST_CHECK_INT((long long)samples.count, 2);
	VOE_TEST_CHECK_FLOAT(voe_base_samples_average(&samples), 0.25, 1e-12);
	VOE_TEST_CHECK_FLOAT(samples.worst, 0.5, 0.0);

	return voe_test_result();
}
