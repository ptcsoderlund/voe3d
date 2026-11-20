# 22 — whole suite failed
folder: .

## Change
Make the whole suite pass. The findings below say what is wrong and where.
Split this into cards, one per folder named, in the order the folders depend
on each other.

## Done when
`checks.sh --all` prints FINDINGS: 0.

## Blocked
FINDING check: failed: [ -f ~/voe3d-scratch/env.sh ] && . ~/voe3d-scratch/env.sh; cmake -P check.cmake
                |                         ^~~~
          /mnt/dev/GitRepos/Github/voe3d/assets/tests/keys.c:66:30: note: Left side of '&&' is false
             66 |         VOE_TEST_CHECK(rows != NULL && count > 0);
                |                                     ^
          /mnt/dev/GitRepos/Github/voe3d/assets/tests/keys.c:67:2: note: Loop condition is true.  Entering loop body
             67 |         for (uint32_t i = 0; i < count; i++) {
                |         ^
          /mnt/dev/GitRepos/Github/voe3d/assets/tests/keys.c:68:3: note: 'row' initialized to a null pointer value
             68 |                 const voe_assets_documented_key *row = &rows[i];
                |                 ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
          /mnt/dev/GitRepos/Github/voe3d/assets/tests/keys.c:71:18: note: Access to field 'section' results in a dereference of a null pointer (loaded from variable 'row')
             71 |                 VOE_TEST_CHECK(row->section != NULL && row->section[0] != '\0');
                |                                ^~~
          1 warning generated.
          
    CMake Error at check/report.cmake:38 (message):
      check failed at: analyser
    Call Stack (most recent call first):
      check/analyser.cmake:150 (step_fail)
      check.cmake:61 (include)
FINDINGS: 1
