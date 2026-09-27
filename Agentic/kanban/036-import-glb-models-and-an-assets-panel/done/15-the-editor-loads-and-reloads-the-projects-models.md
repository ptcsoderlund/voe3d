# 15 — The editor loads and reloads the project's models
folder: editor
decisions: 0168, 0277

## Change
Needs cards 12 and 14 (0277 points 4, 5 and 9).

- `editor/src/models.h`, `editor/src/models.c`: `void voe_editor_models_update(voe_editor_models
  *, voe_editor_session *session, voe_render_device *device, voe_base_arena *scratch, double
  now);` — once a frame, outside any draw: when the session's project folder differs from the
  one it last read against (a new or opened project, or untitled becoming saved), clear the
  store first; with no folder (untitled) do nothing more; otherwise `voe_game_models_update`
  against the project folder, and when `now` is a second past the last look,
  `voe_game_models_watch`. A call that reports failures sets the session notice to
  `Could not read <path>` with the category, through `notice.h`. The struct keeps its own copy of
  the folder it read against. Header points: why between frames; why a second; what the notice
  says and that the thing draws as nothing.
- `editor/src/main.c`: one call a frame, after the world step and before the frame's draw
  opens, with the frame clock's now. Keep it under 800 lines.
- `editor/src/src.md`: the `models` lines say it loads, re-reads and empties.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_editor` exits 0, and this
exits 0 (a scratch copy of the capsule example with the dev program's model placed, then a
text file named `.glb` placed):
```
d=$(mktemp -d) && cp -r examples/capsule "$d/p" && rm -rf "$d/p/Build" && mkdir "$d/p/Assets" &&
cp dev/src/textured_primitives_human.glb "$d/p/Assets/human.glb" &&
printf '\n[90]\nname = "Human"\n[90.voe_3d_model]\npath = "Assets/human.glb"\n[90.voe_scene_transform]\nposition = [0, 0, 0]\nrotation = [0, 0, 0, 1]\nscale = [1, 1, 1]\n' >> "$d/p/main.scene" &&
build/debug/editor/voe_editor "$d/p" --capture "$d/a.png" 2> "$d/a.txt" &&
! grep -q "human.glb" "$d/a.txt" && echo text > "$d/p/Assets/broken.glb" &&
sed -i 's|Assets/human.glb|Assets/broken.glb|' "$d/p/main.scene" &&
build/debug/editor/voe_editor "$d/p" --capture "$d/b.png" 2> "$d/b.txt" &&
grep -q "broken.glb" "$d/b.txt"
```
If the scene reader refuses id 90 or the section spelling, match `examples/capsule/main.scene`.
