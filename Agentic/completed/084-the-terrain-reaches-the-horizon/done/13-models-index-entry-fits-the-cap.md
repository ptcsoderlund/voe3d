# 13 — The models.h index entry fits the cap
folder: 3d/include/3d
after: none
decisions: 0168

## Change
Doc only; no code changes.

`3d/include/3d/3d.md`: the entry `models.h` is 330 characters, over the
300 cap. Cut it to one sentence under 300: the store of loaded models keyed
by path, loaded once and reloaded on change, and what walks it. Drop the
list of extras (blended twin, pictures, soft dot, the landscape's heights
texture written a budget a frame) from the entry.

`3d/include/3d/models.h`: read its header comment only. Every point dropped
from the entry must already be in it; the landscape's heights written a
budget a frame is the one to check. If one is missing, add it to the header
as a short paragraph in the header's existing style. Touch nothing below
the header.

## Done when
`checks.sh --folder 3d/include/3d` reports no finding naming `models.h`.
