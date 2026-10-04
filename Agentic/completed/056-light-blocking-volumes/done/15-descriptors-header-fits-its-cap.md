# 15 — The descriptors.c header comment fits its cap
folder: render/src
after: none
decisions: 0168

## Change
`render/src/descriptors.c`: the top comment (before the first `#include`) is
63 lines, cap 60. Bring it to 60 or fewer without losing a fact: tighten the
binding 0 item (its lines wrap short: "A DYNAMIC uniform buffer: every / bind
of the set…" can join), and if more is needed, move the paragraph on why the
per-slot buffers stay mapped, or the one on the push constant, to a comment
above the function in this file that it explains. Do not raise the cap; no
code changes.

## Done when
`awk '/^#include/{print NR-1; exit}' render/src/descriptors.c` prints 60 or
less, and `checks.sh --folder render/src` reports no header finding on
`descriptors.c`.
