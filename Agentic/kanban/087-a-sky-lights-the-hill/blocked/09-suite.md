# 09 — whole suite failed
folder: .

## Change
Make the whole suite pass. The findings below say what is wrong and where.
Split this into cards, one per folder named, in the order the folders depend
on each other.

## Done when
`checks.sh --all` prints FINDINGS: 0.

## Blocked
FINDING check: failed: [ -f ~/voe3d-scratch/env.sh ] && . ~/voe3d-scratch/env.sh; cmake -P check.cmake
          [312/776] Building C object render/CMakeFiles/voe_render.dir/src/sky_draw.c.o
          [313/776] Building C object render/CMakeFiles/voe_render.dir/src/pass.c.o
          [314/776] Building C object render/CMakeFiles/voe_render.dir/src/target_own.c.o
          [315/776] Building C object render/CMakeFiles/voe_render.dir/src/swapchain.c.o
          [316/776] Building C object text/CMakeFiles/voe_text.dir/src/font.c.o
          [317/776] Building C object render/CMakeFiles/voe_render.dir/src/shadow.c.o
          [318/776] Building C object render/CMakeFiles/voe_render.dir/src/texture_heights.c.o
          [319/776] Building C object render/CMakeFiles/voe_render.dir/src/target.c.o
          [320/776] Building C object render/CMakeFiles/voe_render.dir/src/texture_levels.c.o
          [321/776] Building C object render/CMakeFiles/voe_render.dir/src/target_read.c.o
          [322/776] Building C object render/CMakeFiles/voe_render.dir/src/texture.c.o
          [323/776] Building C object text/CMakeFiles/voe_text.dir/src/truetype.c.o
          [324/776] Building C object text/CMakeFiles/voe_text.dir/src/raster.c.o
          ninja: build stopped: subcommand failed.
          
    CMake Error at check/report.cmake:38 (message):
      check failed at: 3b root release build
    Call Stack (most recent call first):
      check/build.cmake:59 (step_fail)
      check.cmake:57 (include)
FINDINGS: 1
