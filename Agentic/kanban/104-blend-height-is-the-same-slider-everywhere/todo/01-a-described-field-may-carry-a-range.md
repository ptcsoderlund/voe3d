# 01 — A described field may carry a range
folder: base
after: none
decisions: 0168, 0494, 0498

## Change
`base/include/base/describe.h`:
- `voe_base_field_range`: `field` (const char *, as spelled in the struct), `least` and `greatest`
  (double), `unit` (const char *, a literal, `""` for none).
- `voe_base_struct_description` gains `ranges` (const pointer) and `ranges_count`, NULL and 0 for a
  struct described without ranges, as `names` is.
- `voe_base_ranges_find(const voe_base_struct_description *, const char *field)`, static inline,
  the twin of `voe_base_names_find`: the field's range or NULL.
- `VOE_BASE_DESCRIBE_STRUCT_RANGED(struct_name, field_list, ranges_list)` and
  `VOE_BASE_DESCRIBE_STRUCT_NAMED_RANGED(struct_name, field_list, names_list, ranges_list)`. A ranges
  list invokes its one parameter once per ranged field as `R(field, least, greatest, unit)`. Same
  struct, same checks, same field table and (for the second) same names table as today's macros;
  both expand to nothing beyond the struct and checks when descriptions are off. Unlike names, a
  range row holds only constants, so it may sit in a static const initialiser. Keep the existing two
  macros' expansion unchanged for every current user; share internals as you see fit, avoiding the
  empty-initialiser trap the NAMED note already describes.
- A field named in a ranges list that is not a field of the struct should fail the build if that
  is cheap to get (an `offsetof` in a static_assert, say); otherwise say in the header that it is not
  checked.
- Header points to make, beside the NAMES note: a range is a note to a tool like names and
  read-only; the field's kind and text do not change; it is meant for FLOAT32/FLOAT64 of rank 0 and a
  tool ignores it on others; the unit is what a tool shows after the value; what a tool draws (a
  slider) is the tool's business (0494, 0498).

`base/tests/describe.c`: a struct described with `_RANGED` and one with `_NAMED_RANGED`; check
`voe_base_ranges_find` returns the row with its least, greatest and unit, NULL for an unranged
field, NULL for a struct with no ranges, and that the named one still finds its names.

`base/base.md`: the `describe.h` entry also says a field's range and unit. `base/tests/tests.md`:
the `describe.c` entry also says ranges.

## Done when
`ctest --test-dir build/debug -R '^base/describe'` passes with the new checks in it.
