# 20 — The key table test stops on an empty table
folder: assets/tests
after: none
decisions: 0168, 0511, 0512

## Change
Step 7 (the analyser) of the release check finds one null dereference, in
`assets/tests/keys.c`, and it is a true path, not a false alarm: the row walker
checks `rows != NULL && count > 0` (line 66) with `VOE_TEST_CHECK`, which only
counts a failure and carries on (`testing/include/testing/test.h`, line 15), and
then indexes `rows[i]` for `i < count` anyway (lines 67–71).

In `assets/tests/keys.c`, in the function holding lines 66–71: keep the check,
then return before the loop when `rows` is NULL or `count` is 0, so a failed
check is reported and nothing after it reads the table. No suppression
(`__clang_analyzer__`): the code is wrong, not the analyser. Do not touch the
accessors `voe_assets_material_keys` / `voe_assets_parts_keys` or the table
shape (0511/0512); they never return NULL, but the test cannot see that.

If any other function in `keys.c` checks a pointer with `VOE_TEST_CHECK` and
then reads through it, give it the same early return. If the header comment
says nothing on what a failed table check does, add one point: the walk stops
there.

## Done when
From the repo root, after the folder check has configured `build/debug`, this
exits 0 (the analyser has no finding on `keys.c`):

    n=$(cd build/debug && cmd=$(jq -r '[.[] | select(.file | endswith("assets/tests/keys.c"))][0].command' compile_commands.json | sed -E 's/ -o [^ ]+//; s/ -c / /; s/ -MD//; s/ -MT [^ ]+//; s/ -MF [^ ]+//') && eval "$cmd --analyze -Xanalyzer -analyzer-output=text -Wno-unused-command-line-argument" 2>&1 | grep -c 'warning:'); test "$n" = 0
