# 02 — Open and Save start beside the current project, or the last one
folder: editor/src
after: 01
decisions: 0168, 0343
read: feature.md

## Change
0343 points 2, 4 and 5, the session's half. Read `editor/src/session.h`, `editor/src/session.c`,
`editor/src/last_project.h`, `editor/src/browser.h` (card 01's `beside` and `target`) and
`editor/src/src.md`.

- `session.c`, `voe_editor_session_do`: OPEN and SAVE (the untitled case) pass `beside` to
  `voe_editor_browser_show`: `session->project->folder` when it is set; else
  `voe_editor_last_project_read` into a scratch arena made and destroyed around the call (the
  `SESSION_SCRATCH` pattern `session_has_code` uses), NULL when that reads nothing. The browser
  copies `beside`, so the scratch may go at once.
- `session.c`, `voe_editor_session_browser_do`, CONFIRM in OPEN mode: open `browser->target`
  instead of `browser->folder`. SAVE mode is unchanged: it saves into `browser->folder`.
- `session.h`: OPEN's paragraph says the browser starts beside the project's folder, or for an
  untitled scene the last project, with its row chosen (0343); SAVE's says the same for its
  browser and that Save here still means the shown folder; NEW's says it still makes an untitled
  scene and its first Save is where it browses. `voe_editor_session_browser_do`'s comment: OPEN's
  Confirm acts on `target`.
- `src.md`: `session.h`'s entry names where Open and Save start.

## Done when
`grep -c "browser->target" editor/src/session.c` prints 1 or more,
`grep -c "voe_editor_last_project_read" editor/src/session.c` prints 1 or more, and the folder
builds.

Human: `feature.md` `## How to test`, steps 1 to 6. For step 4 press New, then Save: the first
Save of the new untitled scene is the browser that asks where to put it (0343 point 5); it starts
in `examples` with `coin_game` chosen.
