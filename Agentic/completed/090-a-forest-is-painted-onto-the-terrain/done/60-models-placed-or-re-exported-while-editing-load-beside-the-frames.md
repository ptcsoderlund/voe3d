# 60 — Models placed or re-exported while editing load beside the frames
folder: editor
after: 58, 59
decisions: 0168, 0386, 0447, 0448

## Change
0448 point 5 in the editor; the last card of bug 02.

- `editor/src/models.c` —
  - the editor's models hold a `voe_game_models_loader` (card 58), made in `_new` and destroyed
    in `_destroy` before the pool and the store.
  - `voe_editor_models_update` with no progress (between frames): `voe_game_models_loader_step`
    in place of `voe_game_models_update` and `voe_game_models_watch`, `watch` true when the
    once-a-second watch is due, its failures to the notice as now; the materials call and the
    rest after it as now. With a progress (a splash wait's worker): update and watch as now.
  - Header: BETWEEN FRAMES says the files are read and baked beside the frames by the loader,
    one put between two frames each frame, and a splash wait still loads whole; ONCE A SECOND
    says the watch goes through the loader too.
- `editor/src/src.md` — the `models.c` entry.

## Done when
`grep -q "voe_game_models_loader_step" editor/src/models.c` exits 0 and
`build/debug/editor/voe_editor examples/tank_game --capture "$(mktemp -d)/t.png" 2>&1 | grep -q
"^game: models: "` exits 0.

The human's, on the lumberhill project (`~/Projekt/voe3dprojects/lumberhill/lumberhill_project`):
1. `cmake --build --preset release --target voe_editor`, then
   `build/release/editor/voe_editor ~/Projekt/voe3dprojects/lumberhill/lumberhill_project
   --capture /tmp/x.png`: `editor: scene and shapes` is under 3 s (was 9.7 s); a `game: models:`
   and a `game: materials:` line give each phase's time; `top` beside it shows most cores busy.
   Run it again: the second start is as fast as the first, no faster.
2. In the editor, import `Assets/Lanscapestuff/tree asset.glb` again and time it: many times
   faster than before; its `game: models:` line prints.
3. Drag the tree from Assets into a view: the frames go on while it loads (the frame panel shows
   no long frame but its upload), then it appears. Re-export it from Blender (or `touch` it):
   it reloads the same way.
4. Import bug 01's broken copy (`head -c 100000 fir_tree_01_4k.glb > broken.glb`): exactly one
   `error:` line naming it, saying it is damaged, as before (0445); no LODs question.
5. In a debug build, import `fir_tree_01_4k.glb` into a new folder of the project: its LODs are
   made with every core busy, well under the minute card 36 saw.
