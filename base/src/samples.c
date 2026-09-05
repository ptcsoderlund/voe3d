// The three lines of arithmetic behind base/samples.h. Read that header first:
// what is worth knowing about this type is the shape it deliberately is not,
// and that is written down there.
#include <base/samples.h>

#include <base/assert.h>

#include <stddef.h>

void voe_base_samples_reset(voe_base_samples *samples)
{
	VOE_BASE_ASSERT(samples != NULL, "resetting no samples");

	*samples = (voe_base_samples){ 0 };
}

void voe_base_samples_add(voe_base_samples *samples, double value)
{
	VOE_BASE_ASSERT(samples != NULL, "adding to no samples");
	VOE_BASE_ASSERT(value >= 0.0,
			"a measurement that ran backwards — two clock readings taken in the wrong order");

	samples->count++;
	samples->sum += value;
	if (value > samples->worst)
		samples->worst = value;
}

double voe_base_samples_average(const voe_base_samples *samples)
{
	VOE_BASE_ASSERT(samples != NULL, "averaging no samples");

	if (samples->count == 0)
		return 0.0;
	return samples->sum / (double)samples->count;
}
