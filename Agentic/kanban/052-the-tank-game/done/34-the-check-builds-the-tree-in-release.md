# 34 — The check builds the tree in Release
folder: check
after: 33
decisions: 0168, 0340

## Change
0340: the suite builds the whole tree with the `release` preset's settings once per feature, so a
Release-only break (bug 04: a debug assert's expression rejected at `-O3 -DNDEBUG`) is caught
before the human tests, not when someone ships. Read `check/build.cmake`, `check/check.md` and
`CMakePresets.json`.

- `check/build.cmake`: a new step after step 3, `3b root release build`. It configures the root
  (`-S "${root}"`) into `"${checkdir}/release"` with `${common} -DCMAKE_BUILD_TYPE=Release`, the
  same settings the `release` preset gives (Ninja, clang, Release), then builds it, both through
  `run_capture`; on either failure it calls `step_fail` with the output, else `step_ok`. It
  builds every target, tests included, and runs no tests. The header gains its points: why
  Release is built (bug 04, 0340), why the whole tree once per feature and not per card (the
  folder check stays Debug), and why its own folder under `build/check/` and not
  `build/release`.
- `check/check.md`: the `build.cmake` entry says it also builds the tree in Release.

No other step changes.

## Done when
`grep -q 'CMAKE_BUILD_TYPE=Release' check/build.cmake`,
`grep -q 'checkdir}/release' check/build.cmake` and
`grep -q '3b root release build' check/build.cmake` each exit 0.
