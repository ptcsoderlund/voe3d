# 43 — Assets copies a material or layer path with memcpy
folder: assets/src
after: 41
decisions: 0168, 0413

## Change
Bug 05, the call that stops the Windows build: `strcpy` is deprecated by the Windows headers.

- `assets/src/material.c`, about line 82: `strcpy(path, value)` becomes a `memcpy` of `strlen(value) + 1`
  bytes (0413 point 2). The length check above it stays and already bounds it; keep the length in a local
  rather than measuring twice.
- `assets/src/landscape.c`, about line 206: `strcpy(into->layers[k], path)` the same way.

No header or index change.

## Done when
- `grep -nE '\bstrn?cpy[[:space:]]*\(' assets/src/material.c assets/src/landscape.c` prints nothing.
- Tests `assets/material` and `assets/landscape` pass.
