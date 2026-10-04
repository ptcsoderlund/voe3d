# 14 — The editor src index entry for view_passes.c fits its cap
folder: editor/src
after: none
decisions: 0168

## Change
`editor/src/src.md`: the entry for `view_passes.c` (line ~210) is 314
characters, cap 300. Shorten it to one sentence under 300 characters. Any
detail cut that is not already in the top comment of
`editor/src/view_passes.c` goes there instead (keep that comment under 60
lines). No code changes.

## Done when
`checks.sh --folder editor/src` reports no finding on `src.md`.
