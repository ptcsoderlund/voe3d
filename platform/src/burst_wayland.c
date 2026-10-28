// The Linux core count of platform/burst.h: sysconf's online processors, asked
// once and kept. Linux and not Wayland despite the name, as clock_wayland.c says.
//
// Online and not configured: a processor taken offline runs nothing. Two
// threads asking at once both ask and both store the same number, so the keep
// is an atomic and needs no lock. A failed or zero answer is kept as 1.
#define _POSIX_C_SOURCE 200809L

#include <platform/burst.h>

#include <stdatomic.h>
#include <unistd.h>

static _Atomic uint32_t kept;

uint32_t voe_platform_cores(void)
{
	uint32_t cores = atomic_load_explicit(&kept, memory_order_relaxed);
	long online;

	if (cores != 0)
		return cores;
	online = sysconf(_SC_NPROCESSORS_ONLN);
	cores = online < 1 ? 1 : (online > UINT32_MAX ? UINT32_MAX : (uint32_t)online);
	atomic_store_explicit(&kept, cores, memory_order_relaxed);
	return cores;
}
