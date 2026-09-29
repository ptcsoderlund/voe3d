# 40 — Dev says its monitor shows black with no light
folder: dev
after: 37, 38
decisions: 0168, 0287

## Change
Bug 04. Only a comment changes.

`dev/src/monitor.h`, the comment on `voe_dev_monitor_frame`: the light is `3d`'s answer, the
world's one light or, with none, the zeroed light that draws lit surfaces black (0287), as the
window's frame is. Drop the 0238 wording.

## Done when
`! grep -q 0238 dev/src/monitor.h && grep -q 0287 dev/src/monitor.h` exits 0.
