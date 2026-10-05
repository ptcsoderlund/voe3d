# 13 — render tests' card.c entry within the cap
folder: render/tests
after: none
decisions: 0168

## Change
`render/tests/tests.md`: the `card.c` entry is 379 characters; the entry cap
is 300. Cut it to one sentence under 300: which graphics card is taken, and
which vendor-tagged validation messages are dropped, on made-up facts with no
card. The list of cases it drops (discrete over integrated, fallback when the
fastest cannot present, software rasteriser alone, larger memory, no 1.3 or
drawing queue never; NVIDIA-tagged id dropped on AMD, kept on NVIDIA, untagged
kept) belongs in the header comment of `render/tests/card.c`; read that header
and add any case missing there. No code changes.

## Done when
`checks.sh --folder render/tests` reports no finding on `tests.md`'s `card.c`
entry.
