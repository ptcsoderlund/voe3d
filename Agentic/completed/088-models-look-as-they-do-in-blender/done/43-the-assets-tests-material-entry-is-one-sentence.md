# 43 — The assets tests' material.c entry is one sentence
folder: assets/tests
after: none
decisions: 0168

## Change
In `assets/tests/tests.md` the entry for `material.c` is 305 characters;
the cap is 300. Shorten it to one sentence under 300 characters: what the
test claims (defaults read from an empty section, a full round trip byte for
byte, bad values refused MALFORMED). Any detail cut from the entry that the
header comment of `assets/tests/material.c` does not already say (the new
cut-out, sides and emission keys among the defaults and the refusals) moves
into that header. No code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder assets/tests` prints no FINDING naming
`assets/tests/tests.md`.
