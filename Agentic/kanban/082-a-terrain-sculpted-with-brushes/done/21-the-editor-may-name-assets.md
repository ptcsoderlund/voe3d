# 21 — The editor may name assets
folder: cmake
after: none
decisions: 0168, 0380

## Change
`cmake/voe.cmake`, `voe_allowed_deps()`, the `editor` branch: add `assets` to
the `set(deps ...)` row. Rewrite the comment above it so it no longer says assets is
absent: the editor names assets to read, make and resize `.landscape` files (0380);
it still decodes no glTF, image or sound itself; sprite stays absent and a card that
wants it is a decision. Keep the rest of the comment's points.
`cmake/cmake.md` needs no change unless its `voe.cmake` entry turns out wrong.

## Done when
`grep -A30 'folder STREQUAL "editor"' cmake/voe.cmake | grep -E '^ *set\(deps .*\bassets\b'`
exits 0.
