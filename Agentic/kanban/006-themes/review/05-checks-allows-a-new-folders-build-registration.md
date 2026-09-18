# 05 — `checks.sh --folder` allows a new folder's build registration
folder: ~/Projekt/agentic_rules/claude/skills/checks/scripts
decisions: 0175

## Change

This folder is in another repository, `~/Projekt/agentic_rules`; nothing in voe3d changes. Edit
`checks.sh` there, then run `./install.sh` from `~/Projekt/agentic_rules` so `~/.claude` gets it.

In the `folder)` branch of the mode `case`, a changed path outside the card's folder is today always a
`changed outside the card's folder` finding. Add a pattern beside `manifest_re`, named `build_registry_re`,
matching exactly the root `CMakeLists.txt` and `cmake/<name>.cmake` (`^(CMakeLists\.txt|cmake/[^/]+\.cmake)$`).
A path matching it is not a finding when the card in `doing/` has a `^decisions:` line, and still is one when
it has none. Nothing else changes: every other path outside the folder is still a finding either way. Update the
header comment's `--folder:` paragraph to say it.

Commit in `~/Projekt/agentic_rules` (`checks: allow root build registration under a decision (voe3d 0175)`).

## Done when

`bash -n checks.sh` exits 0, and in a throwaway repository made with `mktemp -d` holding a committed
`CMakeLists.txt`, `cmake/x.cmake`, `a/a.md` (`# a` and nothing else) and `Agentic/kanban/f/doing/01-x.md`:
after appending a line to both root build files, `~/.claude/skills/checks/scripts/checks.sh --folder a` prints no
`changed outside` finding when the card has a `decisions: 0001` line, and prints one per file when it does not.
Appending to any other root file (say `README.md`) is a finding in both cases.
