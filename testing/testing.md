# testing

The check macros a test program uses, and nothing else. Not a folder in the
dependency map and not a library: it has no `CMakeLists.txt`, so folder
discovery never sees it, and tests reach it as `voe::testing`.

- `include/testing/test.h` — the three checks, the failure message, and
  `voe_test_result()`. Its header explains why a test needs no registration.
