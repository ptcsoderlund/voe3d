# 0162 `platform` owns folders and paths, and a file write replaces its file whole

Status: accepted
Date: 2026-09-15

Spec 004 opens and saves projects: it reads files, lists folders, makes one, finds the home
folder and the per-person settings folder, and requires that an interrupted save leave the
previously saved file whole. `platform/file.h` today writes a whole file by truncating it first
and says reading is not there yet. Listing a folder, telling a hidden entry, finding home and
turning a relative path absolute are each an operating-system call or an operating-system
convention, and `platform` is the only folder allowed either.

## Decision

**Files.** `platform/file.h` gains a whole-file read into an arena (NUL-terminated for the
caller's convenience, the count excluding it) and an existence test for a regular file.
**`voe_platform_file_write` becomes atomic**: it writes a sibling `<path>.partial`, flushes it to
the disk, and renames it over `path`. A failure at any step removes the partial file and leaves
whatever was at `path` untouched. The header's "nothing is ever written anywhere but the path as
spelled" becomes "…but the path and its `.partial` sibling, which never outlives the call".

**Folders**, in a new `platform/folder.h`: list one folder's entries into an arena, sorted by
byte order of name, each with whether it is a folder and whether it is hidden (a leading `.` on
Linux, the hidden attribute on Windows), `.` and `..` left out; make one folder, one level; the
person's home folder; the person's settings folder (`$XDG_CONFIG_HOME`, else `$HOME/.config`, on
Linux; `%APPDATA%` on Windows).

**Paths**, in a new `platform/path.h`: join a folder and a name, a path's parent (none at a
root), a path's last name, and a path made absolute. Paths are the operating system's spelling —
`/` on Linux, `\` or `/` on Windows — and every result lands in an arena.

Every failure a caller can meet is returned under rule 13, with the detail reported at the site
through `base/report.h`. On Windows the environment is read with `GetEnvironmentVariableA`, not
`getenv` (ADR-0159).

## Rejected

- A separate atomic write beside the truncating one — two calls that differ only in whether a
  crash destroys the old file; nobody wants the destroying one.
- A handle-based file API (open, read, seek, close) — nothing reads a file in pieces.
- Listing folders only, filtered in `platform` — the editor also needs "is this folder empty",
  which counts files and hidden entries; one listing with two flags answers both.
- Path arithmetic in the editor — it would have to know Windows' separators and roots, which is
  operating-system knowledge above `platform`.

## Consequences

- `app`'s PNG capture becomes crash-safe for free; its behaviour is otherwise unchanged.
- A crash mid-save can leave a `<name>.partial` beside the file; the next save of that file
  replaces it.
- `platform` gains two public headers, each with a test that needs no window and no display.
