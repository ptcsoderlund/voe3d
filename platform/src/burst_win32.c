// The Windows core count of platform/burst.h: GetActiveProcessorCount over
// every processor group, asked once and kept.
//
// ALL_PROCESSOR_GROUPS and not GetSystemInfo, whose count stops at the calling
// thread's group of at most 64. Two threads asking at once both ask and both
// store the same number, so the keep is an atomic and needs no lock. A zero
// answer, the call's failure, is kept as 1.
#include <platform/burst.h>

#include <stdatomic.h>

#include <windows.h>

static _Atomic uint32_t kept;

uint32_t voe_platform_cores(void)
{
	uint32_t cores = atomic_load_explicit(&kept, memory_order_relaxed);

	if (cores != 0)
		return cores;
	cores = (uint32_t)GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
	if (cores == 0)
		cores = 1;
	atomic_store_explicit(&kept, cores, memory_order_relaxed);
	return cores;
}
