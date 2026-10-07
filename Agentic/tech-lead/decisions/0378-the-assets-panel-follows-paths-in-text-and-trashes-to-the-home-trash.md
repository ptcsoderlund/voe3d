# 0378 — The Assets panel follows paths in text and trashes to the home trash
date: 2026-10-07
by: planner

## Decision
For 081, carrying out 0377 point 3:
1. **What follows.** A rename or move of `Assets/<from>` to `Assets/<to>` rewrites, in every `.scene` and
   `.prefab` file under the project folder (hidden folders, `Build/`, `Cache/` and `Code/` skipped), every
   quoted string value that is exactly the old project-relative path or begins with it and `/`. It is a
   text rewrite in `authoring`, so kept sections and project components follow too; a bare name such as a
   spawner's prefab name is C's and does not. A later own kind adds its extension to the editor's one list.
   The same match answers "what uses it" for Delete.
2. **The open scene follows in memory**: its text is written, followed and read back into the same world;
   its unsaved flag is kept; the undo line is forgotten, because its states name the old paths.
3. **Refused, with a notice, nothing changed:** a name empty, beginning with `.`, or holding `/`, `\` or
   `"`; a name taken in the target folder (a move never replaces); a followed path of 128 bytes or more;
   a folder moved into itself; any rename, move, duplicate or delete while a prefab is open.
4. **Create → Folder and Rename name in place**: a field in the row, focused with the name selected; the
   folder is made, or the file renamed, only when Enter or a press elsewhere commits; Escape makes nothing.
5. **Duplicate** copies a file beside itself as `<stem> 2<ext>`, the next free number up to 99; a folder's
   Duplicate is refused with a notice.
6. **The selected row** is the last row pressed by either button; a click still enters a folder and opens
   a prefab as before. F2 and the Delete key act on it while the Assets panel holds the keyboard: a press
   in the panel takes it, a press anywhere else gives it back to the scene.
7. **Trash** is the freedesktop home trash, `$XDG_DATA_HOME/Trash` else `~/.local/share/Trash`, with its
   `files/` and `info/<name>.trashinfo`, a taken name numbered. A file on another file system than the
   trash is refused, nothing deleted; the per-drive `.Trash-<uid>` waits until someone needs it.
8. **The right button** is read by the editor against the Assets panel's last-frame rectangles, since
   `ui` knows one button; a move is a rename that refuses a taken target (`renameat2` without replace).

## Reasoning
A text rewrite needs no world per file and no knowledge of which component holds a path, so a project's
own components and the kinds 082–092 bring follow without a change here. Making the folder only on
commit is what lets a refused or cancelled name change nothing (feature 081, How to test 3). Refusing
while a prefab is open avoids following a level set aside as text. Forgetting undo is simpler and safer
than following 64 KiB states in place. Writing the trash ourselves keeps rule 5; `gio trash` would tie
the editor to GNOME.

## Replaces
nothing. Carries out 0377 point 3.
