// The Linux half of platform/trash.h: a path renamed into the freedesktop.org
// home trash beside its .trashinfo. Read the header first — where the trash is
// and which failure is which is written there.
//
// LINUX ONLY, AND THE NAME IS THE BUILD'S — see platform/src/file_wayland.c's
// header. There is no _win32 half: Windows is paused (ADR-0339).
//
// THE ABSOLUTE PATH IS THE PARENT'S realpath PLUS THE NAME, not realpath of the
// path itself: a symlink sent to the trash is the link, and the restore path
// has to name the link, not what it points at.
//
// THE ORDER IS THE SPECIFICATION'S: the info file is claimed with
// O_CREAT | O_EXCL, then written, then the path renamed into files/. A name
// whose info is free but whose files/ entry is taken is skipped too, so a
// leftover from another program never gets replaced. Any failure after the
// claim unlinks the info file, so nothing is left half-trashed.
//
// SAME FILE SYSTEM IS ASKED BEFORE ANYTHING IS CLAIMED, by comparing st_dev of
// the path and of files/; rename's EXDEV would say the same, but only after an
// info file had been written for nothing.
//
// CONSTRAINTS. At most NAME_TRIES numbered names are tried before the trash
// counts as unavailable; a trash holding that many copies of one name is not a
// case worth a smarter search. The folders are made with
// voe_platform_folder_create one level at a time from the root down, each
// level only when it is missing, so a fresh $XDG_DATA_HOME is made whole.
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE // realpath is XSI, not POSIX base, in glibc's headers

#include <platform/trash.h>

#include <platform/folder.h>
#include <platform/path.h>

#include <base/assert.h>
#include <base/report.h>

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define NAME_TRIES 10000

static void report(voe_base_error *error, voe_base_error code)
{
	if (error != NULL)
		*error = code;
}

// Copies text into scratch with room for more, so a caller can cut it at a
// separator in place.
static char *copy(voe_base_arena *scratch, const char *text)
{
	size_t length = strlen(text);
	char *out = voe_base_arena_push(scratch, length + 1);

	memcpy(out, text, length);
	VOE_BASE_ASSERT(out[length] == '\0', "an arena push that is not zeroed");
	return out;
}

// The home trash's own folder, or NULL when neither variable names one.
static const char *trash_folder(voe_base_arena *scratch)
{
	const char *xdg = getenv("XDG_DATA_HOME");
	const char *home;

	if (xdg != NULL && xdg[0] == '/')
		return voe_platform_path_join(scratch, xdg, "Trash");
	home = voe_platform_folder_home(scratch);
	if (home == NULL)
		return NULL;
	return voe_platform_path_join(scratch, home, ".local/share/Trash");
}

// Makes every missing level of path, root first; true when path is a folder at
// the end, false for a relative path (a $HOME that is not absolute). The loop is bounded by path's length.
static bool make_levels(voe_base_arena *scratch, const char *path)
{
	char *walk = copy(scratch, path);
	size_t length = strlen(walk);
	struct stat info;

	VOE_BASE_ASSERT(walk[length] == '\0', "an unterminated copy");
	if (walk[0] != '/')
		return false;
	for (size_t i = 1; i <= length; i++) {
		if (walk[i] != '/' && walk[i] != '\0')
			continue;
		walk[i] = '\0';
		if (stat(walk, &info) != 0 &&
		    !voe_platform_folder_create(walk, NULL))
			return false;
		walk[i] = i < length ? '/' : '\0';
	}
	return stat(path, &info) == 0 && S_ISDIR(info.st_mode);
}

// path's bytes percent-encoded for Path=, everything but RFC 3986's
// unreserved characters and '/' written as %XX.
static const char *percent_encoded(voe_base_arena *scratch, const char *path)
{
	static const char hex[] = "0123456789ABCDEF";
	size_t length = strlen(path);
	char *out = voe_base_arena_push(scratch, length * 3 + 1);
	size_t at = 0;

	for (size_t i = 0; i < length; i++) {
		unsigned char c = (unsigned char)path[i];

		if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
		    (c >= '0' && c <= '9') || strchr("-._~/", c) != NULL) {
			out[at++] = (char)c;
			continue;
		}
		out[at++] = '%';
		out[at++] = hex[c >> 4];
		out[at++] = hex[c & 15];
	}
	VOE_BASE_ASSERT(at <= length * 3, "percent-encoding overran its buffer");
	return out;
}

// Writes the whole of text to file, retrying a short or interrupted write.
static bool write_all(int file, const char *text)
{
	size_t count = strlen(text);
	size_t written = 0;

	while (written < count) {
		ssize_t step = write(file, text + written, count - written);

		if (step < 0 && errno == EINTR)
			continue;
		if (step <= 0)
			return false;
		written += (size_t)step;
	}
	return true;
}

// The info file's text for absolute, deleted now.
static const char *info_text(voe_base_arena *scratch, const char *absolute)
{
	const char *encoded = percent_encoded(scratch, absolute);
	size_t size = strlen(encoded) + 64;
	char *text = voe_base_arena_push(scratch, size);
	char date[32] = { 0 };
	time_t now = time(NULL);
	struct tm local;

	VOE_BASE_ASSERT(now != (time_t)-1, "the clock has no time of day");
	if (localtime_r(&now, &local) == NULL ||
	    strftime(date, sizeof date, "%Y-%m-%dT%H:%M:%S", &local) == 0)
		return NULL;
	(void)snprintf(text, size, "[Trash Info]\nPath=%s\nDeletionDate=%s\n",
		       encoded, date);
	return text;
}

// Claims a free name in trash for name: its info file created exclusively and
// its files/ entry free. Returns the open info file, or -1, with the claimed
// paths in out_info and out_file.
static int claim(voe_base_arena *scratch, const char *trash, const char *name,
		 const char **out_info, const char **out_file)
{
	struct stat info;

	VOE_BASE_ASSERT(name[0] != '\0', "claiming no name");
	for (int n = 1; n <= NAME_TRIES; n++) {
		size_t size = strlen(trash) + strlen(name) + 40;
		char *info_path = voe_base_arena_push(scratch, size);
		char *file_path = voe_base_arena_push(scratch, size);
		int file;

		if (n == 1) {
			(void)snprintf(info_path, size, "%s/info/%s.trashinfo",
				       trash, name);
			(void)snprintf(file_path, size, "%s/files/%s", trash,
				       name);
		} else {
			(void)snprintf(info_path, size,
				       "%s/info/%s.%d.trashinfo", trash, name, n);
			(void)snprintf(file_path, size, "%s/files/%s.%d", trash,
				       name, n);
		}
		file = open(info_path, O_WRONLY | O_CREAT | O_EXCL, 0600);
		if (file < 0 && errno == EEXIST)
			continue;
		if (file < 0) {
			VOE_BASE_ERROR("platform", "could not create %s: %s",
				       info_path, strerror(errno));
			return -1;
		}
		if (lstat(file_path, &info) == 0) {
			(void)close(file);
			(void)unlink(info_path);
			continue;
		}
		*out_info = info_path;
		*out_file = file_path;
		return file;
	}
	VOE_BASE_ERROR("platform", "%s has no free name for %s", trash, name);
	return -1;
}

// Everything after the trash is found: same file system, claim, write, rename.
static voe_base_error send_to_trash(voe_base_arena *scratch, const char *path,
				    const char *absolute, const char *trash)
{
	const char *name = voe_platform_path_name(absolute);
	const char *files = voe_platform_path_join(scratch, trash, "files");
	const char *text = info_text(scratch, absolute);
	const char *info_path = NULL;
	const char *file_path = NULL;
	struct stat at, into;
	int file;

	if (lstat(path, &at) != 0 || stat(files, &into) != 0) {
		VOE_BASE_ERROR("platform", "could not trash %s: %s", path,
			       strerror(errno));
		return VOE_BASE_ERROR_UNAVAILABLE;
	}
	if (at.st_dev != into.st_dev) {
		VOE_BASE_ERROR("platform", "%s is on another file system than %s",
			       path, trash);
		return VOE_BASE_ERROR_UNSUPPORTED;
	}
	if (text == NULL) {
		VOE_BASE_ERROR("platform", "could not read the local time");
		return VOE_BASE_ERROR_UNAVAILABLE;
	}
	file = claim(scratch, trash, name, &info_path, &file_path);
	if (file < 0)
		return VOE_BASE_ERROR_UNAVAILABLE;
	if (!write_all(file, text) || close(file) != 0) {
		VOE_BASE_ERROR("platform", "could not write %s", info_path);
		(void)unlink(info_path);
		return VOE_BASE_ERROR_UNAVAILABLE;
	}
	if (rename(path, file_path) != 0) {
		VOE_BASE_ERROR("platform", "could not move %s to %s: %s", path,
			       file_path, strerror(errno));
		(void)unlink(info_path);
		return errno == EXDEV ? VOE_BASE_ERROR_UNSUPPORTED
				      : VOE_BASE_ERROR_UNAVAILABLE;
	}
	return VOE_BASE_OK;
}

bool voe_platform_trash(const char *path, voe_base_arena *scratch,
			voe_base_error *error)
{
	struct voe_base_arena_mark mark;
	const char *name;
	const char *parent;
	const char *trash;
	char resolved[PATH_MAX];
	voe_base_error result = VOE_BASE_ERROR_UNAVAILABLE;

	VOE_BASE_ASSERT(path != NULL, "trashing no path");
	VOE_BASE_ASSERT(scratch != NULL, "trashing with no scratch arena");

	mark = voe_base_arena_mark(scratch);
	name = voe_platform_path_name(path);
	parent = voe_platform_path_parent(scratch, path);
	trash = trash_folder(scratch);
	if (name[0] == '\0' || strcmp(name, ".") == 0 ||
	    strcmp(name, "..") == 0) {
		VOE_BASE_ERROR("platform", "%s names no file to trash", path);
	} else if (realpath(parent != NULL ? parent : ".", resolved) == NULL) {
		VOE_BASE_ERROR("platform", "could not resolve %s: %s", path,
			       strerror(errno));
	} else if (trash == NULL) {
		VOE_BASE_ERROR("platform", "no home folder to hold a trash");
	} else if (!make_levels(scratch,
				voe_platform_path_join(scratch, trash, "files")) ||
		   !make_levels(scratch,
				voe_platform_path_join(scratch, trash, "info"))) {
		VOE_BASE_ERROR("platform", "could not make the trash at %s",
			       trash);
	} else {
		const char *absolute =
			voe_platform_path_join(scratch, resolved, name);

		result = send_to_trash(scratch, path, absolute, trash);
	}
	voe_base_arena_rewind(scratch, mark);
	report(error, result);
	return result == VOE_BASE_OK;
}
