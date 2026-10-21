// The Windows core count: GetActiveProcessorCount(ALL_PROCESSOR_GROUPS). Read
// platform/cores.h first — what the count is for and why it is at least 1 is
// written there. Written, not built here (0381).
//
// ALL_PROCESSOR_GROUPS AND NOT GetSystemInfo. GetSystemInfo's count stops at
// the calling thread's processor group, 64 cores at most; a machine with more
// would size its pool by one group. The call answers 0 when it fails, a count
// the OS will not give, and so 1.
#include <platform/cores.h>

#include <base/assert.h>

#include <windows.h>

uint32_t voe_platform_cores(void)
{
	DWORD active = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
	uint32_t cores = active > 0 ? (uint32_t)active : 1;

	VOE_BASE_ASSERT(cores >= 1, "a machine running this has a core");
	return cores;
}
