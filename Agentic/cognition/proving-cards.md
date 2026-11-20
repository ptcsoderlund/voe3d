# Proving cards

What can show a card's done-when is met: what the tests print, and what proves an editor card.

## Tests
- The test harness prints no test names: ctest shows one line per suite (such as `assets/material`), and with `-V` a new test shows only through what it prints, such as its refusal lines. A done-when that names tests in the ctest output cannot be met as written. [observed · coder 115-01 · 2026-11-06]

## The editor
- The editor has no tests; an editor card is proved by grep and the `--capture` run. [observed · planner 115 · 2026-11-06]
