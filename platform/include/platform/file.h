// A whole file, read or written in one call. The first file API in this
// engine.
//
//     voe_base_error error;
//     if (!voe_platform_file_write("shot.png", bytes, count, &error))
//             return false;           // the reason is already on stderr
//
//     size_t size;
//     const uint8_t *text = voe_platform_file_read("project.voe3d", arena,
//                                                    &size, &error);
//     if (text == NULL)
//             return false;
//
// FILES ARE THIS FOLDER'S BECAUSE OPENING ONE IS THE OPERATING SYSTEM'S CALL.
// It is open/read/write/close on Linux and CreateFile/ReadFile/WriteFile/
// CloseHandle on Windows, both out of operating-system headers, and platform
// is the only folder allowed to include one (ADR-0157). So the folder that
// wants bytes off disk, or wants bytes onto it, hands over a name, exactly as
// the folder that wants a shared library hands over a name in
// platform/library.h.
//
// THE PATH IS EXACTLY WHAT THE CALLER GAVE. No directory is created, no
// extension is added, no default directory is prefixed. A read looks at
// nothing but that path; a write touches nothing but that path and the
// `.partial` sibling described below, which never outlives the call.
//
// A NULL path or arena, or a NULL out_count on a read, or NULL bytes or a
// count of zero on a write, is the caller's bug and aborts (rule 13). An
// empty file is not something this engine has ever wanted to write, and the
// call that produced no bytes is the bug worth catching — reading one back is
// a different question, answered at voe_platform_file_read.
#pragma once

#include <base/arena.h>
#include <base/error.h>

#include <stddef.h>
#include <stdint.h>

// READING. voe_platform_file_read pushes the whole file into arena and adds
// one more byte past the last one, set to NUL for a caller that wants to
// treat the result as a C string; *out_count is the file's own length and
// does not include that byte. An empty file is a valid, non-NULL pointer
// with *out_count set to 0 — reading nothing is not a failure here, the way
// writing nothing is a caller bug in voe_platform_file_write below; a file
// with no bytes in it is a thing the world can hand back, where a call asking
// to write no bytes is a call with nothing to do.
//
// VOE_BASE_ERROR_UNAVAILABLE out of a read is a path that could not be opened
// for reading at all: nothing there, a folder instead of a file, permission
// refused. VOE_BASE_ERROR_REFUSED is a read that failed after the file was
// opened: the device went away, fewer bytes arrived than the file's own
// length said it would. Which one it was is the category; what exactly
// happened is written to stderr at the site through base/report.h.
//
// Reads the whole file at path into arena, NUL-terminated past the last
// byte, which *out_count does not count. NULL on failure, with the category
// in error when error is not NULL and the real reason already reported.
[[nodiscard]] const uint8_t *voe_platform_file_read(const char *path,
						     voe_base_arena *arena,
						     size_t *out_count,
						     voe_base_error *error);

// voe_platform_file_exists is true only for a path that is a regular file —
// false for a folder, a symlink to nothing, and anything not there. It is
// yes/no and cannot fail: nothing about a missing path is worth reporting.
//
// True only for a path that is a regular file.
bool voe_platform_file_exists(const char *path);

// A STAMP TELLS A CHANGED FILE FROM THE SAME ONE, AND NOTHING ELSE. It mixes
// the modification time, at the finest resolution the system gives, with the
// size into one number. Only equality means anything: two stamps are the same
// file unchanged or they are not, and a caller never orders them, because a
// mix has no order and a clock set back would break one anyway. The size is
// in it because a file rewritten twice inside one clock tick keeps its time;
// a different length still changes the stamp. A NULL path or out is the
// caller's bug and aborts.
//
// True with the stamp in *out for a regular file; false, *out untouched, for
// a folder or anything not there.
[[nodiscard]] bool voe_platform_file_stamp(const char *path, uint64_t *out);

// WRITING. voe_platform_file_write is atomic: it writes every byte to a
// sibling named `<path>.partial`, flushes that file to the disk (`fsync` /
// `FlushFileBuffers`), closes it, and only then renames it over path
// (`rename` / `MoveFileExA` with `MOVEFILE_REPLACE_EXISTING |
// MOVEFILE_WRITE_THROUGH`) — so a crash or a failure at any step before the
// rename leaves whatever was at path completely untouched, and the rename
// itself is the one step no filesystem this engine targets can leave half
// done. A failure at any step once the partial file exists deletes it before
// returning; a caller never has to know it was ever there. A path that
// cannot even be opened for the partial write creates nothing at all.
//
// THE TWO WAYS A WRITE FAILS, AND WHAT EACH ONE MEANS TO A CALLER.
// VOE_BASE_ERROR_UNAVAILABLE is a `.partial` path that could not be opened: a
// directory that is not there, a name that is not allowed, permission
// refused. Nothing was created and nothing was touched. VOE_BASE_ERROR_REFUSED
// is anything after that — a write, a flush, a close, or the final rename —
// that failed: a full disk, a device that went away, a rename refused because
// path is itself a folder. The `.partial` file is deleted and path is left
// exactly as it was. Which one it was is the category; what exactly
// happened — errno, GetLastError — is written to stderr at the site through
// base/report.h, because a category cannot carry "no space left on device".
//
// Writes count bytes to path, replacing what is there and creating it if it
// is not, by writing a sibling and renaming it over path — see above. Returns
// false on failure, with the category in error when error is not NULL and
// the real reason already reported.
[[nodiscard]] bool voe_platform_file_write(const char *path,
					   const uint8_t *bytes, size_t count,
					   voe_base_error *error);

// MOVING. voe_platform_file_move renames or moves a file, or a folder with
// all it holds, from one path to another in one step, and never replaces:
// VOE_BASE_ERROR_REFUSED is a target already taken, file or folder.
// VOE_BASE_ERROR_UNAVAILABLE is anything else: nothing at from, a target
// folder that is not there, another file system, permission refused. The
// real reason is reported at the site, as everywhere in this file. On
// failure nothing moved. A NULL from or to is the caller's bug and aborts.
//
// Moves from to to, never over something already there. Returns false on
// failure, with the category in error when error is not NULL.
[[nodiscard]] bool voe_platform_file_move(const char *from, const char *to,
					  voe_base_error *error);
