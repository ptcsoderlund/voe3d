// What platform/file.h promises, checked from outside: the bytes written come
// back byte for byte on both sides of the API, a file is replaced rather than
// overwritten in place, a crash before the final rename leaves the old file
// whole, and a path that cannot be opened fails without creating anything.
// Needs no window and no display, so it runs under ctest on a machine with
// neither.
//
// PLATFORM IS ALLOWED OS HEADERS EVERYWHERE IN IT, TESTS INCLUDED
// (check.cmake step 5), so this file makes and removes a scratch folder with
// mkdir/rmdir directly rather than through platform/folder.h, which does not
// exist yet at this task (it is a later one in card 004). A test file this
// folder writes is not "reading a file in pieces" (rule 10's reason
// platform/file.h itself has no such API) — it is proving the promise from
// outside with whatever the platform hands out.
//
// voe_platform_file_write IS VERIFIED WITH stdio's fopen/fread AS THE ORACLE,
// AND NOT THIS FOLDER'S OWN READER. A matching pair of bugs in this folder's
// write and read would otherwise cancel out and pass. voe_platform_file_read
// is checked the other way round: against bytes an independent write (also
// stdio) put on disk. Only the round trip at the very top uses both of this
// folder's own functions together, as a caller actually would.
//
// THE REPLACEMENT CASE IS THE ONE WORTH HAVING. A writer that opens without
// truncating leaves the tail of a longer previous file behind, and the
// contents still start with the right bytes — so only the length catches it,
// and only when the second write is the shorter of the two.
//
// Every file is written into the working directory, which ctest sets to the
// build tree, and removed at the end whether the checks passed or not. The
// failing cases print a reported line to stderr on purpose; that is the call
// saying why, not the test going wrong.
//
// A NON-ASCII NAME IS ITS OWN CASE (ADR-0247): a file named in three scripts
// inside a folder named in three, made, checked and read back with platform
// calls only. Its cleanup on Windows goes through _wremove/_wrmdir, since the
// narrow C runtime reads a name in the legacy code page, not UTF-8.
#include <platform/file.h>

#include <base/arena.h>
#include <platform/folder.h>
#include <testing/test.h>

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

#define WRITTEN "voe_platform_file_test.bin"
#define WRITTEN_PARTIAL "voe_platform_file_test.bin.partial"
#define EMPTY "voe_platform_file_test_empty.bin"
#define MISSING "voe_platform_file_test_no_such_directory/file.bin"
#define FOLDER "voe_platform_file_test_folder"
#define FOLDER_PARTIAL "voe_platform_file_test_folder.partial"
#define WORLD "Åsa 李 värld"
#define WORLD_SCENE WORLD "/Min scen ÅÄÖ 李.txt"

// Reads the whole file into buffer and hands back its length, or -1 if it is
// not there. room is the size of buffer; a file that does not fit reads as
// longer than it is, which fails the length check rather than overrunning.
static long read_back(const char *path, unsigned char *buffer, size_t room)
{
	// The MSVC C runtime deprecates fopen (it names fopen_s, Annex K, as the
	// replacement; Linux has none) and -Werror turns that into a build error.
	// This is tests/, never src/ — ADR-0159 — so the call is suppressed here
	// rather than the oracle changed.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	FILE *file = fopen(path, "rb");
#pragma clang diagnostic pop
	size_t count;

	if (file == NULL)
		return -1;

	count = fread(buffer, 1, room, file);
	fclose(file);
	return (long)count;
}

// Writes count bytes to path with stdio, the oracle for voe_platform_file_read
// — see the header comment. count may be 0, which read/write() itself refuses
// by assertion (rule 13: an empty write is a call-site bug), but stdio has no
// such rule and an empty file is exactly what the "empty file" case needs.
static bool write_oracle(const char *path, const unsigned char *bytes,
			 size_t count)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	FILE *file = fopen(path, "wb");
#pragma clang diagnostic pop
	size_t written;

	if (file == NULL)
		return false;
	written = count > 0 ? fwrite(bytes, 1, count, file) : 0;
	fclose(file);
	return written == count;
}

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

static bool is_folder(const char *path)
{
#ifdef _WIN32
	DWORD attributes = GetFileAttributesA(path);

	if (attributes == INVALID_FILE_ATTRIBUTES)
		return false;
	return (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
	struct stat info;

	if (stat(path, &info) != 0)
		return false;
	return S_ISDIR(info.st_mode);
#endif
}

int main(void)
{
	// Deliberately includes a zero byte and a 0xff, so a writer that stopped
	// at a terminator or sign-extended a byte shows up here.
	static const uint8_t payload[] = { 0x00, 0x01, 0x7f, 0x80, 0xff,
					   0x41, 0x00, 0xfe };
	static const uint8_t shorter[] = { 0xab, 0xcd };
	static const uint8_t single[] = { 0x5a };
	unsigned char got[64];
	voe_base_error error = VOE_BASE_OK;
	long length;
	voe_base_arena *arena = voe_base_arena_new(4096);

	// The bytes that went in come back, on both sides of the API: stdio
	// reads back what voe_platform_file_write wrote, and
	// voe_platform_file_read reads back the same file the same way.
	VOE_TEST_CHECK(voe_platform_file_write(WRITTEN, payload,
					       sizeof payload, &error));
	VOE_TEST_CHECK_INT(error, VOE_BASE_OK);
	length = read_back(WRITTEN, got, sizeof got);
	VOE_TEST_CHECK_INT(length, (long)sizeof payload);
	if (length == (long)sizeof payload) {
		for (size_t i = 0; i < sizeof payload; i++)
			VOE_TEST_CHECK_INT(got[i], payload[i]);
	}
	{
		size_t count = 0;
		const uint8_t *read = voe_platform_file_read(WRITTEN, arena,
							      &count, &error);

		VOE_TEST_CHECK(read != NULL);
		VOE_TEST_CHECK_INT(error, VOE_BASE_OK);
		VOE_TEST_CHECK_INT((long long)count, (long long)sizeof payload);
		if (read != NULL && count == sizeof payload) {
			for (size_t i = 0; i < sizeof payload; i++)
				VOE_TEST_CHECK_INT(read[i], payload[i]);
			VOE_TEST_CHECK_INT(read[count], 0);
		}
	}

	// No .partial sibling outlives a successful write.
	VOE_TEST_CHECK(!voe_platform_file_exists(WRITTEN_PARTIAL));

	// A shorter file over a longer one is the shorter one, tail and all.
	VOE_TEST_CHECK(voe_platform_file_write(WRITTEN, shorter,
					       sizeof shorter, &error));
	length = read_back(WRITTEN, got, sizeof got);
	VOE_TEST_CHECK_INT(length, (long)sizeof shorter);
	if (length == (long)sizeof shorter) {
		VOE_TEST_CHECK_INT(got[0], shorter[0]);
		VOE_TEST_CHECK_INT(got[1], shorter[1]);
	}

	// One byte is a file too.
	VOE_TEST_CHECK(voe_platform_file_write(WRITTEN, single, sizeof single,
					       &error));
	length = read_back(WRITTEN, got, sizeof got);
	VOE_TEST_CHECK_INT(length, 1);
	if (length == 1)
		VOE_TEST_CHECK_INT(got[0], single[0]);

	// An empty file is a valid read: a non-NULL pointer and a count of 0.
	VOE_TEST_CHECK(write_oracle(EMPTY, payload, 0));
	{
		size_t count = 123;
		const uint8_t *read = voe_platform_file_read(EMPTY, arena, &count,
							      &error);

		VOE_TEST_CHECK(read != NULL);
		VOE_TEST_CHECK_INT(error, VOE_BASE_OK);
		VOE_TEST_CHECK_INT((long long)count, 0);
		if (read != NULL)
			VOE_TEST_CHECK_INT(read[0], 0);
	}

	// A missing path reads as UNAVAILABLE, not REFUSED.
	error = VOE_BASE_OK;
	{
		size_t count = 0;

		VOE_TEST_CHECK(voe_platform_file_read("voe_platform_file_test_missing.bin",
						      arena, &count, &error)
			      == NULL);
		VOE_TEST_CHECK_INT(error, VOE_BASE_ERROR_UNAVAILABLE);
	}

	// A folder cannot be read as a file.
	VOE_TEST_CHECK(make_folder(FOLDER));
	error = VOE_BASE_OK;
	{
		size_t count = 0;

		VOE_TEST_CHECK(voe_platform_file_read(FOLDER, arena, &count, &error)
			      == NULL);
		VOE_TEST_CHECK_INT(error, VOE_BASE_ERROR_UNAVAILABLE);
	}

	// A directory that is not there is UNAVAILABLE, and nothing is created:
	// neither the file nor the directory it was named in.
	error = VOE_BASE_OK;
	VOE_TEST_CHECK(!voe_platform_file_write(MISSING, payload,
						sizeof payload, &error));
	VOE_TEST_CHECK_INT(error, VOE_BASE_ERROR_UNAVAILABLE);
	VOE_TEST_CHECK_INT(read_back(MISSING, got, sizeof got), -1);

	// Writing to a path that is itself an existing folder: the rename over
	// it fails, the write is REFUSED, the folder is untouched and no
	// .partial is left beside it.
	error = VOE_BASE_OK;
	VOE_TEST_CHECK(!voe_platform_file_write(FOLDER, payload, sizeof payload,
						&error));
	VOE_TEST_CHECK_INT(error, VOE_BASE_ERROR_REFUSED);
	VOE_TEST_CHECK(is_folder(FOLDER));
	VOE_TEST_CHECK(!voe_platform_file_exists(FOLDER_PARTIAL));

	// voe_platform_file_exists: true for a regular file, false for a
	// folder and for a path that is not there.
	VOE_TEST_CHECK(voe_platform_file_exists(WRITTEN));
	VOE_TEST_CHECK(!voe_platform_file_exists(FOLDER));
	VOE_TEST_CHECK(!voe_platform_file_exists("voe_platform_file_test_missing.bin"));

	// The out-parameter is optional, and a caller that does not want to know
	// which way still gets the answer in the return value.
	VOE_TEST_CHECK(voe_platform_file_write(WRITTEN, single, sizeof single,
					       NULL));
	VOE_TEST_CHECK(!voe_platform_file_write(MISSING, single, sizeof single,
						NULL));
	{
		size_t count = 0;

		VOE_TEST_CHECK(voe_platform_file_read(WRITTEN, arena, &count, NULL)
			      != NULL);
	}

	// A name in any script is written, found and read back like any other.
	remove_utf8(WORLD_SCENE, false);
	remove_utf8(WORLD, true);
	VOE_TEST_CHECK(voe_platform_folder_create(WORLD, NULL));
	VOE_TEST_CHECK(voe_platform_file_write(WORLD_SCENE, payload,
					       sizeof payload, &error));
	VOE_TEST_CHECK(voe_platform_file_exists(WORLD_SCENE));
	{
		size_t count = 0;
		const uint8_t *read = voe_platform_file_read(WORLD_SCENE, arena,
							      &count, &error);

		VOE_TEST_CHECK(read != NULL);
		VOE_TEST_CHECK_INT((long long)count, (long long)sizeof payload);
		if (read != NULL && count == sizeof payload)
			VOE_TEST_CHECK(memcmp(read, payload, count) == 0);
	}

	voe_base_arena_destroy(arena);
	remove_utf8(WORLD_SCENE, false);
	remove_utf8(WORLD, true);
	remove(WRITTEN);
	remove(WRITTEN_PARTIAL);
	remove(EMPTY);
	remove(MISSING);
	remove_folder(FOLDER);
	remove_folder(FOLDER_PARTIAL);
	return voe_test_result();
}
