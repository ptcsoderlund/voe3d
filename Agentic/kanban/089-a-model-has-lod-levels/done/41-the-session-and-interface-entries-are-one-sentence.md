# 41 — The session and interface index entries fit their cap
folder: editor/src
after: none
decisions: 0168

## Change
`editor/src/src.md` has two entries over the 300-character cap:

- `session.h` (307): cut to one sentence under 270 characters: the project
  being worked on, its commands, and the questions it holds. Details that may
  be dropped (Play, Refresh, Ship and open prefab by name; the Errors panel
  shown from the build log; the Delete and levels questions; the armed
  command that makes Close, New, Open and Back refuse once over unsaved work;
  the `saved` flag a written Save leaves for main.c) must be in
  `editor/src/session.h`'s header comment. That header is 59 lines, cap 60:
  add there only what it lacks, and if that would pass 60, move the why of
  one function from the header to above that function's declaration in the
  same file.
- `interface.h` (330): cut to one sentence under 270 characters: the
  screen-filling surface in the theme it is handed, and its one draw command
  per root. Details that may be dropped (pixels per millimetre from the
  window's height; the top bar above each root's dock tree; the browser,
  Preferences, Project, Errors and frame breakdown; the Delete and levels
  questions or the colour picker over it) must be in
  `editor/src/interface.h`'s header comment (18 lines); add any it lacks.

Comments and index only; no code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --structure` prints no
FINDING line naming `editor/src/src.md`, and
`bash ~/.claude/skills/checks/scripts/checks.sh --header-lines editor/src/session.h`
prints 60 or less and no "over cap".
