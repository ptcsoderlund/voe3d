# testing

The check macros a test program uses, and nothing else. Not a folder in the
dependency map and not a library: it has no `CMakeLists.txt`, so folder
discovery never sees it, and tests reach it as `voe::testing`.

- `include` — the public header, in `include/testing/`. See
  `include/testing/testing.md`.
