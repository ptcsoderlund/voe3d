## Learned
- 117's suite failure was one analyser finding in `assets/tests/keys.c` (from 116), a true path because `VOE_TEST_CHECK` carries on: a whole-tree analyser replay over `build/debug`'s compile database showed it as the only finding.
- The analyser step can be replayed for one file from `build/debug/compile_commands.json` with jq and sed (drop `-o`, `-c`, `-MD`, `-MT`, `-MF`; add `--analyze -Xanalyzer -analyzer-output=text -Wno-unused-command-line-argument`), which proves an analyser fix on a card without naming `check.cmake`.
