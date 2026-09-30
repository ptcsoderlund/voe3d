# 04 — The Windows build fails in platform

## Seen
The debug build on Windows stops in `voe_platform`. `window_win32.c`, `seat_win32.c` and `gamepad_win32.c`
all fail the same way, through `platform/src/window_win32.h:27`, which includes the Windows SDK's `hidpi.h`:

```
In file included from C:/Users/PerSoderlund/Dev/GitRepos/Github/voe3d/platform/src/window_win32.c:25:
In file included from C:/Users/PerSoderlund/Dev/GitRepos/Github/voe3d/platform/src\window_win32.h:27:
C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\shared\hidpi.h:62:5: error: unknown type name 'USAGE'
   62 |     USAGE Usage;
      |     ^
...
fatal error: too many errors emitted, stopping now [-ferror-limit=]
20 errors generated.
ninja: build stopped: subcommand failed.
```

Compiler: `C:\PROGRA~1\LLVM\bin\clang.exe`, `-std=c23 -Wall -Wextra -Wpedantic -Werror`, Windows SDK
10.0.26100.0. Linux builds and checks pass, so no card saw it (ADR-0130: Linux alone verifies a card).

## Expected
The engine, editor and tank game build on Windows as they do on Linux.

## How to reproduce
1. On Windows, with the Windows SDK 10.0.26100.0 and LLVM clang, check out this branch.
2. `cmake --preset debug`, then `cmake --build --preset debug`.
3. The build stops in `voe_platform` with `unknown type name 'USAGE'` in `hidpi.h`.
