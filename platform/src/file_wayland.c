// The Linux half of platform/file.h: open, write, close. Read the header first —
// what the path means and which failure is which is written there.
//
// LINUX AND NOT WAYLAND, DESPITE THE NAME, AND THE NAME IS THE BUILD'S. A source
// whose name ends in _wayland is compiled on Linux only and nowhere else, which
// is the whole of what this file needs; there is no _linux suffix in
// cmake/voe.cmake and inventing one to be tidy would put a second rule in the
// build for the sake of one file. platform/src/library_wayland.c and
// platform/src/clock_wayland.c carry the same name for the same reason. Nothing
// in here knows there is a compositor.
//
// _POSIX_C_SOURCE is what makes <fcntl.h> and <unistd.h> declare open, write and
// close under -std=c23, which the engine builds with. It is a feature-test macro
// and not a use of any extension; 200809L is the revision this engine assumes.
//
// 0644 is the mode a new file is created with — readable by anyone, writable by
// its owner — and the process umask narrows it further. It is what an ordinary
// data file gets; nothing here is executable and nothing here is a secret.
//
// THE WRITE LOOP IS NOT OPTIONAL AND NEITHER HALF OF IT IS. write() may return
// having written fewer bytes than it was given — on a pipe, on a full-ish disk,
// on a large buffer — and a caller that treats a short write as done truncates
// the file silently. It may also return -1 with EINTR when a signal arrived
// before anything was written, which is not a failure at all and must be
// retried. Everything else is a real failure.
//
// close() IS CHECKED, WHICH LOOKS LIKE SUPERSTITION AND IS NOT. Data written
// into the kernel's page cache can fail to reach the disk after the last write()
// has already returned success, and on NFS and on a full filesystem the error
// surfaces at close. A writer that ignores close is a writer that reports
// success for a file that never landed.
#define _POSIX_C_SOURCE 200809L

#include <platform/file.h>

#include <base/assert.h>
#include <base/report.h>

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

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
	int file;
	size_t written = 0;

	VOE_BASE_ASSERT(path != NULL, "writing a file with no path");
	VOE_BASE_ASSERT(bytes != NULL, "writing a file from no bytes");
	VOE_BASE_ASSERT(count > 0, "writing a file of no bytes");

	file = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (file < 0) {
		VOE_BASE_ERROR("platform", "could not open %s for writing: %s",
			       path, strerror(errno));
		report(error, VOE_BASE_ERROR_UNAVAILABLE);
		return false;
	}

	while (written < count) {
		ssize_t step = write(file, bytes + written, count - written);

		if (step < 0) {
			if (errno == EINTR)
				continue;
			VOE_BASE_ERROR("platform",
				       "writing %s failed after %zu of %zu bytes: %s",
				       path, written, count, strerror(errno));
			report(error, VOE_BASE_ERROR_REFUSED);
			// The file is open and part-written; closing it is the
			// only thing left to do with it, and its result cannot
			// tell us anything the failure above has not.
			(void)close(file);
			return false;
		}
		written += (size_t)step;
	}

	if (close(file) != 0) {
		VOE_BASE_ERROR("platform", "closing %s failed: %s", path,
			       strerror(errno));
		report(error, VOE_BASE_ERROR_REFUSED);
		return false;
	}

	report(error, VOE_BASE_OK);
	return true;
}
