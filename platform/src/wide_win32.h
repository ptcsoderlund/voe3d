// UTF-8 to UTF-16 and back, for the Windows files in platform/src only; never
// included outside this folder. Every path, name and environment value the
// engine holds is UTF-8 (ADR-0247); Windows takes and hands back UTF-16 through
// its "W" calls, and these two functions are the only crossing between them.
//
// THE "W" CALLS AND NOT A UTF-8 CODE-PAGE MANIFEST (ADR-0248). A manifest would
// leave the "A" calls in place, needs Windows 10 1903 or later, and would have
// to be embedded in every executable the build makes, which needs a resource
// step the build does not have. The "W" calls keep it all in this folder.
//
// FLAGS 0, NOT MB_ERR_INVALID_CHARS. Both functions call MultiByteToWideChar /
// WideCharToMultiByte with CP_UTF8 and flags 0, so malformed UTF-8 and a lone
// surrogate become U+FFFD instead of failing: no character makes anything
// fail (ADR-0247).
//
// A PATH THAT DOES NOT FIT THE CALLER'S STACK BUFFER IS AN ORDINARY FAILURE.
// A call that takes no arena converts into a MAX_PATH buffer on its stack, and
// voe_platform_wide_from_utf8 answers false when the text does not fit. The
// caller then fails the way it fails when the open fails. The file calls
// without the `\\?\` prefix refuse a longer path anyway, so this sets no new
// limit.
#pragma once

#include <base/arena.h>

#include <wchar.h>

// Converts NUL-terminated UTF-8 text into out, terminator included. False when
// it does not fit capacity wide units; out is then not to be used.
[[nodiscard]] bool voe_platform_wide_from_utf8(const char *text, wchar_t *out,
					       int capacity);

// A NUL-terminated UTF-8 copy of NUL-terminated UTF-16 text, in arena.
char *voe_platform_utf8_from_wide(const wchar_t *text, voe_base_arena *arena);
