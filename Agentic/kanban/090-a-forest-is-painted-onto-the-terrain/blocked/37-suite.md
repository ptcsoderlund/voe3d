# 37 — whole suite failed
folder: .

## Change
Make the whole suite pass. The findings below say what is wrong and where.
Split this into cards, one per folder named, in the order the folders depend
on each other.

## Done when
`checks.sh --all` prints FINDINGS: 0.

## Blocked
FINDING 3d/tests/tests.md: entry `models.c` is 376 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING 3d/tests/tests.md: entry `models_levels.c` is 392 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING assets/src/src.md: entry `model_gltf_read.h` is 306 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING assets/src/src.md: entry `model_gltf_meshes.c` is 307 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING assets/tests/tests.md: entry `png.c` is 385 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING assets/tests/tests.md: entry `model.c` is 340 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING assets/tests/tests.md: entry `jpeg.c` is 391 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING editor/src/src.md: entry `model_import.c` is 304 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING game/include/game/models.h: header comment is 62 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (a header that is the thing itself says 'header-cap: <n>' in its own text; --header-cap raises every file's)
FINDING game/tests/tests.md: entry `models_levels.c` is 304 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING render/src/src.md: entry `geometry.c` is 345 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING render/tests/tests.md: entry `pools.c` is 375 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING check: failed: [ -f ~/voe3d-scratch/env.sh ] && . ~/voe3d-scratch/env.sh; cmake -P check.cmake
          /home/ptcsoderlund/Projekt/voe3d/assets/src/wav.c:255:6: note: Assuming the condition is false
            255 |             !read_format(&chunks, &out, &tag, error))
                |             ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
          /home/ptcsoderlund/Projekt/voe3d/assets/src/wav.c:254:2: note: Taking false branch
            254 |         if (!find_chunks(bytes, end, &chunks, error) ||
                |         ^
          /home/ptcsoderlund/Projekt/voe3d/assets/src/wav.c:257:17: note: 'tag' is not equal to WAV_TAG_PCM
            257 |         sample_bytes = tag == WAV_TAG_PCM ? 2 : 4;
                |                        ^~~
          /home/ptcsoderlund/Projekt/voe3d/assets/src/wav.c:257:17: note: '?' condition is false
          /home/ptcsoderlund/Projekt/voe3d/assets/src/wav.c:258:23: note: Division by zero
            258 |         if (chunks.data_size % (sample_bytes * out.channels) != 0)
                |             ~~~~~~~~~~~~~~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
          1 warning generated.
          
    CMake Error at check/report.cmake:38 (message):
      check failed at: analyser
    Call Stack (most recent call first):
      check/analyser.cmake:150 (step_fail)
      check.cmake:61 (include)
FINDINGS: 13
