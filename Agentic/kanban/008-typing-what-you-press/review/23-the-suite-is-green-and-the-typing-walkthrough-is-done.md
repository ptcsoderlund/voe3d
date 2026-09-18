# 23 — the suite is green and the typing walkthrough is done
folder: .
decisions: 0168
read: feature.md

## Change

Nothing in the repository changes unless a finding demands it. This card is the feature's acceptance pass, and
it is last because cards 01 and 02 (in `review/`) did the typing and cards 04–22 cleared the suite.

1. **The tool the suite needs is the programmer's.** `slangc` is installed at
   `~/voe3d-scratch/tools/slang/bin/slangc` (2026.17) and is not on PATH in a non-interactive shell, which is
   the only reason `cmake -P check.cmake` stops at step 1 with *slangc was not found*. Put that directory on
   PATH for the session and run again. Do not teach `check.cmake`, `cmake/voe.cmake` or the build to go looking
   for it: a source-transforming tool is installed by the programmer (ADR-0021), and a build that searches for
   one is the thing that decision exists to stop. If it is still not found, say so and stop — that is the
   sponsor's to fix, not a card's.
2. **Run `bash ~/.claude/skills/checks/scripts/checks.sh --all`.** Every finding it prints should already be
   owned by a card in this feature. One that is not is reported, not fixed here.
3. **Walk `feature.md`'s `## How to test`, steps 1 to 7**, in `./build/debug/editor/voe_editor` on Linux, on
   whatever layout the machine is set to: Save on the untitled scene, then the name box beside *Make folder*.
   Report which steps were seen and which were not. Step 8 is what point 2 covers.

## Done when

`bash ~/.claude/skills/checks/scripts/checks.sh --all` prints `FINDINGS: 0`, and the report names each of
`feature.md`'s steps 1 to 7 with what was seen when it was tried.
