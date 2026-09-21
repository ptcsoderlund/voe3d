# 11 — whole suite failed
folder: .

## Change
Make the whole suite pass. The findings below say what is wrong and where.
Split this into cards, one per folder named, in the order the folders depend
on each other.

## Done when
`checks.sh --all` prints FINDINGS: 0.

## Blocked
FINDING 3d/include/3d/draw_system.h: header comment is 224 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (--header-cap to raise)
FINDING 3d/include/3d/outline.h: header comment is 73 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (--header-cap to raise)
FINDING 3d/include/3d/shape_system.h: header comment is 90 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (--header-cap to raise)
FINDING 3d/src/draw_system.c: header comment is 77 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (--header-cap to raise)
FINDING 3d/tests/tests.md: entry `draw_system.c` is 304 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING app/include/app/app.h: header comment is 87 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (--header-cap to raise)
FINDING assets/include/assets/sectioned.h: header comment is 93 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (--header-cap to raise)
FINDING authoring/include/authoring/scene_read.h: header comment is 72 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (--header-cap to raise)
FINDING authoring/include/authoring/scene_write.h: header comment is 80 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (--header-cap to raise)
FINDING base/include/base/describe.h: header comment is 94 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (--header-cap to raise)
FINDING dev/src/main.c: header comment is 555 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (--header-cap to raise)
FINDING ecs/include/ecs/component.h: header comment is 137 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (--header-cap to raise)
FINDING editor/src/inspector.h: header comment is 84 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (--header-cap to raise)
FINDING editor/src/main.c: header comment is 66 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (--header-cap to raise)
FINDING editor/src/scene.h: header comment is 86 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (--header-cap to raise)
FINDING editor/src/themes.h: header comment is 85 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (--header-cap to raise)
FINDING platform/include/platform/file.h: header comment is 73 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (--header-cap to raise)
FINDING platform/include/platform/input.h: header comment is 85 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (--header-cap to raise)
FINDING platform/src/keymap.h: header comment is 77 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (--header-cap to raise)
FINDING platform/src/window_wayland.c: header comment is 140 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (--header-cap to raise)
FINDING platform/src/window_win32.c: header comment is 103 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (--header-cap to raise)
FINDING render/include/render/device.h: header comment is 178 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (--header-cap to raise)
FINDING render/src/frame.c: header comment is 175 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (--header-cap to raise)
FINDING render/src/geometry.c: header comment is 67 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (--header-cap to raise)
FINDING render/src/target.c: header comment is 87 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (--header-cap to raise)
FINDING render/tests/elements.c: header comment is 111 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (--header-cap to raise)
FINDING render/tests/offscreen.c: header comment is 71 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (--header-cap to raise)
FINDING scene/include/scene/transform_component.h: header comment is 67 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (--header-cap to raise)
FINDING scene/include/scene/transform_system.h: header comment is 61 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (--header-cap to raise)
FINDING text/include/text/font.h: header comment is 96 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (--header-cap to raise)
FINDING text/src/font.c: header comment is 71 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (--header-cap to raise)
FINDING text/src/raster.h: header comment is 69 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (--header-cap to raise)
FINDING theme/include/theme/theme.h: header comment is 63 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (--header-cap to raise)
FINDING ui/include/ui/colour.h: header comment is 64 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (--header-cap to raise)
FINDING ui/include/ui/layout.h: header comment is 292 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (--header-cap to raise)
FINDING ui/include/ui/theme.h: header comment is 77 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (--header-cap to raise)
FINDING ui/include/ui/widgets.h: header comment is 210 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (--header-cap to raise)
FINDING ui/src/button.c: header comment is 93 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (--header-cap to raise)
FINDING ui/src/layout.c: header comment is 153 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (--header-cap to raise)
FINDING ui/src/widgets.c: header comment is 71 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (--header-cap to raise)
FINDING ui/tests/layout.c: header comment is 74 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (--header-cap to raise)
FINDINGS: 41
