# 02 — A shape's kind is editable and its kinds are named
folder: 3d
decisions: 0168, 0191, 0195, 0198

## Change
`3d/include/3d/shape_component.h`:

- `kind` moves from `F_READ_ONLY` to `F` in `VOE_3D_SHAPE_FIELDS`. The struct is byte-for-byte what it was.
- A names array declared here and defined in the `.c`:

		// The name of each kind, indexed by the kind's own number; NULL
		// for nought, which is no kind.
		extern const char *const voe_3d_shape_kind_names[4];

  and the names list beside the field list:

		#define VOE_3D_SHAPE_NAMES(N) N(kind, voe_3d_shape_kind_names)

- `VOE_BASE_DESCRIBE_STRUCT(voe_3d_shape, VOE_3D_SHAPE_FIELDS)` becomes
  `VOE_BASE_DESCRIBE_STRUCT_NAMED(voe_3d_shape, VOE_3D_SHAPE_FIELDS, VOE_3D_SHAPE_NAMES)` (base/describe.h,
  card 01).
- The header's `KIND IS READ-ONLY` paragraph is replaced by one saying that KIND IS CHOSEN FROM ITS THREE NAMES
  (0195): a tool shows it as a dropdown of `voe_3d_shape_kind_names` and a chosen one reaches the row through
  the shape's intent like the colour does, so the entity draws that kind from the next run on
  (3d/shape_system.h); that the names are indexed by the kind's own number, which is why entry nought is NULL;
  and that a kind this build does not know is still drawn as nothing with a warning, because a file may be newer
  than the build. The `THE SHAPE'S INTENT ... puts kind back to the entity's own` sentence goes with it.

`3d/src/shape_component.c`: define `const char *const voe_3d_shape_kind_names[4] = { NULL, "Cube", "Capsule",
"Cylinder" }`, in the kinds' own order, with one line saying the index is the kind. Nothing else in the file
changes — the table, its registration, its default row and its reads are what they were.

`3d/tests/shape.c`: the claim that `kind` is read-only becomes the claim that it is not, and add beside it that
`voe_3d_shape_description()` has one names entry, that `voe_base_names_find(description, "kind")` has
`value_count` 4 with `values[VOE_3D_SHAPE_CUBE]` "Cube", `values[VOE_3D_SHAPE_CYLINDER]` "Cylinder" and
`values[0]` NULL, and that `colour` has no names entry. This is the table half of the test, which needs no
graphics card.

`3d/3d.md`: the `include/3d/shape_component.h` line says a kind chosen from its three names instead of "a
read-only kind", and "why kind is read-only" becomes why the kinds are named and numbered from one.
`3d/tests/tests.md`: `shape.c`'s line says kind is described, editable and named.

## Done when
`checks.sh` for `3d` exits 0 with the changed and added checks in `3d/shape`, and `cmake --build --preset debug`
builds the whole tree.
