// The Windows half of platform/file.h: CreateFileW, ReadFile/WriteFile,
// CloseHandle, and the MoveFileExW that makes a write atomic. Read the header
// first — what the path means and which failure is which is written there.
//
// A PATH IS UTF-8 AND IS CONVERTED ON THE WAY IN (ADR-0247, ADR-0248). Every
// call here is the "W" one, given the path through platform/src/wide_win32.h,
// so a name in any script opens the file it names.
//
// OPENING A FOLDER FOR READING FAILS ON ITS OWN HERE. CreateFileW without
// FILE_FLAG_BACKUP_SEMANTICS refuses a directory outright, so a read of one
// reports UNAVAILABLE straight out of the open — unlike Linux, which has to
// fstat afterwards to catch the same thing; see platform/src/file_wayland.c.
//
// CREATE_ALWAYS is create-or-truncate in one flag: the `.partial` sibling is
// made if it is not there and emptied if it somehow already is (a previous
// save that crashed before its rename). It also sets ERROR_ALREADY_EXISTS as a
// success code, which is why nothing here reads GetLastError except on a call
// that actually failed.
//
// THE READ AND WRITE LOOPS ARE NOT OPTIONAL. ReadFile and WriteFile may report
// success having moved fewer bytes than asked for. There is no EINTR on this
// platform, so a returned zero with no error is a stall and refused rather
// than retried forever.
//
// FlushFileBuffers AND CloseHandle ARE BOTH CHECKED for the reason the Linux
// file gives: on a network share or a full volume the error for data already
// accepted can surface only at flush or at close — exactly why the write goes
// to a `.partial` sibling first and path is untouched until MoveFileExW.
//
// DWORD IS 32 BITS AND size_t IS 64, so both loops move at most 1 GB a step;
// the cap costs one comparison rather than a size limit in the public header.
//
// THE PATH AND ITS PARTIAL ARE MAX_PATH WIDE STACK BUFFERS, not an arena: these
// functions take none for it. A path that does not fit fails like an open that
// fails — read UNAVAILABLE, exists false, write UNAVAILABLE — and the file
// calls without `\\?\` refuse a longer path anyway; see
// platform/src/wide_win32.h.
#include <platform/file.h>

#include "wide_win32.h"

#include <base/assert.h>
#include <base/report.h>

#include <windows.h>

#include <string.h>
#include <wchar.h>

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
	wchar_t wide_path[MAX_PATH];
	HANDLE file;
	LARGE_INTEGER size_info;
	size_t size;
	uint8_t *buffer;
	size_t got = 0;

	VOE_BASE_ASSERT(path != NULL, "reading a file with no path");
	VOE_BASE_ASSERT(arena != NULL, "reading a file into no arena");
	VOE_BASE_ASSERT(out_count != NULL, "reading a file with no count to fill");

	if (!voe_platform_wide_from_utf8(path, wide_path, MAX_PATH)) {
		VOE_BASE_ERROR("platform", "could not open %s for reading: the path is too long",
			       path);
		report(error, VOE_BASE_ERROR_UNAVAILABLE);
		return NULL;
	}

	file = CreateFileW(wide_path, GENERIC_READ, FILE_SHARE_READ, NULL,
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
	wchar_t wide_path[MAX_PATH];
	DWORD attributes;

	VOE_BASE_ASSERT(path != NULL, "checking existence of no path");

	if (!voe_platform_wide_from_utf8(path, wide_path, MAX_PATH))
		return false;
	attributes = GetFileAttributesW(wide_path);
	if (attributes == INVALID_FILE_ATTRIBUTES)
		return false;
	return (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

bool voe_platform_file_write(const char *path, const uint8_t *bytes,
			     size_t count, voe_base_error *error)
{
	static const wchar_t suffix[] = L".partial";
	wchar_t wide_path[MAX_PATH];
	wchar_t partial[MAX_PATH];
	HANDLE file;
	size_t written = 0;

	VOE_BASE_ASSERT(path != NULL, "writing a file with no path");
	VOE_BASE_ASSERT(bytes != NULL, "writing a file from no bytes");
	VOE_BASE_ASSERT(count > 0, "writing a file of no bytes");

	// A path whose .partial sibling does not fit fails as the create of
	// that sibling would; see the header comment.
	if (!voe_platform_wide_from_utf8(path, wide_path, MAX_PATH) ||
	    wcslen(wide_path) + wcslen(suffix) >= MAX_PATH) {
		VOE_BASE_ERROR("platform", "could not open %s.partial for writing: the path is too long",
			       path);
		report(error, VOE_BASE_ERROR_UNAVAILABLE);
		return false;
	}
	// memcpy, not wcscpy/wcscat, which the MSVC C runtime deprecates.
	(void)memcpy(partial, wide_path, wcslen(wide_path) * sizeof *partial);
	(void)memcpy(partial + wcslen(wide_path), suffix, sizeof suffix);

	file = CreateFileW(partial, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
			   FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		VOE_BASE_ERROR("platform",
			       "could not open %s.partial for writing: error %lu",
			       path, (unsigned long)GetLastError());
		report(error, VOE_BASE_ERROR_UNAVAILABLE);
		return false;
	}

	while (written < count) {
		size_t left = count - written;
		DWORD step = left > 0x40000000u ? 0x40000000u : (DWORD)left;
		DWORD done = 0;

		if (!WriteFile(file, bytes + written, step, &done, NULL)) {
			VOE_BASE_ERROR("platform",
				       "writing %s.partial failed after %zu of %zu bytes: error %lu",
				       path, written, count,
				       (unsigned long)GetLastError());
			report(error, VOE_BASE_ERROR_REFUSED);
			(void)CloseHandle(file);
			(void)DeleteFileW(partial);
			return false;
		}
		if (done == 0) {
			VOE_BASE_ERROR("platform",
				       "writing %s.partial stalled after %zu of %zu bytes",
				       path, written, count);
			report(error, VOE_BASE_ERROR_REFUSED);
			(void)CloseHandle(file);
			(void)DeleteFileW(partial);
			return false;
		}
		written += done;
	}

	if (!FlushFileBuffers(file)) {
		VOE_BASE_ERROR("platform", "flushing %s.partial failed: error %lu",
			       path, (unsigned long)GetLastError());
		report(error, VOE_BASE_ERROR_REFUSED);
		(void)CloseHandle(file);
		(void)DeleteFileW(partial);
		return false;
	}

	if (!CloseHandle(file)) {
		VOE_BASE_ERROR("platform", "closing %s.partial failed: error %lu",
			       path, (unsigned long)GetLastError());
		report(error, VOE_BASE_ERROR_REFUSED);
		(void)DeleteFileW(partial);
		return false;
	}

	if (!MoveFileExW(partial, wide_path,
			 MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
		VOE_BASE_ERROR("platform", "replacing %s with %s.partial failed: error %lu",
			       path, path, (unsigned long)GetLastError());
		report(error, VOE_BASE_ERROR_REFUSED);
		(void)DeleteFileW(partial);
		return false;
	}

	report(error, VOE_BASE_OK);
	return true;
}
