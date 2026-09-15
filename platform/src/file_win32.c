// The Windows half of platform/file.h: CreateFileA, ReadFile/WriteFile,
// CloseHandle, and the MoveFileExA that makes a write atomic. Read the header
// first — what the path means and which failure is which is written there.
//
// CreateFileA AND NOT CreateFileW, for the same reason as LoadLibraryA in
// platform/src/library_win32.c: a path here is a name the engine or its caller
// spelled in ASCII. A path a person typed in another script is a different
// question and will arrive through a different function when something asks.
//
// OPENING A FOLDER FOR READING FAILS ON ITS OWN HERE. CreateFileA without
// FILE_FLAG_BACKUP_SEMANTICS refuses a directory outright, so a read of one
// reports UNAVAILABLE straight out of the open — unlike Linux, which has to
// fstat afterwards to catch the same thing; see platform/src/file_wayland.c.
//
// CREATE_ALWAYS is create-or-truncate in one flag: the `.partial` sibling is
// made if it is not there and emptied if it somehow already is (a previous
// save that crashed before its rename), which is what "write a whole file"
// means. It also sets ERROR_ALREADY_EXISTS as a success code, which is why
// nothing here reads GetLastError except on a call that actually failed.
//
// THE READ AND WRITE LOOPS ARE NOT OPTIONAL. ReadFile and WriteFile may report
// success having moved fewer bytes than asked for, and a caller that treats
// the first call as done gets a short read or a truncated write. There is no
// EINTR on this platform, so a returned zero with no error is treated as a
// stall and refused rather than retried forever.
//
// FlushFileBuffers AND CloseHandle ARE BOTH CHECKED for the reason the Linux
// file gives: on a network share or a full volume the error for data already
// accepted can surface only at flush or at close, and a writer that ignores
// either reports success for a file that never landed — exactly why the write
// goes to a `.partial` sibling first and path itself is untouched until the
// final MoveFileExA.
//
// DWORD IS 32 BITS AND size_t IS 64, so a buffer larger than 4 GB cannot be
// handed to ReadFile or WriteFile in one go. Both loops write in steps, so the
// step is simply capped; nothing this engine reads or writes is anywhere near
// that, and the cap costs one comparison rather than a size limit in the
// public header.
//
// THE PARTIAL PATH IS BUILT ON THE STACK, NOT AN ARENA, for the same reason
// platform/src/window_win32.c converts a window title on the stack: this
// function takes no arena and never has. A path longer than PARTIAL_PATH_MAX
// is a call-site mistake, not a runtime condition, and asserts rather than
// returning a failure a caller would have to plan for.
#include <platform/file.h>

#include <base/assert.h>
#include <base/report.h>

#include <windows.h>

#include <stdio.h>
#include <string.h>

// Long enough for any path this engine constructs plus ".partial"; see the
// header comment above for why this is a stack buffer.
#define PARTIAL_PATH_MAX 4096

// The out-parameter is optional (rule 13), so every path sets it through here
// rather than repeating the check four times.
static void report(voe_base_error *error, voe_base_error code)
{
	if (error != NULL)
		*error = code;
}

const uint8_t *voe_platform_file_read(const char *path, voe_base_arena *arena,
				      size_t *out_count, voe_base_error *error)
{
	HANDLE file;
	LARGE_INTEGER size_info;
	size_t size;
	uint8_t *buffer;
	size_t got = 0;

	VOE_BASE_ASSERT(path != NULL, "reading a file with no path");
	VOE_BASE_ASSERT(arena != NULL, "reading a file into no arena");
	VOE_BASE_ASSERT(out_count != NULL, "reading a file with no count to fill");

	file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
			   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		VOE_BASE_ERROR("platform",
			       "could not open %s for reading: error %lu", path,
			       (unsigned long)GetLastError());
		report(error, VOE_BASE_ERROR_UNAVAILABLE);
		return NULL;
	}

	if (!GetFileSizeEx(file, &size_info)) {
		VOE_BASE_ERROR("platform", "could not read %s's size: error %lu",
			       path, (unsigned long)GetLastError());
		report(error, VOE_BASE_ERROR_UNAVAILABLE);
		(void)CloseHandle(file);
		return NULL;
	}

	size = (size_t)size_info.QuadPart;
	// +1 for the NUL the header promises past the last byte; arena_push
	// zeroes what it hands back, so an empty file is already terminated.
	buffer = voe_base_arena_push(arena, size + 1);

	while (got < size) {
		size_t left = size - got;
		DWORD step = left > 0x40000000u ? 0x40000000u : (DWORD)left;
		DWORD done = 0;

		if (!ReadFile(file, buffer + got, step, &done, NULL)) {
			VOE_BASE_ERROR("platform",
				       "reading %s failed after %zu of %zu bytes: error %lu",
				       path, got, size,
				       (unsigned long)GetLastError());
			report(error, VOE_BASE_ERROR_REFUSED);
			(void)CloseHandle(file);
			return NULL;
		}
		if (done == 0) {
			VOE_BASE_ERROR("platform",
				       "reading %s stopped after %zu of %zu bytes",
				       path, got, size);
			report(error, VOE_BASE_ERROR_REFUSED);
			(void)CloseHandle(file);
			return NULL;
		}
		got += done;
	}

	// Nothing was buffered on our side for a read-only handle, so
	// CloseHandle's result cannot tell us anything the loop above has
	// not; see the header comment on why a write's close is checked and
	// a read's is not.
	(void)CloseHandle(file);

	*out_count = size;
	report(error, VOE_BASE_OK);
	return buffer;
}

bool voe_platform_file_exists(const char *path)
{
	DWORD attributes;

	VOE_BASE_ASSERT(path != NULL, "checking existence of no path");

	attributes = GetFileAttributesA(path);
	if (attributes == INVALID_FILE_ATTRIBUTES)
		return false;
	return (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

bool voe_platform_file_write(const char *path, const uint8_t *bytes,
			     size_t count, voe_base_error *error)
{
	char partial[PARTIAL_PATH_MAX];
	HANDLE file;
	size_t written = 0;

	VOE_BASE_ASSERT(path != NULL, "writing a file with no path");
	VOE_BASE_ASSERT(bytes != NULL, "writing a file from no bytes");
	VOE_BASE_ASSERT(count > 0, "writing a file of no bytes");
	VOE_BASE_ASSERT(strlen(path) + strlen(".partial") < sizeof partial,
			"path is too long for voe_platform_file_write");
	(void)snprintf(partial, sizeof partial, "%s.partial", path);

	file = CreateFileA(partial, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
			   FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		VOE_BASE_ERROR("platform",
			       "could not open %s for writing: error %lu",
			       partial, (unsigned long)GetLastError());
		report(error, VOE_BASE_ERROR_UNAVAILABLE);
		return false;
	}

	while (written < count) {
		size_t left = count - written;
		DWORD step = left > 0x40000000u ? 0x40000000u : (DWORD)left;
		DWORD done = 0;

		if (!WriteFile(file, bytes + written, step, &done, NULL)) {
			VOE_BASE_ERROR("platform",
				       "writing %s failed after %zu of %zu bytes: error %lu",
				       partial, written, count,
				       (unsigned long)GetLastError());
			report(error, VOE_BASE_ERROR_REFUSED);
			(void)CloseHandle(file);
			(void)DeleteFileA(partial);
			return false;
		}
		if (done == 0) {
			VOE_BASE_ERROR("platform",
				       "writing %s stalled after %zu of %zu bytes",
				       partial, written, count);
			report(error, VOE_BASE_ERROR_REFUSED);
			(void)CloseHandle(file);
			(void)DeleteFileA(partial);
			return false;
		}
		written += done;
	}

	if (!FlushFileBuffers(file)) {
		VOE_BASE_ERROR("platform", "flushing %s failed: error %lu",
			       partial, (unsigned long)GetLastError());
		report(error, VOE_BASE_ERROR_REFUSED);
		(void)CloseHandle(file);
		(void)DeleteFileA(partial);
		return false;
	}

	if (!CloseHandle(file)) {
		VOE_BASE_ERROR("platform", "closing %s failed: error %lu",
			       partial, (unsigned long)GetLastError());
		report(error, VOE_BASE_ERROR_REFUSED);
		(void)DeleteFileA(partial);
		return false;
	}

	if (!MoveFileExA(partial, path,
			 MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
		VOE_BASE_ERROR("platform", "replacing %s with %s failed: error %lu",
			       path, partial, (unsigned long)GetLastError());
		report(error, VOE_BASE_ERROR_REFUSED);
		(void)DeleteFileA(partial);
		return false;
	}

	report(error, VOE_BASE_OK);
	return true;
}
