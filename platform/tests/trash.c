// What platform/trash.h promises, checked from outside: a file goes to
// files/ with an info/ entry naming its percent-encoded path, a folder goes
// whole, a taken name is numbered, and nothing at the path is UNAVAILABLE.
//
// ON WINDOWS ONLY THE MISSING PATH IS CHECKED: everything that succeeds there
// would fill the person's real Recycle Bin (ADR-0382 point 4), so the
// freedesktop checks and their scratch setup are Linux's, inside #ifndef _WIN32.
//
// ON LINUX XDG_DATA_HOME IS POINTED AT A SCRATCH FOLDER under the test's own working
// folder, so the person's real trash is never touched and the trash is on the
// same file system as the files sent to it. The scratch tree is torn down by
// name with remove/rmdir, before and after, as platform/tests/folder.c does.
#define _POSIX_C_SOURCE 200809L

#include <platform/trash.h>

#include <base/arena.h>
#include <testing/test.h>

#define WORK "voe_platform_trash_test_work"

#ifndef _WIN32
#include <platform/file.h>

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define DATA "voe_platform_trash_test_data"
#define FILES DATA "/Trash/files"
#define INFO DATA "/Trash/info"

static const uint8_t byte[] = { 'x' };

static bool exists(const char *path)
{
	struct stat info;

	return stat(path, &info) == 0;
}

static void cleanup(void)
{
	static const char *const files[] = {
		WORK "/a file.txt", WORK "/box/inner.txt", WORK "/twice",
		FILES "/a file.txt", FILES "/box/inner.txt", FILES "/twice",
		FILES "/twice.2", INFO "/a file.txt.trashinfo",
		INFO "/box.trashinfo", INFO "/twice.trashinfo",
		INFO "/twice.2.trashinfo",
	};
	static const char *const folders[] = {
		WORK "/box", WORK, FILES "/box", FILES, INFO, DATA "/Trash",
		DATA,
	};

	for (size_t i = 0; i < sizeof files / sizeof files[0]; i++)
		(void)remove(files[i]);
	for (size_t i = 0; i < sizeof folders / sizeof folders[0]; i++)
		(void)rmdir(folders[i]);
}

static void trash_moves_a_file_with_its_info(voe_base_arena *arena)
{
	voe_base_error error = VOE_BASE_ERROR_UNAVAILABLE;
	size_t count = 0;
	const char *info;

	VOE_TEST_CHECK(voe_platform_file_write(WORK "/a file.txt", byte,
					       sizeof byte, NULL));
	VOE_TEST_CHECK(voe_platform_trash(WORK "/a file.txt", arena, &error));
	VOE_TEST_CHECK_INT(error, VOE_BASE_OK);
	VOE_TEST_CHECK(!exists(WORK "/a file.txt"));
	VOE_TEST_CHECK(exists(FILES "/a file.txt"));

	info = (const char *)voe_platform_file_read(
		INFO "/a file.txt.trashinfo", arena, &count, NULL);
	VOE_TEST_CHECK(info != NULL);
	if (info == NULL)
		return;
	VOE_TEST_CHECK(strncmp(info, "[Trash Info]\nPath=/", 19) == 0);
	VOE_TEST_CHECK(strstr(info, "/" WORK "/a%20file.txt\nDeletionDate=")
		       != NULL);
}

static void trash_moves_a_folder_whole(voe_base_arena *arena)
{
	voe_base_error error = VOE_BASE_ERROR_UNAVAILABLE;

	VOE_TEST_CHECK(mkdir(WORK "/box", 0755) == 0);
	VOE_TEST_CHECK(voe_platform_file_write(WORK "/box/inner.txt", byte,
					       sizeof byte, NULL));
	VOE_TEST_CHECK(voe_platform_trash(WORK "/box", arena, &error));
	VOE_TEST_CHECK_INT(error, VOE_BASE_OK);
	VOE_TEST_CHECK(!exists(WORK "/box"));
	VOE_TEST_CHECK(exists(FILES "/box/inner.txt"));
	VOE_TEST_CHECK(exists(INFO "/box.trashinfo"));
}

static void trash_numbers_a_taken_name(voe_base_arena *arena)
{
	for (int i = 0; i < 2; i++) {
		VOE_TEST_CHECK(voe_platform_file_write(WORK "/twice", byte,
						       sizeof byte, NULL));
		VOE_TEST_CHECK(voe_platform_trash(WORK "/twice", arena, NULL));
	}
	VOE_TEST_CHECK(exists(FILES "/twice"));
	VOE_TEST_CHECK(exists(FILES "/twice.2"));
	VOE_TEST_CHECK(exists(INFO "/twice.trashinfo"));
	VOE_TEST_CHECK(exists(INFO "/twice.2.trashinfo"));
}
#endif

static void trash_of_nothing_is_unavailable(voe_base_arena *arena)
{
	voe_base_error error = VOE_BASE_OK;

	VOE_TEST_CHECK(!voe_platform_trash(WORK "/missing", arena, &error));
	VOE_TEST_CHECK_INT(error, VOE_BASE_ERROR_UNAVAILABLE);
#ifndef _WIN32
	VOE_TEST_CHECK(!exists(INFO "/missing.trashinfo"));
#endif
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(4096);
#ifndef _WIN32
	char data[PATH_MAX];

	cleanup();
	VOE_TEST_CHECK(getcwd(data, sizeof data - sizeof DATA - 1) != NULL);
	(void)strcat(data, "/" DATA);
	VOE_TEST_CHECK(setenv("XDG_DATA_HOME", data, 1) == 0);
	VOE_TEST_CHECK(mkdir(WORK, 0755) == 0);

	trash_moves_a_file_with_its_info(arena);
	trash_moves_a_folder_whole(arena);
	trash_numbers_a_taken_name(arena);
#endif
	trash_of_nothing_is_unavailable(arena);

	voe_base_arena_destroy(arena);
#ifndef _WIN32
	cleanup();
#endif
	return voe_test_result();
}
