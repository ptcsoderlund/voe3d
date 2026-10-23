# 13 — whole suite failed
folder: .

## Change
Make the whole suite pass. The findings below say what is wrong and where.
Split this into cards, one per folder named, in the order the folders depend
on each other.

## Done when
`checks.sh --all` prints FINDINGS: 0.

## Blocked
FINDING check: failed: [ -f ~/voe3d-scratch/env.sh ] && . ~/voe3d-scratch/env.sh; cmake -P check.cmake
          [557/884] Building C object game/CMakeFiles/voe_test_game_project.dir/tests/project.c.o
          [558/884] Building C object dev/CMakeFiles/voe_dev.dir/src/quad.c.o
          [559/884] Building C object dev/CMakeFiles/voe_dev.dir/src/monitor.c.o
          [560/884] Building C object dev/CMakeFiles/voe_dev.dir/src/readout.c.o
          [561/884] Building C object dev/CMakeFiles/voe_dev.dir/src/surface.c.o
          [562/884] Building C object game/CMakeFiles/voe_test_game_steps.dir/tests/steps.c.o
          [563/884] Building C object game/CMakeFiles/voe_test_game_world.dir/tests/world.c.o
          [564/884] Building C object dev/CMakeFiles/voe_dev.dir/src/sprites.c.o
          [565/884] Building C object game/CMakeFiles/voe_test_game_frame.dir/tests/frame.c.o
          [566/884] Building C object dev/CMakeFiles/voe_dev.dir/src/text.c.o
          [567/884] Building C object dev/CMakeFiles/voe_dev.dir/src/main.c.o
          [568/884] Building C object game/CMakeFiles/voe_test_game_starting.dir/tests/starting.c.o
          [569/884] Building C object dev/CMakeFiles/voe_dev.dir/src/startup.c.o
          ninja: build stopped: subcommand failed.
          
    CMake Error at check/report.cmake:38 (message):
      check failed at: root configure and build
    Call Stack (most recent call first):
      check/build.cmake:45 (step_fail)
      check.cmake:57 (include)
FINDINGS: 1
