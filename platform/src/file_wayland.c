// The Linux half of platform/file.h: open, read/write, close, and the
// rename that makes a write atomic. Read the header first — what the path
// means and which failure is which is written there.
//
// LINUX AND NOT WAYLAND, DESPITE THE NAME, AND THE NAME IS THE BUILD'S. A source
// whose name ends in _wayland is compiled on Linux only and nowhere else, which
// is the whole of what this file needs; there is no _linux suffix in
// cmake/voe.cmake and inventing one to be tidy would put a second rule in the
// build for the sake of one file. platform/src/library_wayland.c and
// platform/src/clock_wayland.c carry the same name for the same reason. Nothing
// in here knows there is a compositor.
//
// _POSIX_C_SOURCE is what makes <fcntl.h> and <unistd.h> declare open, read,
// write and close under -std=c23, which the engine builds with. It is a
// feature-test macro and not a use of any extension; 200809L is the revision
// this engine assumes.
//
// 0644 is the mode a new file is created with — readable by anyone, writable by
// its owner — and the process umask narrows it further. It is what an ordinary
// data file gets; nothing here is executable and nothing here is a secret.
//
// READING TRUSTS fstat's SIZE AND THEN CHECKS IT. The size at fstat time is the
// number of bytes pushed into the arena and the number the read loop tries to
// fill; a file that shrinks under us between the fstat and the last read is a
// race no caller of this engine can hit in practice (nothing else writes a
// project's own files while it is open), and is reported as REFUSED rather than
// silently handed back short. Opening a folder for reading succeeds on Linux —
// it is the fstat right after that catches it and reports UNAVAILABLE, closing
// the descriptor first.
//
// THE WRITE LOOP IS NOT OPTIONAL AND NEITHER HALF OF IT IS. write() may return
// having written fewer bytes than it was given — on a pipe, on a full-ish disk,
// on a large buffer — and a caller that treats a short write as done truncates
// the file silently. It may also return -1 with EINTR when a signal arrived
// before anything was written, which is not a failure at all and must be
// retried. Everything else is a real failure. read() gets the same loop for the
// same reason.
//
// fsync AND close ARE BOTH CHECKED, WHICH LOOKS LIKE SUPERSTITION AND IS NOT.
// Data written into the kernel's page cache can fail to reach the disk after
// the last write() has already returned success, and on NFS and on a full
// filesystem the error can surface at fsync or at close. A writer that ignores
// either reports success for a file that never landed — which is exactly the
// case rule 13's REFUSED exists for, and exactly why the write goes to a
// `.partial` sibling first: path itself is never touched until fsync and close
// have both said yes.
//
// THE PARTIAL PATH IS BUILT ON THE STACK, NOT AN ARENA. voe_platform_file_write
// takes no arena — it never has — so "<path>.partial" is composed into a fixed
// buffer, the same trade platform/src/window_win32.c makes for a window title:
// a path longer than PARTIAL_PATH_MAX is a call-site mistake, not a runtime
// condition, and asserts rather than returning a failure a caller would have to
// plan for.
#define _POSIX_C_SOURCE 200809L

#include <platform/file.h>

#include <base/assert.h>
#include <base/report.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// Linux's own PATH_MAX, so an ordinary absolute path plus ".partial" always
// fits; see the header comment above for why this is a stack buffer and not
// an arena push.
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
	int file;
	struct stat info;
	size_t size;
	uint8_t *buffer;
	size_t got = 0;

	VOE_BASE_ASSERT(path != NULL, "reading a file with no path");
	VOE_BASE_ASSERT(arena != NULL, "reading a file into no arena");
	VOE_BASE_ASSERT(out_count != NULL, "reading a file with no count to fill");

	file = open(path, O_RDONLY);
	if (file < 0) {
		VOE_BASE_ERROR("platform", "could not open %s for reading: %s",
			       path, strerror(errno));
		report(error, VOE_BASE_ERROR_UNAVAILABLE);
		return NULL;
	}

	if (fstat(file, &info) != 0) {
		VOE_BASE_ERROR("platform", "could not read %s's size: %s", path,
			       strerror(errno));
		report(error, VOE_BASE_ERROR_UNAVAILABLE);
		(void)close(file);
		return NULL;
	}
	if (!S_ISREG(info.st_mode)) {
		VOE_BASE_ERROR("platform", "%s is not a regular file", path);
		report(error, VOE_BASE_ERROR_UNAVAILABLE);
		(void)close(file);
		return NULL;
	}

	size = (size_t)info.st_size;
	// +1 for the NUL the header promises past the last byte; arena_push
	// zeroes what it hands back, so an empty file is already terminated.
	buffer = voe_base_arena_push(arena, size + 1);

	while (got < size) {
		ssize_t step = read(file, buffer + got, size - got);

		if (step < 0) {
			if (errno == EINTR)
				continue;
			VOE_BASE_ERROR("platform",
				       "reading %s failed after %zu of %zu bytes: %s",
				       path, got, size, strerror(errno));
			report(error, VOE_BASE_ERROR_REFUSED);
			(void)close(file);
			return NULL;
		}
		if (step == 0) {
			VOE_BASE_ERROR("platform",
				       "reading %s stopped after %zu of %zu bytes",
				       path, got, size);
			report(error, VOE_BASE_ERROR_REFUSED);
			(void)close(file);
			return NULL;
		}
		got += (size_t)step;
	}

	// Nothing was buffered on our side for a read-only descriptor, so
	// close's result cannot tell us anything the loop above has not; see
	// the header comment on why a write's close is checked and a read's
	// is not.
	(void)close(file);

	*out_count = size;
	report(error, VOE_BASE_OK);
	return buffer;
}

bool voe_platform_file_exists(const char *path)
{
	struct stat info;

	VOE_BASE_ASSERT(path != NULL, "checking existence of no path");

	if (stat(path, &info) != 0)
		return false;
	return S_ISREG(info.st_mode);
}

// Nanoseconds since the epoch, the size multiplied by an odd 64-bit constant
// (the golden ratio's) so a size change moves high bits too, the two XORed.
bool voe_platform_file_stamp(const char *path, uint64_t *out)
{
	struct stat info;
	uint64_t nanoseconds;

	VOE_BASE_ASSERT(path != NULL, "stamping no path");
	VOE_BASE_ASSERT(out != NULL, "stamping into nothing");

	if (stat(path, &info) != 0 || !S_ISREG(info.st_mode))
		return false;
	nanoseconds = (uint64_t)info.st_mtim.tv_sec * 1000000000u +
		      (uint64_t)info.st_mtim.tv_nsec;
	*out = nanoseconds ^ ((uint64_t)info.st_size * 0x9E3779B97F4A7C15u);
	return true;
}

bool voe_platform_file_write(const char *path, const uint8_t *bytes,
			     size_t count, voe_base_error *error)
{
	char partial[PARTIAL_PATH_MAX];
	int file;
	size_t written = 0;

	VOE_BASE_ASSERT(path != NULL, "writing a file with no path");
	VOE_BASE_ASSERT(bytes != NULL, "writing a file from no bytes");
	VOE_BASE_ASSERT(count > 0, "writing a file of no bytes");
	VOE_BASE_ASSERT(strlen(path) + strlen(".partial") < sizeof partial,
			"path is too long for voe_platform_file_write");
	(void)snprintf(partial, sizeof partial, "%s.partial", path);

	file = open(partial, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (file < 0) {
		VOE_BASE_ERROR("platform", "could not open %s for writing: %s",
			       partial, strerror(errno));
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
				       partial, written, count, strerror(errno));
			report(error, VOE_BASE_ERROR_REFUSED);
			(void)close(file);
			(void)unlink(partial);
			return false;
		}
		written += (size_t)step;
	}

	if (fsync(file) != 0) {
		VOE_BASE_ERROR("platform", "flushing %s failed: %s", partial,
			       strerror(errno));
		report(error, VOE_BASE_ERROR_REFUSED);
		(void)close(file);
		(void)unlink(partial);
		return false;
	}

	if (close(file) != 0) {
		VOE_BASE_ERROR("platform", "closing %s failed: %s", partial,
			       strerror(errno));
		report(error, VOE_BASE_ERROR_REFUSED);
		(void)unlink(partial);
		return false;
	}

	if (rename(partial, path) != 0) {
		VOE_BASE_ERROR("platform", "replacing %s with %s failed: %s",
			       path, partial, strerror(errno));
		report(error, VOE_BASE_ERROR_REFUSED);
		(void)unlink(partial);
		return false;
	}

	report(error, VOE_BASE_OK);
	return true;
}
