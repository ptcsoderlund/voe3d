# 002 Fields of any shape — acceptance

Nothing in the editor or the dev program changes: no real component has an array field yet, and
the inspector does not show them (both out of scope). The feature is proven by test components,
so you try it through the tests, their expected texts, and two builds you break on purpose.

## Start it

1. Configure, build, and run every check once:

       cmake --preset debug
       cmake -P check.cmake

   Expected: it exits zero.

## Try this

1. **An array field is laid out exactly as C would lay it out, and nothing else moved** — run
   the layout tests:

       cmake --build --preset debug
       ctest --test-dir build/debug -R 'base/describe|scene/identity'

   Expected: both pass. `base/tests/describe.c` (lines 80–90) declares a 4-by-2 grid of vectors
   and eight 32-byte names, and the test compares them against the plain C struct.

2. **A bad declaration fails the build with a readable message** — in `base/tests/describe.c`,
   change line 88 from `F(int32_t, one, INT32)` to `F(int32_t, one, INT32, 0)` and build:

       cmake --build --preset debug --target voe_test_base_describe

   Expected: the build fails with `zero size arrays are an extension`. Put the line back, then
   change `F(vector, grid, FLOAT3, 4, 2)` on line 84 to `F(vector, grid, FLOAT2, 4, 2)` and build
   again. Expected: `the declared type is not the size of FLOAT2`. Then restore the file:

       git checkout base/tests/describe.c

3. **A scene saves arrays as nested brackets** — open `authoring/tests/scene_write.c` at line 420.
   Expected: the text the test demands, including `pair = [[1, 2, 3], [4, 5, 6]]`,
   `tags = ["a", "x\"y\\z", ""]`, a grid three brackets deep and `deepest` eight deep. Run:

       ctest --test-dir build/debug -R authoring

   Expected: both authoring tests pass, so the writer produces exactly that text.

4. **Saving then loading, and loading then saving, change nothing** — covered by the same run
   (`test_shapes_round_trip_text_first` and `_world_first` in `authoring/tests/scene_read.c`).
   Expected: it passed in step 3.

5. **A wrongly shaped array refuses the whole file** — `authoring/tests/scene_read.c` from
   line 438 lists each case: a short row, a flattened list, a mixed list, a bad escape, a string
   too long, nine levels of brackets, an empty list. Expected: each one refuses, leaving no
   entities; it passed in step 3. To see a refusal bite, change `[4, 5]` on line 443 to
   `[4, 5, 6]` and rerun step 3's `ctest`. Expected: `authoring/scene_read` now fails. Restore:

       git checkout authoring/tests/scene_read.c

6. **Extra spaces load and are written back tidy** — `test_shapes_tolerated_spacing` in the same
   file reads `pair = [ [1,2,3] ,[4, 5,6] ]` and expects the canonical spelling back. Expected:
   it passed in step 3.

7. **Existing scenes are unchanged** — the older writer test's three-entity text in
   `authoring/tests/scene_write.c` was not edited. Expected: it passed in step 3.

8. **The checks pass** — `cmake -P check.cmake` exits zero, as in Start it.

## Results
