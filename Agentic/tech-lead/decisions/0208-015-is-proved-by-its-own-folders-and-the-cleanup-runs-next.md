# 0208 — 015 is proved by its own folders; the cap cleanup runs next
date: 2026-09-21
by: tech-lead

## Decision
Feature 015-a-move-gizmo alone is exempt from proving its last card with `checks.sh --all`. Its last card is
proved by `checks.sh --folder 3d`, `checks.sh --folder editor` and the whole ctest suite passing. 015 still
clears every finding that is its own: the `platform/window.h` include in `3d/include/3d/gizmo.h` and
`3d/tests/gizmo.c` is dropped (`voe_platform_size` already arrives through `render/device.h`), `gizmo.h`'s
header comes under the cap, `3d/include/3d/3d.md` lists `gizmo.h`, and the `gizmo.c` entry in `3d/src/src.md`
comes under the entry cap. The findings older than 015 (header comments and entries over their caps in about
14 folders) are cleared by a cleanup feature that runs immediately after 015 and before any other work order.
From the feature after that cleanup, every feature's last card is proved by `checks.sh --all` again, with no
exceptions.

## Reasoning
015 was nearly ready for a test and the debt is not its own; the workflow runs one feature at a time, so a
cleanup first would have parked 015 mid-flight. Alternatives: the cleanup before 015 (holds a ready feature
behind ~14 cards of prose), raising the caps (gives up the small reads the caps exist for).

## Replaces
nothing
