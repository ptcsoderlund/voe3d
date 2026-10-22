# 36 — A refused prepare says why once, and the fir tree imports
folder: editor
after: 35
decisions: 0168, 0443, 0444, 0445

## Change
0445 points 3, 4 and 6 for Make LODs, the reload and the editor's own re-reads; the last card of
bug 01.

- `editor/src/loading.h` — `voe_editor_loading_prepare` gains `bool refused` (the prepare refused
  the `.glb`; its cause has its line) and a `voe_editor_notice refusal`; the prepare work's comment
  says a refusal is set there, not a line of its own.
- `editor/src/loading.c` — the prepare work clears the kept error before
  `voe_game_models_prepare`; on false it sets `refused` and builds `refusal` with
  `voe_editor_notice_from_report` on the worker (the kept error is the worker thread's); the
  "its levels were not made" line goes.
- `editor/src/main.c` — after the prepare's wait: a refused prepare puts `refusal` in the session's
  notice and skips `voe_editor_models_reload`; otherwise as today. The block's comment says so.
- `editor/src/models.c` —
  - `say_failure`: the notice is the kept line whole through `voe_editor_notice_from_report` on
    the first path (it names the file), not its last phrase after a colon.
  - `reread`: the path pushed as the subject around its read and load; "could not read %s again:
    %s" goes where the load refused (its cause has a line); a file that will not read keeps one
    line under the subject, no path in the message.
- `editor/src/src.md` — the `loading.c` and `models.c` entries.

## Done when
`! grep -q "its levels were not made" editor/src/loading.c` exits 0 and
`grep -q "refused" editor/src/loading.h` exits 0.

The human's, in the editor on the lumberhill project:
1. Import `Assets/Lanscapestuff/fir_tree_01_4k.glb` (bug 01's file). No error line; behind the
   splash its LODs are made (about a minute in a debug build). Place it in the scene; the
   Inspector's LODs rows count LOD 1 to LOD 6.
2. Pick it in the foliage brush with another tree and a rock and paint a slope (090 step 1). From
   the hilltop the far firs draw their simple LODs (090 step 6).
3. Cut a copy short outside the project (`head -c 100000 fir_tree_01_4k.glb > broken.glb`) and
   import it: exactly one `error:` line on stderr, naming `Assets/…/broken.glb`, saying it is
   damaged and to export it again; the top bar's notice says the same; no LODs question.
