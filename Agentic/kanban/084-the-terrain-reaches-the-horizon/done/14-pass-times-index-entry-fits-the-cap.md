# 14 — The pass_times.c index entry fits the cap
folder: render/tests
after: none
decisions: 0168

## Change
Doc only; no code changes.

`render/tests/tests.md`: the entry `pass_times.c` is 366 characters, over
the 300 cap. Cut it to one sentence under 300: the frame breakdown, passes
and spans listed by name in order, each above nought and within the frame,
headless. Drop the per-case detail (cascade 0, the window alone without a
shadow pass, the `view window: terrain` span, capacity 1).

`render/tests/pass_times.c`: read its header comment only. It already
states each dropped case; change it only if one is missing. Touch no test
code.

## Done when
`checks.sh --folder render/tests` reports no finding naming `pass_times.c`.
