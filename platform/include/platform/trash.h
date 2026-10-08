// A file or folder sent to the desktop's trash, where a person can take it
// back from their file manager: the freedesktop home trash on Linux and the
// Recycle Bin on Windows (ADR-0381, ADR-0382). The Assets panel's Delete is
// the caller (ADR-0378 point 7).
//
//     voe_base_error error;
//     if (!voe_platform_trash("Assets/old.prefab", scratch, &error))
//             return false;               // the reason is already on stderr
//
// ON LINUX, THE HOME TRASH OF THE FREEDESKTOP.ORG TRASH SPECIFICATION: $XDG_DATA_HOME/Trash
// when that variable is an absolute path, else $HOME/.local/share/Trash, its
// files/ and info/ folders made as needed. The file or folder is renamed into
// files/, whole, beside an info/<name>.trashinfo holding "[Trash Info]",
// "Path=" the absolute path percent-encoded, and "DeletionDate=" the local time
// as YYYY-MM-DDThh:mm:ss — what a file manager reads to restore it.
//
// ON LINUX A TAKEN NAME IS NUMBERED: name, name.2, name.3, ... The info file is
// made first, with an exclusive create, so two callers never claim the same name.
//
// ON WINDOWS, THE RECYCLE BIN, through the shell, which names and restores.
//
// FAILURES. On Linux a path on another file system than the trash is
// VOE_BASE_ERROR_UNSUPPORTED and nothing is moved: the per-drive
// .Trash-<uid> folder is not written until someone needs it. Any other failure
// — nothing at path, a trash that cannot be made, a rename refused — is
// VOE_BASE_ERROR_UNAVAILABLE, the real reason reported where it happened. On
// failure the info file is removed again and the path is where it was.
// On Windows a path not on a fixed drive, which has no Recycle Bin, is
// VOE_BASE_ERROR_UNSUPPORTED and nothing is deleted; nothing at path, a path
// too long, a shell failure or a delete the person declined is
// VOE_BASE_ERROR_UNAVAILABLE, the shell's code reported.
//
// Working memory comes from scratch and is rewound before returning; nothing
// pushed before the call is touched. A NULL path or scratch is the caller's
// bug and aborts (rule 13); error may be NULL.
#pragma once

#include <base/arena.h>
#include <base/error.h>

// Sends path to the home trash. False on failure, with the category in error
// when error is not NULL and the real reason already reported.
[[nodiscard]] bool voe_platform_trash(const char *path, voe_base_arena *scratch,
				      voe_base_error *error);
