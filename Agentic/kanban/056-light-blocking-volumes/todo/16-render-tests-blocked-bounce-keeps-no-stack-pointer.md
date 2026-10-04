# 16 — blocked_bounce.c leaves no stack pointer behind; bounce_probes.c entry fits
folder: render/tests
after: none
decisions: 0168, 0347

## Change
Two findings in this folder.

1. `cmake -P check.cmake` fails at its analyser step on
   `render/tests/blocked_bounce.c`: `a_blocked_lamp()` (line ~395) builds the
   local array `house` and hands it to `block_with(s, …)` (line ~194), which
   keeps the pointer in `struct scene`; when `a_blocked_lamp` returns, `s` still
   refers to the dead stack array (clang-analyzer, "dangling reference"). Fix it
   inside `a_blocked_lamp` only: make `house` `static const` so it outlives
   the call, or end the function by blocking with an empty set
   (`(voe_render_light_blockers){ NULL, 0 }`) — whichever the analyser accepts.
   Check `a_blocked_patch()` (line ~354) for the same pattern and fix it the
   same way. The test's checks and meaning do not change.
2. `render/tests/tests.md`: the entry for `bounce_probes.c` (line ~59) is 319
   characters, cap 300. Shorten it to one sentence under 300 characters; any
   detail cut that is not in the top comment of `render/tests/bounce_probes.c`
   goes there (keep it under 60 lines).

## Done when
`checks.sh --folder render/tests` reports no finding, the `render/` ctest
`blocked_bounce` test still passes in it, and
`clang --analyze -Xanalyzer -analyzer-output=text` as `check.cmake`'s analyser
step runs it reports no warning on `render/tests/blocked_bounce.c` (read
`check/analyser.cmake` for the exact invocation).
