# 37 — Split the store's landscape and foliage draw tests
folder: 3d
after: none
decisions: 0168

## Change
`3d/tests/models_landscape.c` (859 lines) and `3d/tests/draw_foliage.c` (869) are changed by cards 43, 48
and 49. Split each by what its tests cover into files in `3d/tests/` named for that, each under 800 lines,
each with its header comment and its own `main` as the folder's other test files have. Move tests; change
none. Entries in `3d/tests/tests.md`.

## Done when
`wc -l` of the two files and each new file shows none over 800, and for each of the two, its
`VOE_TEST_CHECK` lines and those of the files split from it count as many as
`git show 7e86cb12:3d/tests/<file>.c | grep -c VOE_TEST_CHECK`.
