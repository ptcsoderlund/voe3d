# 27 — A field's names are bound at run time
folder: base
decisions: 0168, 0284, 0245

## Change
Bug: on Windows every file of `examples/tank_game/Code/` fails with "initializer
element is not a compile-time constant" at `VOE_BASE_DESCRIBE_STRUCT_NAMED` in
`3d/include/3d/shape_component.h` and `physics/include/physics/collider_component.h`.
Their names arrays are `VOE_BASE_IMPORTED` (dllimport under `VOE_BASE_IMPORTING`),
and `base/include/base/describe.h` puts that address in a static initialiser.
Fix it once, in base; neither of those headers changes.

`base/include/base/describe.h`, the descriptions-on branch only:
- `VOE_BASE_DESCRIBE_NAMES_ROW_`: the row's initialiser keeps `.field` and
  `.value_count` (both constants: `sizeof` of an array lvalue) and no longer
  sets `.values`.
- A new expansion-only macro, invoked through the names list a second time
  inside the accessor, that stores each row's `values_` into its row in order
  (a local index counting up is enough).
- `VOE_BASE_DESCRIBE_TABLE_NAMED_`: `named` becomes `static` without `const`;
  the accessor runs the store step before returning. `rows` and `description`
  stay `static const` (`.names = named` is the address of a local static, a
  constant).
- Header comment, at the NAMES paragraph or the "Everything from here down"
  part: a names array's address is stored when the accessor runs, never in an
  initialiser, because an imported address is not a constant (0245, 0284);
  the accessor is called from one thread at a time.
- `voe_base_field_names`, `voe_base_names_find` and the plain table do not
  change.

`base/tests/describe.c`: a second named struct whose names argument is an
array lvalue that is not an address constant: a `static` pointer to an array
of names, dereferenced in its names list (`N(field, (*pointer))` style), so
`sizeof` still sees the bound but the value is a load. The old expansion
refused to compile exactly that, as dllimport data is refused on Windows. A
test function checks that its description's names come back: the field
spelled right, the count the array's bound, each entry equal to the array's,
and a second call returning the same. One paragraph in the file's header
comment says why the pointer: it is Linux's stand-in for imported data.

`base/base.md`: the `describe.h` entry gains a phrase that names are bound when
asked for, only if it still fits the entry's cap.

## Done when
`ctest --test-dir build/debug -R "^base/describe$"` passes with the new test
function, and reverting the `describe.h` change makes `voe_test_base_describe`
fail to compile (the coder checks this once and restores the change).

Human, on Windows: build the editor from this branch, open `examples/tank_game`;
the project's code builds (no error in `examples/tank_game/Build/build.log`),
and Play runs with shells and enemy tanks as in `feature.md` step 5.
