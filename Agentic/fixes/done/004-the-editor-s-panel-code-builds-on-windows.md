# 004 — The editor's panel code builds on Windows
folder: editor

## Seen
On Windows (clang, Debug, `-Wall -Wextra -Wpedantic -Werror`) `voe_editor` fails in four files:

```
editor/src/panel_drag.c:85:9: error: comparison of integers of different signs: 'uint32_t' (aka 'unsigned int') and 'voe_editor_closable' [-Werror,-Wsign-compare]
   85 |                 if (c != which && root->floats[c].raised > top)
editor/src/settings_layout.c:214:29: error: comparison of integers of different signs: 'voe_editor_dock_axis' and '__size_t' (aka 'unsigned long long') [-Werror,-Wsign-compare]
  214 |         VOE_BASE_ASSERT(node->axis < AXIS_COUNT, "a split's axis read past its words");
editor/src/float_edges.c:66:9: error: comparison of integers of different signs: 'uint32_t' (aka 'unsigned int') and 'voe_editor_closable' [-Werror,-Wsign-compare]
   66 |                 if (c != which && root->floats[c].raised > top)
editor/src/dock_move.c:142:9: error: comparison of integers of different signs: 'uint32_t' (aka 'unsigned int') and 'voe_editor_closable' [-Werror,-Wsign-compare]
  142 |                 if (c != dragged && root->floats[c].floating &&
editor/src/dock_move.c:330:10: error: comparison of integers of different signs: 'uint32_t' (aka 'unsigned int') and 'voe_editor_closable' [-Werror,-Wsign-compare]
  330 |                         if (c != which)
```

## Expected
`voe_editor` compiles without warnings on Windows as on Linux.

## Change
An enum with no negative enumerators is `unsigned int` under the Linux (Itanium) ABI and `int` under
the Windows (MSVC) ABI, so a comparison between an enum value and an unsigned value is silent on Linux
and a `-Wsign-compare` error on Windows.

- `editor/src/panel_drag.c:85`, `editor/src/float_edges.c:66`, `editor/src/dock_move.c:142` and
  `:330` compare a `uint32_t` loop index `c` against a `voe_editor_closable` (`which`, `dragged`).
- `editor/src/settings_layout.c:214` asserts a `voe_editor_dock_axis` against `AXIS_COUNT`, a `size_t`.

Make each comparison in one signedness on both ABIs, the same way at all five places. Then sweep the
rest of `editor/src` for other comparisons of an enum-typed value against an unsigned value
(`uint32_t`, `size_t`, a `u`-suffixed or `sizeof`/countof constant): Linux cannot show them, so the
compiler there is no proof. Fix any found the same way.

## Done when
- No comparison in `editor/src` sets an enum-typed value against an unsigned one; the sweep's result
  (places found, or none) is in the commit message.
- The editor's check builds `voe_editor` clean on Linux.
- On Windows, the human's build of `voe_editor` compiles all four files (checked by the human).
