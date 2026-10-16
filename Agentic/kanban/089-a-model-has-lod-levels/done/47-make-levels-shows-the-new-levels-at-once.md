# 47 — Make levels shows the new levels at once
folder: editor
after: 46
decisions: 0168, 0431

## Change
0431 points 1 and 3: after a prepare's splash wait, the model just prepared is loaded again into the
editor's store with `voe_game_models_reload` (`game/include/game/models.h`, card 46), so a new count
or strength is drawn on the next frame without reopening the project.

- `editor/src/models.h` / `models.c`: new
  `void voe_editor_models_reload(voe_editor_models *models, voe_editor_session *session,
  voe_render_device *device, const char *path, voe_base_arena *scratch)`: nothing when the session
  has no project folder or the store last read against another folder (as `follow_folder` tells);
  else `voe_game_models_reload` on the store with the session's folder; a false says the failure in
  `session->notice` the way `say_failure` does (`Could not read <path>: <reason>`). The reloaded
  model must wear its parts before it is drawn: confirm in `voe_editor_models_update` that the
  materials call runs on every update, and if it runs only after a watch that reloaded something,
  set the same condition here. Its comment: between frames, a path the store lacks left alone, why
  (0431). The header's ONCE A SECOND paragraph gains a phrase: a prepared model is also loaded again
  on asking. Scratch rewound.
- `editor/src/main.c`, the `session.prepare_due` block (around line 447): after a true wait and
  before `prepare_due` is cleared, `voe_editor_models_reload(models, &session, gpu,
  session.prepare_path, scratch)`. The block's comment says the model is loaded again so its new
  levels draw at once. 764 lines: add only these lines.
- `editor/src/loading.h`: the PREPARE sentence (line ~19) gains that main.c loads the model again
  after the wait (0431); nothing else.
- `editor/src/src.md`: the `models.h` and `models.c` entries name the reload in a phrase, each one
  sentence under 300 characters; if one would pass that, drop a detail its file's header already
  says.

## Done when
`voe_editor` builds in the folder's check with no warning, and
`grep -n 'voe_editor_models_reload' editor/src/main.c editor/src/models.c` prints lines in both.
The human, in the lumberhill project with the terrain switched off:
1. Place `buildings/wooden_stable.glb`, fly far away and read the Frame panel's Triangles.
2. Right-click it in Assets → Make levels, Levels 4 and Strong, Make: after the splash, without
   reopening, the Inspector's "of" count and the far Triangles change.
3. Make levels again with Levels 2 and Gentle: the Inspector says "of 2" and the far Triangles rise.
