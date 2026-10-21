// The Linux core count: sysconf(_SC_NPROCESSORS_ONLN). Read platform/cores.h
// first — what the count is for and why it is at least 1 is written there.
//
// LINUX AND NOT WAYLAND, DESPITE THE NAME, as every _wayland file here is: the
// suffix is the build's way of saying Linux only. Nothing in here knows there
// is a compositor.
//
// A sysconf that fails answers -1 and one that cannot tell may answer 0; both
// are a count the OS will not give, and so 1.
#include <platform/cores.h>

#include <base/assert.h>

#include <unistd.h>

uint32_t voe_platform_cores(void)
{
	long online = sysconf(_SC_NPROCESSORS_ONLN);
	uint32_t cores = online > 0 && online <= (long)UINT32_MAX ? (uint32_t)online : 1;

	VOE_BASE_ASSERT(cores >= 1, "a machine running this has a core");
	return cores;
}
