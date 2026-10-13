# 47 — The editor source's long entries are one sentence each
folder: editor/src
after: none
decisions: 0168

## Change
In `editor/src/src.md` four entries are over the 300-character cap:
`session.c` (326), `assets_panel.c` (356), `assets_manage.h` (396) and
`scene.h` (305). Shorten each to one sentence under 300 characters that says
what the file is for; the itemised detail belongs in the file's own header
comment.

For each, open the header comment of the file (`editor/src/session.c`,
`editor/src/assets_panel.c`, `editor/src/assets_manage.h`,
`editor/src/scene.h`) and check that what the entry drops is said there:

- `session.c`: the import with materials read again and the load a frame
  later.
- `assets_panel.c`: a `.glb`'s materials and parts written after its copy.
- `assets_manage.h`: a `.glb`'s `.parts` taken along on a move, and the
  materials and parts read again after with the open material refreshed.
- `scene.h`: the material open in the Inspector, its last-written copy,
  refreshed when the materials are read again.

Add only what is missing. `scene.h`'s header is at the 60-line cap and
`assets_manage.h`'s at 57: any addition there is paid for by tightening the
same header. Measure with
`bash ~/.claude/skills/checks/scripts/checks.sh --header-lines editor/src/scene.h editor/src/assets_manage.h editor/src/session.c editor/src/assets_panel.c`.
No code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder editor/src` prints no FINDING naming `editor/src/src.md`, and
the `--header-lines` command above prints no "over cap".
