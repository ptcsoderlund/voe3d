// The Linux half of platform/environment.h: getenv, its value copied onto the
// arena. Read the header first. The name says wayland for the reason
// library_wayland.c gives; it means Linux. The bytes are UTF-8 as they stand,
// so nothing is converted.
#include <platform/environment.h>

#include <base/assert.h>

#include <stdlib.h>
#include <string.h>

const char *voe_platform_environment_read(const char *name,
					  voe_base_arena *arena)
{
	const char *value;
	size_t length;
	char *copy;

	VOE_BASE_ASSERT(name != NULL, "reading an environment variable with no name");
	VOE_BASE_ASSERT(arena != NULL, "reading an environment variable into no arena");

	value = getenv(name);
	if (value == NULL)
		return NULL;

	length = strlen(value);
	copy = voe_base_arena_push(arena, length + 1);
	memcpy(copy, value, length + 1);
	return copy;
}
