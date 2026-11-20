# 03 — `project.voe3d` read strictly
folder: authoring
after: 02
decisions: 0168, 0505, 0506, 0509

## Change
- `authoring/include/authoring/project.h`, `authoring/src/project.c`: add
  `voe_authoring_project_read_strict`, with the signature of `voe_authoring_project_read`.
  - It refuses everything that reader refuses.
  - It also refuses:
    - a key in `[project]` that the reader does not take (today a warning);
    - a section the reader does not read;
    - a window width or height outside `VOE_AUTHORING_PROJECT_WINDOW_MIN` to `_MAX`.
  - Each refusal is reported naming the line, the key and why.
  - One body serves both readers.
  - Header: why there is a strict reader (0509 point 3).
- `authoring/tests/project.c`: add
  - `strict_refuses_an_unknown_key`, which also checks that the kept report names the key and its
    line;
  - `strict_refuses_a_window_out_of_range`;
  - `strict_reads_what_write_writes`.
- `authoring/tests/tests.md`: the `project.c` entry.

## Done when
`ctest --test-dir build/debug -R '^authoring/project$'` exits 0, and its output includes the three
new tests.
