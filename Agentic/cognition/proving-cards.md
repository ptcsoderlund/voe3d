# Proving cards

What can show a card's done-when is met: what the tests print, and what proves an editor card.

## Tests
- The test harness prints no test names: ctest shows one line per suite (such as `assets/material`), and with `-V` a new test shows only through what it prints, such as its refusal lines. A done-when that names tests in the ctest output cannot be met as written. [observed · coder 115-01 · 2026-11-06]

## The editor
- The editor has no tests; an editor card is proved by grep and the `--capture` run. [observed · planner 115 · 2026-11-06]
- On this WSL machine an editor `--capture` aborts on a Vulkan validation error, a storage buffer range of 346977280 past the device's limit of 134217728, after `Docs/` is written. [observed · coder 117-24 · 2026-11-24]
- No coder runs Play or the game: a how-to-test that needs a person in the editor and a running game is seen by no agent, only the build and the folder's checks, as with 117's card 18. [observed · coder 117-18 · 2026-11-24]

## The analyser
- The analyser does not report every check-then-use-anyway in tests: authoring's reference test checks `out.text` against NULL, then passes it to `strstr` regardless; left as it is after 116. [observed · planner 116-20 · 2026-11-20]
- `VOE_TEST_CHECK` carries on after a failed check, so a check-then-use in a test is a true path; 117's suite failed on one such finding in `assets/tests/keys.c`, while that file's `check_rows` reads a checked key unguarded and is not flagged. [observed · planner 117-19 · 2026-11-24]
- An analyser fix is proved on a card by replaying the analyser for one file from `build/debug`'s compile database, without naming `check.cmake`. [observed · planner 117-19 · 2026-11-24]
