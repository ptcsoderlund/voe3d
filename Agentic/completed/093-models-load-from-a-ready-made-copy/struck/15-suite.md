# 15 — whole suite failed
folder: .

## Change
Make the whole suite pass. The findings below say what is wrong and where.
Split this into cards, one per folder named, in the order the folders depend
on each other.

## Done when
`checks.sh --all` prints FINDINGS: 0.

## Blocked
FINDING check: failed: [ -f ~/voe3d-scratch/env.sh ] && . ~/voe3d-scratch/env.sh; cmake -P check.cmake
    ok    guard version
    ok    guard map
    ok    includes
    ok    tests (232 passed — 3d 70, app 7, assets 14, audio 4, authoring 10, base 5, dev 0, ecs 4, editor 0, game 9, math 6, physics 4, platform 14, render 60, scene 11, sprite 1, text 3, theme 1, ui 9)
    ok    harness reports a failure
    ok    descriptions off (scene, 11 passed)
    FAIL  analyser
          /mnt/dev/GitRepos/Github/voe3d/game/src/models.c:367:3: warning: Value stored to 'going' is never read [deadcode.DeadStores]
            367 |                 going = load_from(&call, world, &kinds, folder, paths, &stopped);
                |                 ^       ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
          /mnt/dev/GitRepos/Github/voe3d/game/src/models.c:367:3: note: Value stored to 'going' is never read
            367 |                 going = load_from(&call, world, &kinds, folder, paths, &stopped);
                |                 ^       ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
          1 warning generated.
          
    CMake Error at check/report.cmake:38 (message):
      check failed at: analyser
    Call Stack (most recent call first):
      check/analyser.cmake:150 (step_fail)
      check.cmake:61 (include)
FINDINGS: 1
