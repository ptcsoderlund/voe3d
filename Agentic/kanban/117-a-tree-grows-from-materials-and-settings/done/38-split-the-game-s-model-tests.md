# 38 — Split the game's model tests
folder: game
after: none
decisions: 0168

## Change
`game/tests/models.c` is 910 lines and card 50 changes it. Split it by what its tests cover into files in
`game/tests/` named for that, each under 800 lines, each with its header comment and its own `main` as the
folder's other test files have. Move tests; change none. Entries in `game/tests/tests.md`.

## Done when
`wc -l` of `game/tests/models.c` and each new file shows none over 800, and their `VOE_TEST_CHECK` lines
together count as many as `git show 7e86cb12:game/tests/models.c | grep -c VOE_TEST_CHECK`.
