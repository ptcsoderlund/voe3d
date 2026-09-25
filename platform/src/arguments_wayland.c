// The Linux half of platform/arguments.h: main's argv, handed back as it is.
// The name says wayland for the reason library_wayland.c gives; it means Linux.
// The arena is not used here.
#include <platform/arguments.h>

#include <base/assert.h>

voe_platform_arguments voe_platform_arguments_read(int argc, char *argv[],
						   voe_base_arena *arena)
{
	(void)arena;
	VOE_BASE_DEBUG_ASSERT(argc >= 0 && argv != NULL, "reading arguments main did not give");

	return (voe_platform_arguments){.count = argc, .values = (const char *const *)argv};
}
