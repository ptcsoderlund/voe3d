# 01 — Strict readers for materials and landscapes
folder: assets
after: none
decisions: 0168, 0505, 0506, 0509

## Change
Add two readers that refuse what the lenient ones pass over. They are for a file the editor takes in
from disk (0509 point 3). The lenient readers and their callers do not change.

- `assets/include/assets/material.h`, `assets/src/material.c`: add
  `bool voe_assets_material_read_strict(const char *text, size_t size, voe_assets_material_file *out, voe_base_error *error)`.
  - It refuses everything `voe_assets_material_read` refuses.
  - It also returns MALFORMED for a section other than `[Material]`, and for a key the reader does
    not take. The retired map keys the header names are the exception and read as they do now.
  - Its stderr line names the line (the sectioned key's `line`), the key and why.
  - One body serves both readers.
  - Header: what strict adds, and why it is a second reader. A project still opens with keys an
    older or newer editor wrote; a file taken in is refused instead.
- `assets/include/assets/landscape.h`, `assets/src/landscape.c`: add
  `voe_assets_landscape_read_strict`, with the signature of `voe_assets_landscape_read`.
  - It refuses everything that reader refuses.
  - It also returns MALFORMED for a section the format does not have, and for a key no section
    takes. Its stderr line names the line and the key.
  - Change `landscape_paint.c` and `landscape_foliage.c` too wherever they take keys.
  - Header: the same points as the material reader's.
- `assets/tests/material.c`: add
  - `strict_refuses_an_unknown_key`, which also checks that the error kept by `base/report.h`
    names the key and its line;
  - `strict_takes_a_retired_map_key`;
  - `strict_reads_what_write_writes`.
- `assets/tests/landscape.c`: add `strict_refuses_an_unknown_key` and
  `strict_reads_what_write_writes`.
- `assets/tests/tests.md`: the entries for both test files.
- `assets/assets.md`: the lines for the two headers mention the strict reader.

## Meaning
- Scout: readers vary in how strict they are. The material reader passes over a key it does not
  know. The landscape reader skips unknown keys silently and already refuses many out-of-range
  values. The parts reader is strict already.

## Done when
`ctest --test-dir build/debug -R '^assets/(material|landscape)$'` exits 0, and its output includes
the five new tests.
