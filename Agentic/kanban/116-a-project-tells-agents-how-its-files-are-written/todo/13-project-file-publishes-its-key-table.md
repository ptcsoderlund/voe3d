# 13 — The project file publishes its key table
folder: authoring
after: 11
decisions: 0168, 0511, 0512

## Change
- `authoring/include/authoring/project.h`: include `<assets/sectioned.h>`. Declare
  `const voe_assets_documented_key *voe_authoring_project_keys(uint32_t *count);`. The header points to
  the table.
- `authoring/src/project.c`: add a static table and the function. Its rows:
  - [project] `scene`, required (default "");
  - [window] `width` and `height`, ranging over `VOE_AUTHORING_PROJECT_WINDOW_MIN`–`_MAX`, with the
    header's defaults (1280 and 720) spelled as text;
  - [window] `fullscreen`, default false.

  Set `refused` from how this file's strict read behaves.
- `authoring/authoring.md`: add the table's index entry.
- `authoring/tests/project.c`: add the test `project_keys_read_strictly`:
  - `[project]` with `scene = "a.scene"`, plus each row that has a default set to that default under
    its section, reads through `voe_authoring_project_read_strict`;
  - when the width row says `refused`, a width above greatest is refused.

## Done when
`ctest --test-dir build/debug -R '^authoring/project$' --no-tests=error` exits 0.
