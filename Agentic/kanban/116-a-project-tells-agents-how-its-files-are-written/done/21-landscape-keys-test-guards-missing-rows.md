# 21 — The key-table tests stop reading a row they did not find
folder: assets
after: none
decisions: 0168, 0511

## Change
The analyser step of `cmake -P check.cmake` reports a NULL dereference in
`assets/tests/landscape_keys.c`. It is right: a failed `VOE_TEST_CHECK` never
stops the program (header of `testing/include/testing/test.h`), so a row the
check found missing is still read. Fix the code with a real guard; no
suppression, no change to what is checked.

- `assets/tests/landscape_keys.c`: `row_for(key)` returns NULL for a key the
  table lacks. In `landscape_keys_ranges` (NULL check near :37, then reads of
  `size`, `cells`, `kind_size`, `blend` at :39–47) and in the function holding
  the `size != NULL && cells != NULL` check near :84 (then `size->` at :85–88,
  `cells->greatest` at :91–92 and :97–98), keep each check as it is and leave
  the function right after it when any of those rows is NULL, so the failure
  is still counted and reported and nothing reads through a NULL row. Look for
  any other `row_for` result used without such a guard in the file and treat
  it the same way.
- `assets/tests/keys.c`: the same shape at :75–77, a check that
  `row->default_text != NULL` followed by a read of `row->default_text[0]`.
  Guard that read the same way. The analyser does not report it, but the read
  crashes the test instead of letting it report.

## Meaning
- Holds: the landscape key table, reached through `voe_assets_landscape_keys`, is part of 116 (0511 point 1), so `landscape_keys.c` is this feature's own test and its finding is 116's to fix.
- Holds: 0168 rule 8 says an analyser finding fails the check as a warning does. Suppress only where the analyser is wrong. Here it is right.
- Scout: `voe_test_check` only reports and counts; a failed check does not stop the test, so code after it runs with `cells` NULL.

## Done when
`checks.sh --folder assets` prints FINDINGS: 0, with the analyser step
reporting nothing in `assets/tests/landscape_keys.c` or `assets/tests/keys.c`,
and ctest passes `assets/landscape_keys` and `assets/keys`.
