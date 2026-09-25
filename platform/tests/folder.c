// What platform/folder.h promises, checked from outside: entries are sorted by
// byte order with '.' and '..' left out, folder and hidden are answered
// correctly, an empty folder lists zero, a missing folder fails as
// UNAVAILABLE, creating over an existing name is REFUSED and under a missing
// parent is UNAVAILABLE, and _settings honours XDG_CONFIG_HOME. Needs no
// window and no display, so it runs under ctest on a machine with neither.
//
// PLATFORM IS ALLOWED OS HEADERS EVERYWHERE IN IT, TESTS INCLUDED
// (check.cmake step 5), so the scratch tree below is made and torn down with
// mkdir/rmdir/remove directly, as platform/tests/file.c already does, and for
// the same reason: proving the promise from outside rather than with the
// folder that is itself under test. voe_platform_file_write from the sibling
// task builds the two scratch files, which is dogfooding a settled API, not
// the one under test here.
//
// SETTINGS' XDG_CONFIG_HOME CHECKS ARE LINUX-ONLY, GUARDED BY #ifndef _WIN32.
// setenv/unsetenv are POSIX (_POSIX_C_SOURCE 200809L, defined below before any
// header pulls in <stdlib.h>, the same macro platform/src/folder_wayland.c
// sets for the same standard); platform/src/folder_win32.c reads a different
// variable (%APPDATA%) with no fallback to check the same way. Task 3's
// Windows backend for _settings is written and unverified, as ADR-0130
// allows.
//
// HIDDEN IS ONE MEANING ON BOTH PLATFORMS (ADR-0166). The .dotted checks in
// the three-entry listing below are unconditional and unchanged by that
// decision — they are the promise, proved identically on both platforms, not
// a Linux-only check. The #ifdef _WIN32 block right after them checks the
// other half of the same promise: a name Windows itself marks hidden with
// FILE_ATTRIBUTE_HIDDEN is hidden too, on top of the dot rule. That block is
// the one part of this file the coder could not run on Linux; the rest,
// .dotted checks included, ran here exactly as it will on Windows.
//
// A NON-ASCII NAME IS ITS OWN CASE (ADR-0247): a folder and a file named
// outside ASCII come back from a listing byte for byte. Their cleanup on
// Windows goes through _wremove/_wrmdir, since the narrow C runtime reads a
// name in the legacy code page, not UTF-8.
#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include <platform/folder.h>

#include <base/arena.h>
#include <platform/file.h>
#include <testing/test.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#else
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#define LIST_ROOT "voe_platform_folder_test_list"
#define LIST_NESTED LIST_ROOT "/nested"
#define LIST_FILE LIST_ROOT "/file.txt"
#define LIST_DOTTED LIST_ROOT "/.dotted"

#define CREATE_ROOT "voe_platform_folder_test_create"
#define CREATE_MISSING_CHILD CREATE_ROOT "/missing_parent/child"

#define WORLD "Åsa 李 värld"
#define WORLD_FOLDER WORLD "/Östen 张"
#define WORLD_FILE WORLD "/å.txt"

static bool make_folder(const char *path)
{
#ifdef _WIN32
	return _mkdir(path) == 0;
#else
	return mkdir(path, 0755) == 0;
#endif
}

static void remove_folder(const char *path)
{
#ifdef _WIN32
	(void)_rmdir(path);
#else
	(void)rmdir(path);
#endif
}

// Removes a file, or with folder a folder, by a UTF-8 name — see the header
// comment for why Windows converts it first.
static void remove_utf8(const char *path, bool folder)
{
#ifdef _WIN32
	wchar_t wide[MAX_PATH];

	if (MultiByteToWideChar(CP_UTF8, 0, path, -1, wide, MAX_PATH) == 0)
		return;
	(void)(folder ? _wrmdir(wide) : _wremove(wide));
#else
	(void)(folder ? rmdir(path) : remove(path));
#endif
}

// Torn down at the start, so a leftover from a previous failed run cannot make
// this run report the wrong thing, and at the end, whether the checks passed
// or not.
static void cleanup(void)
{
	(void)remove(LIST_FILE);
	(void)remove(LIST_DOTTED);
	remove_folder(LIST_NESTED);
	remove_folder(LIST_ROOT);
	remove_folder(CREATE_ROOT);
	remove_utf8(WORLD_FILE, false);
	remove_utf8(WORLD_FOLDER, true);
	remove_utf8(WORLD, true);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(4096);
	voe_base_error error = VOE_BASE_OK;
	static const uint8_t byte[] = { 'x' };

	cleanup();

	// The tree the listing checks below read back: an empty folder, a
	// plain file and a dotted one.
	VOE_TEST_CHECK(make_folder(LIST_ROOT));
	VOE_TEST_CHECK(make_folder(LIST_NESTED));
	VOE_TEST_CHECK(voe_platform_file_write(LIST_FILE, byte, sizeof byte,
					       NULL));
	VOE_TEST_CHECK(voe_platform_file_write(LIST_DOTTED, byte, sizeof byte,
					       NULL));

	// An empty folder lists zero.
	{
		voe_platform_folder_listing listing;

		VOE_TEST_CHECK(voe_platform_folder_list(LIST_NESTED, arena,
							&listing, &error));
		VOE_TEST_CHECK_INT(error, VOE_BASE_OK);
		VOE_TEST_CHECK_INT(listing.count, 0);
	}

	// Three entries, sorted by byte order — '.' (0x2e) sorts before a
	// letter — with folder and hidden answered for each.
	{
		voe_platform_folder_listing listing;

		VOE_TEST_CHECK(voe_platform_folder_list(LIST_ROOT, arena,
							&listing, &error));
		VOE_TEST_CHECK_INT(error, VOE_BASE_OK);
		VOE_TEST_CHECK_INT(listing.count, 3);
		if (listing.count == 3) {
			VOE_TEST_CHECK(strcmp(listing.entries[0].name,
					      ".dotted") == 0);
			VOE_TEST_CHECK(!listing.entries[0].folder);
			VOE_TEST_CHECK(listing.entries[0].hidden);

			VOE_TEST_CHECK(strcmp(listing.entries[1].name,
					      "file.txt") == 0);
			VOE_TEST_CHECK(!listing.entries[1].folder);
			VOE_TEST_CHECK(!listing.entries[1].hidden);

			VOE_TEST_CHECK(strcmp(listing.entries[2].name,
					      "nested") == 0);
			VOE_TEST_CHECK(listing.entries[2].folder);
			VOE_TEST_CHECK(!listing.entries[2].hidden);
		}
	}

#ifdef _WIN32
	// ADR-0166's other half: FILE_ATTRIBUTE_HIDDEN marks a further entry
	// hidden, on top of the dot rule already checked above. One call to
	// mark file.txt hidden, one relisting to read it back — the attribute
	// is cleared again right here so a rerun of this test is not affected
	// by a leftover from this one.
	{
		voe_platform_folder_listing listing;

		VOE_TEST_CHECK(SetFileAttributesA(LIST_FILE,
						  FILE_ATTRIBUTE_HIDDEN));
		VOE_TEST_CHECK(voe_platform_folder_list(LIST_ROOT, arena,
							&listing, &error));
		VOE_TEST_CHECK_INT(error, VOE_BASE_OK);
		VOE_TEST_CHECK_INT(listing.count, 3);
		if (listing.count == 3) {
			VOE_TEST_CHECK(strcmp(listing.entries[0].name,
					      ".dotted") == 0);
			VOE_TEST_CHECK(listing.entries[0].hidden);

			VOE_TEST_CHECK(strcmp(listing.entries[1].name,
					      "file.txt") == 0);
			VOE_TEST_CHECK(listing.entries[1].hidden);
		}
		VOE_TEST_CHECK(SetFileAttributesA(LIST_FILE,
						  FILE_ATTRIBUTE_NORMAL));
	}
#endif

	// Names in any script come back byte for byte, sorted by byte order:
	// "Ö" is 0xc3 0x96 and "å" 0xc3 0xa5, so the folder comes first.
	VOE_TEST_CHECK(voe_platform_folder_create(WORLD, NULL));
	VOE_TEST_CHECK(voe_platform_folder_create(WORLD_FOLDER, NULL));
	VOE_TEST_CHECK(voe_platform_file_write(WORLD_FILE, byte, sizeof byte,
					       NULL));
	{
		voe_platform_folder_listing listing;

		VOE_TEST_CHECK(voe_platform_folder_list(WORLD, arena, &listing,
							&error));
		VOE_TEST_CHECK_INT(error, VOE_BASE_OK);
		VOE_TEST_CHECK_INT(listing.count, 2);
		if (listing.count == 2) {
			VOE_TEST_CHECK(strcmp(listing.entries[0].name,
					      "Östen 张") == 0);
			VOE_TEST_CHECK(listing.entries[0].folder);

			VOE_TEST_CHECK(strcmp(listing.entries[1].name,
					      "å.txt") == 0);
			VOE_TEST_CHECK(!listing.entries[1].folder);
		}
	}

	// A folder that is not there fails as UNAVAILABLE.
	error = VOE_BASE_OK;
	{
		voe_platform_folder_listing listing;

		VOE_TEST_CHECK(!voe_platform_folder_list(
			"voe_platform_folder_test_missing", arena, &listing,
			&error));
		VOE_TEST_CHECK_INT(error, VOE_BASE_ERROR_UNAVAILABLE);
	}

	// Made once, refused the second time, UNAVAILABLE under a parent that
	// is not there.
	error = VOE_BASE_OK;
	VOE_TEST_CHECK(voe_platform_folder_create(CREATE_ROOT, &error));
	VOE_TEST_CHECK_INT(error, VOE_BASE_OK);

	error = VOE_BASE_OK;
	VOE_TEST_CHECK(!voe_platform_folder_create(CREATE_ROOT, &error));
	VOE_TEST_CHECK_INT(error, VOE_BASE_ERROR_REFUSED);

	error = VOE_BASE_OK;
	VOE_TEST_CHECK(!voe_platform_folder_create(CREATE_MISSING_CHILD,
						   &error));
	VOE_TEST_CHECK_INT(error, VOE_BASE_ERROR_UNAVAILABLE);

#ifndef _WIN32
	// Home is whatever the operating system says it is, with no trailing
	// separator even when $HOME has one.
	{
		const char *home = getenv("HOME");

		if (home != NULL && home[0] != '\0') {
			const char *got = voe_platform_folder_home(arena);
			size_t length = strlen(home);

			if (length > 1 && home[length - 1] == '/')
				length--;
			VOE_TEST_CHECK(got != NULL);
			if (got != NULL) {
				VOE_TEST_CHECK_INT((long long)strlen(got),
						   (long long)length);
				VOE_TEST_CHECK(strncmp(got, home, length)
					      == 0);
			}
		}
	}

	// Settings honours XDG_CONFIG_HOME when it is set and absolute, and
	// falls back to $HOME/.config both when it is relative and when it is
	// unset. setenv/unsetenv are POSIX, hence the whole block is guarded.
	{
		const char *home = getenv("HOME");
		char expected[512] = { 0 };
		const char *got;

		if (home != NULL && home[0] != '\0')
			(void)snprintf(expected, sizeof expected, "%s/.config",
				       home);

		VOE_TEST_CHECK(setenv("XDG_CONFIG_HOME",
				      "/tmp/voe_platform_folder_test_xdg", 1)
			      == 0);
		got = voe_platform_folder_settings(arena);
		VOE_TEST_CHECK(got != NULL);
		if (got != NULL)
			VOE_TEST_CHECK(strcmp(got,
					      "/tmp/voe_platform_folder_test_xdg")
				      == 0);

		VOE_TEST_CHECK(setenv("XDG_CONFIG_HOME", "relative/path", 1)
			      == 0);
		got = voe_platform_folder_settings(arena);
		if (expected[0] != '\0') {
			VOE_TEST_CHECK(got != NULL);
			if (got != NULL)
				VOE_TEST_CHECK(strcmp(got, expected) == 0);
		}

		VOE_TEST_CHECK(unsetenv("XDG_CONFIG_HOME") == 0);
		got = voe_platform_folder_settings(arena);
		if (expected[0] != '\0') {
			VOE_TEST_CHECK(got != NULL);
			if (got != NULL)
				VOE_TEST_CHECK(strcmp(got, expected) == 0);
		}
	}
#endif

	voe_base_arena_destroy(arena);
	cleanup();
	return voe_test_result();
}
