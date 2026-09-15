// A whole file, written in one call. The first file API in this engine, and for
// now the only one.
//
//     voe_base_error error;
//     if (!voe_platform_file_write("shot.png", bytes, count, &error))
//             return false;           // the reason is already on stderr
//
// FILES ARE THIS FOLDER'S BECAUSE OPENING ONE IS THE OPERATING SYSTEM'S CALL.
// It is open/write/close on Linux and CreateFile/WriteFile/CloseHandle on
// Windows, both out of operating-system headers, and platform is the only folder
// allowed to include one (ADR-0157). So the folder that has bytes and wants them
// on disk hands them over with a name, exactly as the folder that wants a shared
// library hands over a name in platform/library.h.
//
// THE PATH IS EXACTLY WHAT THE CALLER GAVE. No directory is created, no
// extension is added, no default directory is prefixed, and nothing is ever
// written anywhere but the path as spelled. A path whose directory does not
// exist is a failure and not an invitation to make one: whoever named the file
// knows where it was meant to go, and this folder does not.
//
// The file is replaced. Whatever was at that path is truncated to nothing first,
// and a file that was not there is created — so the result is always exactly
// `count` bytes and never a short write left inside an older, longer file.
//
// THE TWO WAYS IT FAILS, AND WHAT EACH ONE MEANS TO A CALLER.
// VOE_BASE_ERROR_UNAVAILABLE is a path that could not be opened at all: a
// directory that is not there, a name that is not allowed, permission refused.
// Nothing was created and nothing was touched. VOE_BASE_ERROR_REFUSED is a write
// or a close that failed after the file was opened: a full disk, a device that
// went away. The file then exists and its contents are whatever got through, and
// this call does not delete it — a caller who wants it gone knows the path.
// Which one it was is the category; what exactly happened — errno, GetLastError
// — is written to stderr at the site through base/report.h, because a category
// cannot carry "no space left on device".
//
// A NULL path, NULL bytes or a count of zero is the caller's bug and aborts
// (rule 13). An empty file is not something this engine has ever wanted to
// write, and the call that produced no bytes is the bug worth catching.
//
// READING A FILE IS NOT HERE. It is written when a loader needs one (rule 10),
// and it will be a second function beside this one rather than a handle type
// with open, seek and close on it.
#pragma once

#include <base/error.h>

#include <stddef.h>
#include <stdint.h>

// Writes count bytes to path, replacing what is there and creating it if it is
// not. Returns false on failure, with the category in error when error is not
// NULL and the real reason already reported.
[[nodiscard]] bool voe_platform_file_write(const char *path,
					   const uint8_t *bytes, size_t count,
					   voe_base_error *error);
