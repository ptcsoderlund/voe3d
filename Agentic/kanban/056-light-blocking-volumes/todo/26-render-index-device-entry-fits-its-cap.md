# 26 — The render index entry for device.h fits its cap
folder: render/include/render
after: none
decisions: 0168

## Change
`render/include/render/render.md`: the entry for `device.h` (line ~6) is 313
characters, cap 300. Shorten it to one sentence under 300 characters naming
what the header offers; the list of pass contents (point lights, light
blockers, their kinds, the sun's mask) can be folded into one phrase. Any
detail cut that is not already in the top comment of
`render/include/render/device.h` goes there instead. No code changes; do not
split or reshape `device.h` itself.

## Done when
`checks.sh --folder render/include/render` reports no finding on `render.md`.
