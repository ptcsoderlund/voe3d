# 41 — Platform reads an environment variable
folder: platform
after: none
decisions: 0168, 0413

## Change
Bug 05: `getenv` is deprecated by the Windows headers, so the environment gets a reader here (0413 point 1).

- New `include/platform/environment.h`: `const char *voe_platform_environment_read(const char *name,
  voe_base_arena *arena);` — the value as UTF-8, copied onto the arena; NULL when the variable is not set; an
  empty value is `""`, not NULL. A NULL name or arena is the caller's bug and aborts. Header points: why the
  environment is platform's (an OS call; Windows reads it wide, 0247/0248); unset versus empty; the copy is
  the caller's to keep.
- New `src/environment_wayland.c`: `getenv`, copied onto the arena.
- New `src/environment_win32.c`: `GetEnvironmentVariableW` through `src/wide_win32.h`
  (`voe_platform_wide_from_utf8` for the name, `voe_platform_utf8_from_wide` for the value). Take over the
  body of `src/folder_win32.c`'s static `read_variable`; that function goes and its callers in
  `folder_win32.c` call `voe_platform_environment_read` with UTF-8 names. Fix `folder_win32.c`'s header
  line that says how the environment is read. Unset versus empty: `ERROR_ENVVAR_NOT_FOUND`.
- New `tests/environment.c`: `PATH` comes back non-NULL and non-empty; `VOE_PLATFORM_NO_SUCH_VARIABLE` comes
  back NULL. No `setenv` (Windows has none and the test must build there).
- Index entries: `platform.md` (the new header, and "the environment" in the opening list), `src/src.md`
  (both new sources; `folder_win32.c` now reads through it), `tests/tests.md` (the new test).

Only Linux is built; write the Windows file as carefully as the Linux one (0381).

## Done when
- Test `platform/environment` passes.
- `grep -n read_variable platform/src/folder_win32.c` prints nothing.
