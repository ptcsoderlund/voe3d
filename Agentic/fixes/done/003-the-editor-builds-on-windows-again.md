# 003 — The editor builds on Windows again
folder: editor

## Seen
The editor built on Linux last night. On Windows (clang, Debug, `-Wall -Wextra -Wpedantic -Werror`) it fails:

```
C:/Users/PerSoderlund/Dev/GitRepos/Github/voe3d/editor/src/inspector_layers.c:213:8: error: comparison of integers of different signs: 'long' and 'unsigned int' [-Werror,-Wsign-compare]
  213 |                          to < VOE_EDITOR_SCULPT_GROW_MIN ? VOE_EDITOR_SCULPT_GROW_MIN :
      |                          ~~ ^ ~~~~~~~~~~~~~~~~~~~~~~~~~~
```

## Expected
`voe_editor` compiles without warnings on Windows as on Linux.

## Change
`editor/src/inspector_layers.c:211–215` clamps `to`, a `long` from `lround`, between
`VOE_EDITOR_SCULPT_GROW_MIN` and `VOE_EDITOR_SCULPT_GROW_MAX`. `editor/src/sculpt.h:81` defines the
minimum as `1u`, an `unsigned int`. On Linux `long` is 64 bits and the `1u` converts to `long`; on
Windows `long` is 32 bits, the same rank as `unsigned int`, so `to` converts to unsigned: the warning,
and a negative `to` would clamp to the maximum. The maximum, `VOE_ASSETS_LANDSCAPE_GROW_MAX`, is a
signed `500`; the minimum should be signed too, as the clamps in `editor/src/inspector_foliage.c`
compare against signed constants. Its other user, `GROW_DENSITY_MIN` in `inspector_layers.c:44`,
divides it by `10.0` and is unaffected.

## Done when
- `editor/src/sculpt.h` defines `VOE_EDITOR_SCULPT_GROW_MIN` without the `u` suffix, and no comparison
  in `editor/src` sets a `long` against an unsigned constant.
- The editor's check builds `voe_editor` clean on Linux.
- On Windows, the human's build of `voe_editor` compiles `inspector_layers.c` (checked by the human).
