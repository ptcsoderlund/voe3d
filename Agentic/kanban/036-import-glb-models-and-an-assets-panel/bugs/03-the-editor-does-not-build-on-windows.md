# 03 — The editor does not build on Windows

## Seen
The Windows debug build (CLion, clang with the MSVC runtime) stops in `editor/src/models.c`:

```
C:/Users/PerSoderlund/Dev/GitRepos/Github/voe3d/editor/src/models.c:62:20: error: 'strdup' is deprecated: The POSIX name for this item is deprecated. Instead, use the ISO C and C++ conformant name: _strdup. See online help for details. [-Werror,-Wdeprecated-declarations]
   62 |                 models->folder = strdup(folder);
      |                                  ^
C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\ucrt\string.h:531:20: note: 'strdup' has been explicitly marked deprecated here
1 error generated.
ninja: build stopped: subcommand failed.
```

Linux builds fine, so the checks never saw it.

## Expected
The editor builds on Windows as it does on Linux, with warnings still errors.

## How to reproduce
1. On Windows, open the tree in CLion (or configure the debug preset with clang).
2. Build `voe_editor`.
3. The build fails at `editor/src/models.c` line 62.
