# 06 — The editor shows the splash while it loads a scene
folder: editor
after: 05
decisions: 0168, 0346, 0356

## Change
New and Open's Confirm put a load off by one frame, so a splash frame is drawn before the work.

- `editor/src/session.h`, `editor/src/session.c`: `voe_editor_session` gains `load_due` and,
  for Open, the chosen folder copied into the session (the browser's memory may not outlive
  the frame). NEW once allowed and OPEN's Confirm (`voe_editor_session_browser_do`) keep their
  refusals, arming and notices as now but set `load_due` instead of putting the project in
  place. New `void voe_editor_session_load(voe_editor_session *session, voe_editor_scene *scene)`
  does what they did then (the project replaced, selection cleared, refresh and ship ended,
  `replaced` and `refresh_due` set, a failed open's notice) and clears `load_due`. The header's
  NEW and `replaced` paragraphs say the load lands a frame later, behind a splash frame (0356).
- `editor/src/main.c`: at the top of a loop frame, when `session.load_due`: one
  `voe_game_starting_frame` with the held splash (or NULL) and the line "Loading scene...",
  a false one ending the program as a closed window does; then `voe_editor_session_load`;
  rewind `scratch`. Keep it to a few lines.
- `editor/src/src.md`: the `session` entries say the load is put off a frame.

## Done when
`cmake --build --preset debug --target voe_editor` succeeds, and
`build/debug/editor/voe_editor --capture "$(mktemp -d)/shot.png"` exits 0.

Human: How to test steps 4, 5, 6 and 7 of `feature.md`.
