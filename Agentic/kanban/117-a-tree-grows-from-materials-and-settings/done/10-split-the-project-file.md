# 10 — Split the project file
folder: editor
after: 09
decisions: 0168

## Change
`editor/src/project.c` is 799 lines, and card 11 adds to it. Split it by function, with no change in
behaviour: the prefab's open and Back (`voe_editor_project_prefab_open`, `_prefab_back` and their helpers)
move to a new `editor/src/project_prefab.c`, with a header comment.

`editor/src/project.h` is unchanged. Update the entries in `editor/src/src.md`.

## Done when
`cmake --build --preset debug --target voe_editor` exits 0, and `editor/src/project.c` is under 700 lines.
