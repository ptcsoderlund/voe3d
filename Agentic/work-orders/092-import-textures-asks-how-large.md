# 092 — Import textures asks how large

## What
When I right-click a `.glb` and choose Import textures, the editor asks for a largest size: 512, 1024, 2048
or 4096, with 2048 already selected. Every picture it writes (colour, normal, emission and the merged ORM) is
at most that size on its longest side. It keeps its shape and is never made bigger than it was in the
`.glb`. If I run Import textures again with another size, the pictures are replaced at that size (0453).

## Why
Pictures exported from Blender often come out at 4K. I don't want 4K everywhere: it is slow to load and
eats video memory. Assets in the project are the engine's to resize; my sources live outside the project.

## How to test
1. Export a model from Blender whose pictures are 4K, and import it.
2. Right-click it and choose Import textures. A question asks for the largest size, with 2048 selected. Pick
   1024.
3. Open the written PNGs in an image viewer. None is larger than 1024 on its longest side, and a non-square
   picture keeps its shape.
4. Look at the model in the scene. It looks like before, only less sharp up close. The normal-map bumps still
   look right, not flat or noisy.
5. Run Import textures again and pick 4096. The PNGs are back at 4K, the size the `.glb` holds, and not
   larger.
6. Import a model whose pictures are 512 and pick 2048. The pictures stay at 512.
7. Cancel the question. Nothing is written.
