// The Windows half of platform/trash.h: a path sent to the Recycle Bin by
// SHFileOperationW (ADR-0381, ADR-0382). Read the header first — which failure
// is which is written there.
//
// SHFileOperationW, NOT IFileOperation. It is one call in shell32, which
// platform already links (ADR-0248), and needs no COM; IFileOperation does the
// same through COM vtables in C for no gain here. Writing the Recycle Bin's
// $I/$R files by hand, as the Linux side writes the freedesktop trash, would
// depend on a format Windows does not document.
//
// ONLY A FIXED DRIVE. With FOF_ALLOWUNDO and no UI, a drive that has no Recycle
// Bin (removable, network) deletes for good without asking, which Delete must
// never do. The path's volume, by GetVolumePathNameW, must be DRIVE_FIXED, or
// the call is UNSUPPORTED with nothing deleted — the Linux side's refusal of
// another file system.
//
// THE NUKE WARNING STAYS. A fixed drive whose Recycle Bin is switched off or
// too small would still delete for good; FOF_WANTNUKEWARNING asks the person
// first even under FOF_NOCONFIRMATION, so nothing is ever lost without a
// question. Saying no there is an aborted operation, UNAVAILABLE.
//
// THE DOUBLE NUL. pFrom is a list of paths, each ended by a NUL and the list by
// a second one; a single path missing the second makes the shell read past it.
// The absolute path is written into a buffer one wider than MAX_PATH, never
// filled past MAX_PATH by GetFullPathNameW, so the last unit is always free
// for it. The path is made absolute because SHFileOperationW is documented to
// want full paths and a relative one is read against a working folder it does
// not promise.
//
// CONSTRAINTS. The path, converted and absolute, fits MAX_PATH wide units or
// it is UNAVAILABLE (platform/src/wide_win32.h); the shell takes no `\\?\`
// path, so a longer one could not be sent anyway. Nothing here uses scratch,
// but it is marked and rewound as the header promises.
#include <platform/trash.h>

#include "wide_win32.h"

#include <base/assert.h>
#include <base/report.h>

#include <windows.h>

#include <shellapi.h>

#include <wchar.h>

static void report(voe_base_error *error, voe_base_error code)
{
	if (error != NULL)
		*error = code;
}

// The shell's delete of absolute, which ends in two NULs, into the Recycle Bin.
static voe_base_error send_to_recycle_bin(const char *path,
					  const wchar_t *absolute)
{
	SHFILEOPSTRUCTW operation = { 0 };
	int code;

	operation.hwnd = NULL;
	operation.wFunc = FO_DELETE;
	operation.pFrom = absolute;
	operation.pTo = NULL;
	operation.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT |
			   FOF_NOERRORUI | FOF_WANTNUKEWARNING;
	code = SHFileOperationW(&operation);
	if (code != 0) {
		VOE_BASE_ERROR("platform", "could not trash %s: error 0x%x", path,
			       (unsigned int)code);
		return VOE_BASE_ERROR_UNAVAILABLE;
	}
	if (operation.fAnyOperationsAborted) {
		VOE_BASE_ERROR("platform", "trashing %s was cancelled", path);
		return VOE_BASE_ERROR_UNAVAILABLE;
	}
	return VOE_BASE_OK;
}

// Everything after the path is converted: it exists, is absolute and is on a
// fixed drive, then the shell sends it.
static voe_base_error trash_wide(const char *path, const wchar_t *wide)
{
	wchar_t absolute[MAX_PATH + 1] = { 0 };
	wchar_t volume[MAX_PATH + 1] = { 0 };
	DWORD length;

	if (GetFileAttributesW(wide) == INVALID_FILE_ATTRIBUTES) {
		VOE_BASE_ERROR("platform", "could not trash %s: error %lu", path,
			       (unsigned long)GetLastError());
		return VOE_BASE_ERROR_UNAVAILABLE;
	}
	// MAX_PATH, not the buffer's size: the last unit stays the second NUL.
	length = GetFullPathNameW(wide, MAX_PATH, absolute, NULL);
	if (length == 0 || length >= MAX_PATH) {
		VOE_BASE_ERROR("platform", "could not trash %s: the path is too long or unresolved",
			       path);
		return VOE_BASE_ERROR_UNAVAILABLE;
	}
	absolute[length + 1] = L'\0';
	if (!GetVolumePathNameW(absolute, volume, MAX_PATH + 1)) {
		VOE_BASE_ERROR("platform", "could not find the drive of %s: error %lu",
			       path, (unsigned long)GetLastError());
		return VOE_BASE_ERROR_UNAVAILABLE;
	}
	if (GetDriveTypeW(volume) != DRIVE_FIXED) {
		VOE_BASE_ERROR("platform", "%s is not on a fixed drive, which has no Recycle Bin",
			       path);
		return VOE_BASE_ERROR_UNSUPPORTED;
	}
	VOE_BASE_ASSERT(absolute[length] == L'\0' && absolute[length + 1] == L'\0',
			"a path list without its two NULs");
	return send_to_recycle_bin(path, absolute);
}

bool voe_platform_trash(const char *path, voe_base_arena *scratch,
			voe_base_error *error)
{
	struct voe_base_arena_mark mark;
	wchar_t wide[MAX_PATH];
	voe_base_error result;

	VOE_BASE_ASSERT(path != NULL, "trashing no path");
	VOE_BASE_ASSERT(scratch != NULL, "trashing with no scratch arena");

	mark = voe_base_arena_mark(scratch);
	if (!voe_platform_wide_from_utf8(path, wide, MAX_PATH)) {
		VOE_BASE_ERROR("platform", "could not trash %s: the path is too long",
			       path);
		result = VOE_BASE_ERROR_UNAVAILABLE;
	} else {
		result = trash_wide(path, wide);
	}
	voe_base_arena_rewind(scratch, mark);
	report(error, result);
	return result == VOE_BASE_OK;
}
