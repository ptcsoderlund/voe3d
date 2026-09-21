# 016 — Every folder meets the caps

## What
The whole tree passes `checks.sh --all` with no findings. Nothing the editor, `voe_dev` or any test
does changes: this touches comments and the folder `.md` pages only.

Every code file's header comment (the comment at its top) is 60 lines or fewer. It says what the file
does, how it is used and its constraints. Reasoning about one function moves down to sit above that
function, word for word where it still holds; nothing true is deleted to make room. Every
"- `name` — sentence" entry in a folder's `.md` is 300 characters or fewer, and what no longer fits
moves into that file's header.

The caps stay where they are (60 lines, 300 characters). From the feature after this one, every
feature is proved by `checks.sh --all` again (decision 0208).

## Why
The caps were tightened after much of the code was written, so about 40 older findings across 14
folders fail the whole-tree check. That check is what proves a feature is done, and it cannot do that
while it is red for reasons older than the feature.

## How to test
1. Run `bash ~/.claude/skills/checks/scripts/checks.sh --all`. It reports `FINDINGS: 0`.
2. Run `cmake -P check.cmake`. It passes.
3. Build and run the whole ctest suite. Every test passes.
4. Open the editor on a saved level. It starts, draws both views, and selecting, the move gizmo, and
   undo and redo work as they did before.
5. Open `ui/include/ui/layout.h` (today its header comment is the longest, at 292 lines). The top
   comment is short and tells you what the file is for; the reasoning that used to be there now
   sits above the functions it explains.
