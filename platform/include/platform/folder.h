// A folder's contents, one level, sorted; making one folder; and the two
// folders a person's home and this engine's settings live in. Read
// platform/file.h first — this is the same trade, for folders instead of
// files.
//
//     voe_base_error error;
//     voe_platform_folder_listing listing;
//     if (!voe_platform_folder_list(".", arena, &listing, &error))
//             return false;               // the reason is already on stderr
//     for (uint32_t i = 0; i < listing.count; i++)
//             printf("%s%s\n", listing.entries[i].name,
//                    listing.entries[i].folder ? "/" : "");
//
// FOLDERS ARE THIS FOLDER'S FOR THE SAME REASON FILES ARE (platform/file.h):
// listing one, making one, and finding where a person's home and settings live
// are each an operating-system call or an operating-system convention, and
// platform is the only folder allowed either (ADR-0157, ADR-0162).
//
// LISTING. voe_platform_folder_list pushes one voe_platform_folder_entry per
// name in path into arena — never "." or "..", which are not entries a caller
// ever wants — sorted ascending by byte order of name, which is strcmp's order
// and not a locale's. folder is whether the name is itself a folder, following
// a symlink to decide when the name at path is one; hidden is one meaning on
// every platform (ADR-0166): a name beginning with '.', with Windows'
// FILE_ATTRIBUTE_HIDDEN marking a further entry hidden on top of it. An empty
// folder is a valid
// listing of zero, in an entries pointer that may be NULL — there is nothing
// to index — exactly as voe_platform_file_read hands back a valid pointer for
// zero bytes. The names themselves are copied into arena; nothing in the
// listing points back at memory the operating system owns.
//
// VOE_BASE_ERROR_UNAVAILABLE out of a listing is a path that could not be
// opened as a folder at all: nothing there, a file instead of a folder,
// permission refused. There is no second way for a listing to fail — once it
// is open, reading its own entries back does not fail on the platforms this
// engine targets.
//
// CREATING. voe_platform_folder_create makes exactly one level — the parent
// must already be there. VOE_BASE_ERROR_UNAVAILABLE is the parent missing or
// not allowed to hold a new folder; VOE_BASE_ERROR_REFUSED is a name already
// taken, whether by a folder, a file or anything else.
//
// HOME AND SETTINGS. voe_platform_folder_home is the person's own folder —
// $HOME on Linux, %USERPROFILE% on Windows — and NULL when the operating
// system has not set it, which nothing here can recover from. It cannot fail
// the way a file or folder operation can: there is no report for "not set",
// because what a caller does about a missing home folder is not this folder's
// business. voe_platform_folder_settings is where this engine's own settings
// belong: $XDG_CONFIG_HOME when it is set to an absolute path, else
// $HOME/.config, on Linux; %APPDATA% on Windows; NULL when neither answer
// exists. Both push their result into arena, and neither, like every path
// this header or platform/path.h hands back, carries a trailing separator.
//
// A NULL path, arena or out is the caller's bug and aborts (rule 13).
#pragma once

#include <base/arena.h>
#include <base/error.h>

#include <stdint.h>

typedef struct {
	const char *name;
	bool folder;
	bool hidden;
} voe_platform_folder_entry;

typedef struct {
	const voe_platform_folder_entry *entries;
	uint32_t count;
} voe_platform_folder_listing;

// Lists path's own entries into arena. False on failure, with the category in
// error when error is not NULL and the real reason already reported.
[[nodiscard]] bool voe_platform_folder_list(const char *path,
					    voe_base_arena *arena,
					    voe_platform_folder_listing *out,
					    voe_base_error *error);

// Makes one folder, one level. False on failure, with the category in error
// when error is not NULL and the real reason already reported.
[[nodiscard]] bool voe_platform_folder_create(const char *path,
					      voe_base_error *error);

// The person's own folder, pushed into arena, or NULL when the operating
// system has not set one.
const char *voe_platform_folder_home(voe_base_arena *arena);

// Where this engine's own settings belong, pushed into arena, or NULL when
// neither the operating system nor its usual convention names one.
const char *voe_platform_folder_settings(voe_base_arena *arena);
