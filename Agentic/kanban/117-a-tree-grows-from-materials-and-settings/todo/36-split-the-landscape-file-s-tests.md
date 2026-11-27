# 36 — Split the landscape file's tests
folder: assets
after: none
decisions: 0168

## Change
`assets/tests/landscape.c` is 906 lines and card 45 changes it. Split it by what its tests cover into
files in `assets/tests/` named for that, each under 800 lines, each with its header comment and its own
`main` as the folder's other test files have. Move tests; change none. Entries in `assets/tests/tests.md`.

## Done when
`wc -l` of `assets/tests/landscape.c` and each new file shows none over 800, and their `VOE_TEST_CHECK`
lines together count as many as `git show 7e86cb12:assets/tests/landscape.c | grep -c VOE_TEST_CHECK`.
