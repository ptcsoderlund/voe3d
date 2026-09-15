// The Windows half of platform/file.h: CreateFileA, WriteFile, CloseHandle.
// Read the header first — what the path means and which failure is which is
// written there.
//
// CreateFileA AND NOT CreateFileW, for the same reason as LoadLibraryA in
// platform/src/library_win32.c: a path here is a name the engine or its caller
// spelled in ASCII. A path a person typed in another script is a different
// question and will arrive through a different function when something asks.
//
// CREATE_ALWAYS is create-or-truncate in one flag: the file is made if it is not
// there and emptied if it is, which is what "write a whole file" means. It also
// sets ERROR_ALREADY_EXISTS as a success code, which is why nothing here reads
// GetLastError except on a call that actually failed.
//
// THE WRITE LOOP IS NOT OPTIONAL. WriteFile may report success having written
// fewer bytes than it was given, and a caller that treats the first call as done
// truncates the file silently. There is no EINTR on this platform, so a returned
// zero is the only failure to look at — but the short-write loop is the same
// loop as Linux's and for the same reason. A call that succeeds having written
// nothing at all would be a loop that never ends, so it is refused rather than
// retried: there is no signal to have interrupted it and nothing would change.
//
// CloseHandle IS CHECKED for the reason the Linux file gives: on a network share
// or a full volume the error for data already accepted can surface only when the
// handle is closed, and a writer that ignores it reports success for a file that
// never landed.
//
// DWORD IS 32 BITS AND size_t IS 64, so a buffer larger than 4 GB cannot be
// handed to WriteFile in one go. The loop already writes in steps, so the step
// is simply capped; nothing this engine writes is anywhere near that, and the
// cap costs one comparison rather than a size limit in the public header.
#include <platform/file.h>

#include <base/assert.h>
#include <base/report.h>

#include <windows.h>

// The out-parameter is optional (rule 13), so every path sets it through here
// rather than repeating the check four times.
static void report(voe_base_error *error, voe_base_error code)
{
	if (error != NULL)
		*error = code;
}

bool voe_platform_file_write(const char *path, const uint8_t *bytes,
			     size_t count, voe_base_error *error)
{
	HANDLE file;
	size_t written = 0;

	VOE_BASE_ASSERT(path != NULL, "writing a file with no path");
	VOE_BASE_ASSERT(bytes != NULL, "writing a file from no bytes");
	VOE_BASE_ASSERT(count > 0, "writing a file of no bytes");

	file = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
			   FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		VOE_BASE_ERROR("platform",
			       "could not open %s for writing: error %lu", path,
			       (unsigned long)GetLastError());
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
				       path, written, count,
				       (unsigned long)GetLastError());
			report(error, VOE_BASE_ERROR_REFUSED);
			// The handle is open and the file part-written; closing
			// it is the only thing left to do with it, and its
			// result cannot tell us anything the failure above has
			// not.
			(void)CloseHandle(file);
			return false;
		}
		if (done == 0) {
			VOE_BASE_ERROR("platform",
				       "writing %s stalled after %zu of %zu bytes",
				       path, written, count);
			report(error, VOE_BASE_ERROR_REFUSED);
			(void)CloseHandle(file);
			return false;
		}
		written += done;
	}

	if (!CloseHandle(file)) {
		VOE_BASE_ERROR("platform", "closing %s failed: error %lu", path,
			       (unsigned long)GetLastError());
		report(error, VOE_BASE_ERROR_REFUSED);
		return false;
	}

	report(error, VOE_BASE_OK);
	return true;
}
