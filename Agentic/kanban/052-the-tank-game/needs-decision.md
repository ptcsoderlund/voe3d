# Needs decision — bug 04: a card in the root folder cannot pass its folder check

## Question
Decision 0340 has `check.cmake` split and then given a whole-tree Release step. `check.cmake` sits at
the repository root, so every card that edits it has `folder: .`. No such card can pass
`checks.sh --folder .` today. How is the root folder's per-folder check made to pass?

## What the planner found
- The per-folder line in `CLAUDE.md` `## Checks` takes `top=.`, finds `./CMakeLists.txt`, and builds
  the target `voe_.`. That target does not exist: `ninja -C build/debug -t query voe_.` prints
  "unknown target 'voe_.'" and exits 1. `checks.sh --folder .` reports that as a finding, so the coder
  of any root card blocks.
- No `cmake/` or other subfolder route avoids it. `check.cmake` is the entry `CLAUDE.md` and
  `ONBOARDING.md` name, so both the split and the Release step change a root file.
- `checks.sh --folder .` counts only files directly in the root as the root's own. A split that puts
  the parts in a new `check/` folder is therefore two cards: `check/` writes the parts, then `.` turns
  `check.cmake` into the file that includes them.
- The `base` fix (`VOE_BASE_DEBUG_ASSERT` keeps its expression compiled but never runs it) is not
  affected by this. It is ready to plan as one card in `base`.

## Options
1. **The human changes the per-folder line in `CLAUDE.md` so that the root folder exits 0.** For
   example, the guard becomes `[ "$top" != . ] && [ -f "$top/CMakeLists.txt" ] || exit 0`. The root's
   files are already covered by the whole-suite `cmake -P check.cmake`. After that the planner writes:
   the `base` fix, the parts in `check/`, `check.cmake` including them, and the Release step.
2. **The first root card of bug 04 makes that same `CLAUDE.md` edit itself.** A decision names it, so
   the card may change the human's instructions file. The cards and the result are the same as in
   option 1.
3. **Plan only the `base` fix now and leave the `check.cmake` work of 0340 until root cards can pass.**
   Ship builds again, but the Release check 0340 asks for waits.

## Recommendation
Option 1. It changes one line of a file that only the human edits, and every later root card can then
pass its folder check, not only the cards for this bug.
