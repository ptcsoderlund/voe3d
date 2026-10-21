# 02 — Importing and loading models is too slow

## Seen
Model loading is slow no matter what: even the first import of a model is too slow. It is not Play that is
slow, it is the loading itself. Iteration speed is no good like this.

Measured on the lumberhill project (`~/Projekt/voe3dprojects/lumberhill/lumberhill_project`: `wooden_stable.glb`
21 MB, `tree asset.glb` 108 MB, 4k pictures beside them and inside them, the Hill landscape), started
headless with `voe_editor <folder> --capture <png>` on a 16-core machine with an RTX 4070 Laptop GPU:

| build | scene and shapes | total start | CPU used |
|---|---|---|---|
| release | 9.6–9.8 s | 10.0–10.2 s | about 2 cores (user 20.5 s over 10.5 s wall) |
| debug | 29.8 s | 30.5 s | about 2 cores |

The start holds about 2.4 GB of memory at its peak.

## Expected
Loading a model uses all the machine's cores, and each step's time is printed, so the slow step is plain
(0447). The lumberhill start in a release build spends under 3 s in "scene and shapes" (a third of
today's), and importing a model is as many times faster. No cache of prepared models: a second load of an
unchanged model is as fast as the first, no faster. A load during editing does not hitch the frames
(0386), and import, LOD and error behaviour stays as 0443–0446 left it.

## How to reproduce
1. Build the release editor (`cmake --build --preset release --target voe_editor`).
2. Run `build/release/editor/voe_editor ~/Projekt/voe3dprojects/lumberhill/lumberhill_project --capture /tmp/x.png`.
3. Read the `editor: scene and shapes` line: about 9.7 s today. Run `top` beside it: two cores busy of 16.
4. In the editor, import `tree asset.glb` again into a project and time it.
