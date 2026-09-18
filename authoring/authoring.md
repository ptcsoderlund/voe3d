# authoring

A world turned into a scene file, a scene file turned back into a world
(ADR-0151), and a project file read and written (ADR-0164). Scene text is the
format ADR-0149 names; what goes into it is what a person authored, ADR-0150.

**It needs the field descriptions, and a shipped game does not link it.** Nothing
here knows what a transform or a camera is: a component is written field by field
through the description its folder registered, and a game's own build compiles
those out (ADR-0145). So this folder is authoring-time code — the editor's, and a
tool's — and a game that turns descriptions on, to load scenes at run time, may
link it.

The writer reads components and writes none. The reader is the one exception to a
component being written only by its own system: it adds rows to entities it has
just created, and edits nothing (ADR-0152). Nothing here opens a file: `platform`
owns files, and text lands in an arena the caller hands over.

- `include` — the public headers, in `include/authoring/`. See
  `include/authoring/authoring.md`.
- `src` — the implementation. See `src/src.md`.
- `tests` — one plain C program per module, found by the build. See
  `tests/tests.md`.
