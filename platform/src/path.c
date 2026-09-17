// The platform-independent half of platform/path.h: joining, finding a
// parent and finding a name. Read the header first — what each call promises
// is written there. voe_platform_path_absolute is not here: it is the one
// call that has to ask the operating system something, and it lives in
// src/path_wayland.c and src/path_win32.c instead (see their headers).
//
// THE SEPARATOR DECISION IS ONE #ifdef, NOT ONE PER FUNCTION. Everything
// below reads VOE_PLATFORM_PATH_SEPARATOR to know what to write and calls
// is_separator() to know what to read; neither one is ever asked again which
// platform it is. That is the whole of what "compile-time constant rather
// than an #ifdef in each function" (task 4) buys: join(), parent() and
// name() are exactly one implementation, not two kept in step by hand.
//
// A ROOT IS A DIFFERENT SHAPE ON EACH PLATFORM — "/" against "C:\" — so
// is_root() still reads _WIN32 once, the same way the separator does. That is
// a shape the two platforms do not share, not a repeated decision.
//
// NAME NEVER COPIES. Its result is a pointer straight into path, which is
// why a path ending in a separator answers "" for its name: the character
// after the last separator is path's own terminating NUL, and there is
// nothing to trim off the end of a string this function does not own.
#include <platform/path.h>

#include <base/arena.h>
#include <base/assert.h>

#include <string.h>
#ifdef _WIN32
#include <ctype.h>
#endif

#ifdef _WIN32
#define VOE_PLATFORM_PATH_SEPARATOR '\\'
#else
#define VOE_PLATFORM_PATH_SEPARATOR '/'
#endif

// What counts as a separator when reading a path. Windows accepts either
// spelling; Linux accepts only its own.
static bool is_separator(char c)
{
#ifdef _WIN32
	return c == '\\' || c == '/';
#else
	return c == '/';
#endif
}

// True when path is exactly a root and nothing more: "/" on Linux, a drive
// letter followed by ':' and one separator ("C:\", "C:/") on Windows.
static bool is_root(const char *path)
{
	size_t length = strlen(path);

#ifdef _WIN32
	return length == 3 && isalpha((unsigned char)path[0]) &&
	       path[1] == ':' && is_separator(path[2]);
#else
	return length == 1 && is_separator(path[0]);
#endif
}

const char *voe_platform_path_join(voe_base_arena *arena, const char *folder,
				   const char *name)
{
	size_t folder_length;
	bool needs_separator;
	size_t name_length;
	char *result;
	size_t pos;

	VOE_BASE_ASSERT(arena != NULL, "joining a path into no arena");
	VOE_BASE_ASSERT(folder != NULL, "joining a path with no folder");
	VOE_BASE_ASSERT(name != NULL, "joining a path with no name");

	folder_length = strlen(folder);
	needs_separator =
		folder_length == 0 || !is_separator(folder[folder_length - 1]);
	name_length = strlen(name);

	result = voe_base_arena_push(
		arena, folder_length + (needs_separator ? 1 : 0) +
			       name_length + 1);

	memcpy(result, folder, folder_length);
	pos = folder_length;
	if (needs_separator)
		result[pos++] = VOE_PLATFORM_PATH_SEPARATOR;
	memcpy(result + pos, name, name_length);
	pos += name_length;
	result[pos] = '\0';

	return result;
}

const char *voe_platform_path_parent(voe_base_arena *arena, const char *path)
{
	size_t length;
	size_t effective;
	size_t sep_index;
	bool found;
	size_t parent_length;
	char *result;

	VOE_BASE_ASSERT(arena != NULL, "finding a path's parent into no arena");
	VOE_BASE_ASSERT(path != NULL, "finding the parent of no path");

	if (is_root(path))
		return NULL;

	// A trailing separator names nothing past it, so it is ignored before
	// the last name is found — path and "path" plus one separator share a
	// parent.
	length = strlen(path);
	effective = length;
	if (effective > 0 && is_separator(path[effective - 1]))
		effective--;

	found = false;
	sep_index = 0;
	for (size_t i = effective; i > 0; i--) {
		if (is_separator(path[i - 1])) {
			sep_index = i - 1;
			found = true;
			break;
		}
	}
	if (!found)
		return NULL;

	// Excluding the separator is the ordinary case ("/tmp/abc" -> "/tmp");
	// what is left over is restored to a whole root when trimming it would
	// otherwise leave a drive letter with no separator of its own, or
	// nothing at all.
	parent_length = sep_index;
#ifdef _WIN32
	if (parent_length == 2 && isalpha((unsigned char)path[0]) &&
	    path[1] == ':')
		parent_length = 3;
#else
	if (parent_length == 0)
		parent_length = 1;
#endif

	result = voe_base_arena_push(arena, parent_length + 1);
	memcpy(result, path, parent_length);
	result[parent_length] = '\0';
	return result;
}

const char *voe_platform_path_name(const char *path)
{
	size_t length;

	VOE_BASE_ASSERT(path != NULL, "finding the name of no path");

	length = strlen(path);
	for (size_t i = length; i > 0; i--) {
		if (is_separator(path[i - 1]))
			return path + i;
	}
	return path;
}
