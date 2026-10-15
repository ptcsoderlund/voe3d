# 45 — The levels test reads no unset level count
folder: 3d/tests
after: none
decisions: 0168

## Change
`check.cmake`'s analyser step fails on `3d/tests/model_levels.c`, in
`levels_asked()` (line ~107): claim 7 reads `medium.level_count` at line
~129, but `VOE_TEST_CHECK` does not stop the test, so when `sphere_made()`
returns false `medium` was never written ("The left operand of '==' is a
garbage value"). The analyser is right; no suppression.

Fix it in `levels_asked()` only: declare `levels` and `medium` initialised
empty (`= {}`), so a failed make reads as zero levels and the check fails
cleanly instead of reading garbage. Check the rest of the file for the same
shape (a `voe_3d_model_levels` read after a check that may have failed) and
fix it the same way. The claims and the header comment do not change.

## Done when
`ctest --test-dir build/debug -R "^3d/model_levels$"` passes, and `clang --analyze
-Xanalyzer -analyzer-output=text`, run as `check/analyser.cmake`'s
`analyse_entry` runs it (the file's entry in
`build/debug/compile_commands.json`), prints no warning on
`3d/tests/model_levels.c`.
