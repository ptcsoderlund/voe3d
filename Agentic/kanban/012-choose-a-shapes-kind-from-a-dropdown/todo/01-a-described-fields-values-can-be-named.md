# 01 — A described field's values can be named
folder: base
decisions: 0168, 0195, 0198

## Change
`base/include/base/describe.h`: a struct's description may carry, beside its fields, a small table saying what
one field's values are called.

- A new type, above `voe_base_struct_description`:

		typedef struct {
			// The field these names belong to, as it is spelled in
			// the struct.
			const char *field;
			// values[i] is the name of value i, NULL for a value
			// this list does not name.
			const char *const *values;
			uint32_t value_count;
		} voe_base_field_names;

  Declared whether or not descriptions are compiled in, as every other type in this header is.
- `voe_base_struct_description` gains `const voe_base_field_names *names;` and `uint32_t names_count;` after
  `field_count`. A struct described the way every struct is described today has them NULL and nought.
- `VOE_BASE_DESCRIBE_STRUCT_NAMED(struct_name, field_list, names_list)`: the same struct, the same checks and
  the same field table as `VOE_BASE_DESCRIBE_STRUCT`, plus the names table. A names list is a macro taking one
  parameter and invoking it once per named field with the field's name and the array of names:

		#define VOE_3D_SHAPE_NAMES(N) N(kind, voe_3d_shape_kind_names)

  Each row is `{ .field = #field_, .values = (values_), .value_count = (uint32_t)(sizeof(values_) /
  sizeof((values_)[0])) }`, so the count is the array's declared bound and never a number written twice.
  `VOE_BASE_DESCRIBE_STRUCT(struct_name, field_list)` keeps its two arguments and stays exactly what it is: give
  the named form its own table macro beside `VOE_BASE_DESCRIBE_TABLE_` rather than expanding the plain one
  through it with an empty names list — an empty initializer for a zero-length array is the GNU extension
  `-Wpedantic -Werror` refuses. Both table macros expand to nothing when `VOE_BASE_DESCRIPTIONS` is off.
- `static inline const voe_base_field_names *voe_base_names_find(const voe_base_struct_description *description,
  const char *field)`: the entry whose `field` matches, or NULL — NULL for a NULL description, a NULL `names` or
  a nought `names_count`. `strcmp` over at most a handful of rows, so the header includes `<string.h>`.
- The header gets one paragraph in its own voice: NAMES ARE A NOTE TO A TOOL, EXACTLY AS READ-ONLY IS. Say that
  entry `i` names value `i` and a NULL entry is a value with no name, so a set numbered from one leaves its
  first entry NULL; that the field's kind does not change — a named field is the `UINT32` it already was and the
  scene text it is written to is unchanged; that the names array belongs to the declaring folder and is
  `extern` there so a build without descriptions carries no copy of it; and that what a tool does with them —
  a dropdown of the named values — is the tool's business and not this folder's (0195, 0198).

`base/tests/describe.c`: add to the struct the test already describes, or to a third one beside it, a field
`F(uint32_t, mode, UINT32)` named through `VOE_BASE_DESCRIBE_STRUCT_NAMED` with `{ NULL, "One", "Two" }`, and
check: the description's `names_count` is 1; `voe_base_names_find(d, "mode")` finds the entry, its
`value_count` is 3, `values[0]` is NULL, `values[1]` is "One"; `voe_base_names_find(d, "absent")` is NULL; and
the existing struct described by the plain macro has `names` NULL, `names_count` nought and
`voe_base_names_find` answering NULL for one of its own field names.

`base/base.md`: the `include/base/describe.h` line gains that a field's values may be named for a tool.

## Done when
`checks.sh` for `base` exits 0 with the new checks in `base/describe`, and `cmake --build --preset debug` builds
the whole tree — no other folder should need a line, the two new members being the only change to an existing
type and every existing description keeping its two-argument macro.
