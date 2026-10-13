# 05 — Windows does not build: strcpy is deprecated

## Seen
The sponsor, building on Windows (clang, MSVC headers from the Windows 10.0.26100.0 SDK, Debug), filed under
0381. `voe_assets` fails and the build stops there:

```
FAILED: [code=1] assets/CMakeFiles/voe_assets.dir/src/material.c.obj
C:/Users/PerSoderlund/Dev/GitRepos/Github/voe3d/assets/src/material.c:82:2: error: 'strcpy' is deprecated: This
function or variable may be unsafe. Consider using strcpy_s instead. To disable deprecation, use
_CRT_SECURE_NO_WARNINGS. See online help for details. [-Werror,-Wdeprecated-declarations]
   82 |         strcpy(path, value);
```

The MSVC headers mark `strcpy` with `__declspec(deprecated)`, and `-Werror` makes the warning an error. The
call came in with 085, not 087. It is filed here because 087 is the feature on the board.

The build stops at the first failure, so this is only the first one. Other calls the MSVC headers deprecate
are still in code that builds on Windows: `strcpy` in `assets/src/landscape.c:206`, `getenv` in
`render/src/instance.c:284`, and `strcpy`/`fopen`-style calls in several tests (`3d/tests`, `assets/tests`,
`game/tests`, `audio/tests`, `app/tests`, `platform/tests`, `scene/tests`). The `_wayland.c` sources do not
build on Windows.

## Expected
The tree builds on Windows with `-Werror`, as 0381 requires. The fix covers every call that the Windows
headers deprecate, not only `material.c:82`. No Linux guard: per 0159 the next such call is found on Windows
(the sponsor chose this again on 2026-10-13).

## How to reproduce
On Windows, configure the debug preset and build `voe_assets`, or the whole tree.
