# 03 — whole suite failed
folder: .

## Change
Make the whole suite pass. The findings below say what is wrong and where.
Split this into cards, one per folder named, in the order the folders depend
on each other.

## Done when
`checks.sh --all` prints FINDINGS: 0.

## Blocked
FINDING 3d/include/3d/3d.md: missing: every code folder has <folder>.md
FINDING 3d/src/src.md: missing: every code folder has <folder>.md
FINDING 3d/tests/tests.md: missing: every code folder has <folder>.md
FINDING app/include/app/app.md: missing: every code folder has <folder>.md
FINDING app/src/src.md: missing: every code folder has <folder>.md
FINDING app/tests/tests.md: missing: every code folder has <folder>.md
FINDING assets/include/assets/assets.md: missing: every code folder has <folder>.md
FINDING assets/src/src.md: missing: every code folder has <folder>.md
FINDING assets/tests/tests.md: missing: every code folder has <folder>.md
FINDING authoring/include/authoring/authoring.md: missing: every code folder has <folder>.md
FINDING authoring/include/authoring/scene_write.h: line 98: one level of dereference only (my_ptr*, never my_ptr**)
FINDING authoring/src/src.md: missing: every code folder has <folder>.md
FINDING authoring/src/scene_write.c: line 748: one level of dereference only (my_ptr*, never my_ptr**)
FINDING authoring/tests/tests.md: missing: every code folder has <folder>.md
FINDING base/include/base/base.md: missing: every code folder has <folder>.md
FINDING base/src/src.md: missing: every code folder has <folder>.md
FINDING base/tests/tests.md: missing: every code folder has <folder>.md
FINDING dev/src/src.md: missing: every code folder has <folder>.md
FINDING dev/src/main.c: line 1425: one level of dereference only (my_ptr*, never my_ptr**)
FINDING ecs/include/ecs/ecs.md: missing: every code folder has <folder>.md
FINDING ecs/src/src.md: missing: every code folder has <folder>.md
FINDING ecs/tests/tests.md: missing: every code folder has <folder>.md
FINDING editor/src/src.md: missing: every code folder has <folder>.md
FINDING math/include/math/math.md: missing: every code folder has <folder>.md
FINDING math/src/src.md: missing: every code folder has <folder>.md
FINDING math/tests/tests.md: missing: every code folder has <folder>.md
FINDING platform/include/platform/platform.md: missing: every code folder has <folder>.md
FINDING platform/src/src.md: missing: every code folder has <folder>.md
FINDING platform/tests/tests.md: missing: every code folder has <folder>.md
FINDING render/include/render/render.md: missing: every code folder has <folder>.md
FINDING render/src/descriptors.c: line 211: one level of dereference only (my_ptr*, never my_ptr**)
FINDING scene/include/scene/scene.md: missing: every code folder has <folder>.md
FINDING scene/src/src.md: missing: every code folder has <folder>.md
FINDING scene/tests/tests.md: missing: every code folder has <folder>.md
FINDING sprite/include/sprite/sprite.md: missing: every code folder has <folder>.md
FINDING sprite/src/src.md: missing: every code folder has <folder>.md
FINDING sprite/tests/tests.md: missing: every code folder has <folder>.md
FINDING testing/include/testing/testing.md: missing: every code folder has <folder>.md
FINDING text/include/text/text.md: missing: every code folder has <folder>.md
FINDING text/src/src.md: missing: every code folder has <folder>.md
FINDING text/tests/tests.md: missing: every code folder has <folder>.md
FINDING ui/include/ui/ui.md: missing: every code folder has <folder>.md
FINDING ui/src/src.md: missing: every code folder has <folder>.md
FINDING ui/tests/tests.md: missing: every code folder has <folder>.md
FINDING check: failed: cmake -P check.cmake
    FAIL  tools
          slangc was not found. It is a source-transforming tool, so it is installed by the programmer; the Vulkan SDK is the easiest way to get one.
    CMake Error at check.cmake:70 (message):
      check failed at: tools
    Call Stack (most recent call first):
      check.cmake:130 (step_fail)
FINDINGS: 45
