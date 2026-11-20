# Outside changes

What the editor does when a file of the open project changes on disk: how it notices, how it takes the change in, and what it never takes for one.

## Taking in
- A file with no unsaved changes is taken in silently as one undo step: the tech-lead's choice; the sponsor spoke only of the unsaved case. [assumed · tech-lead · 2026-11-06] see 0506
- A change to `project.voe3d`'s `scene` key does not switch the scene that is open. [observed · accept 115 · 2026-11-20]
- A changed `[Levels]` in a `.parts` file takes effect at the next Make LODs or project open. [observed · accept 115 · 2026-11-20] see 0470
- A `.parts` rewritten with the same content is still taken in and still makes an undo step; only materials and `project.voe3d` are compared first and skipped when unchanged. [observed · accept 115 · 2026-11-20]
- Taking in a `.material` re-reads the material tables but not the Inspector's open copy; the editor refreshes that copy after each take so the Inspector shows the new values. [observed · accept 115 · 2026-11-20]
- Taking in the scene or a closed prefab rebuilds the world; the selection survives it, found again by authored id. [observed · accept 115 · 2026-11-20]
- Changes wait while Play is in any stage but idle, configuring and building included, not only while the game runs. [observed · accept 115 · 2026-11-20] see 0509

## Landscapes
- A landscape taken in from disk shows at the next models update, not inside the undo step's take: loading it needs the device. [observed · planner 115 · 2026-11-06]
- A whole landscape is copied only by resampling it at its own cell count, which is an exact copy. [observed · planner 115 · 2026-11-06]
- A landscape that failed to load and is then fixed on disk is taken in over its failed entry. [observed · accept 115 · 2026-11-20]
- At most 8 landscapes wait to load at once; a ninth changed in the same update is left out, said only on stderr. [observed · accept 115 · 2026-11-20]

## Undo across files
- Undo or redo to a side of a file step that has no bytes leaves the file as it is and says so; an empty side is never written. [observed · accept 115 · 2026-11-20]
- An undo that crosses a file step refused on reading still moves the scene to the state stepped to and counts as done; the refusal is shown, and the redo line is kept. [observed · accept 115 · 2026-11-20]

## Refused
- A scene take refused by the strict read names the open prefab's path, else the project's scene file. [observed · accept 115 · 2026-11-20]

## The question
- Only the load-from-disk-or-keep-mine question's buttons answer it: not Escape, not a press outside it, unlike the Delete and textures questions, since keep mine lets the next Save overwrite the outside change. [observed · accept 115 · 2026-11-20]
- The file browser hides the question while it shows but never closes it; the question comes back when the browser hides. [observed · accept 115 · 2026-11-20]
- Files waiting to ask are asked one at a time, the next a frame after the last answer is taken. [observed · accept 115 · 2026-11-20]

## Noticing
- What the watch counts as the project's files is a copy of the assets walk's extension and skipped-folder lists; a new file kind must be added to both. [observed · accept 115 · 2026-11-20]
- A new `.landscape` is noted without being read; a changed file that cannot be read is not yet a change and is looked at again a second later. [observed · accept 115 · 2026-11-20]
- The watch lives as long as the open project: made when startup gives the editor one, ended with it on both ways out. [observed · coder 115-07 · 2026-11-06]
- Moving to no project folder forgets everything the watch knew. [observed · accept 115 · 2026-11-20]

## Once seen
- A settled change is known whether it was taken or not, so after "keep mine" or a refusal the same change is not found again each second; only taking it moves the bytes the editor compares against. [observed · accept 115 · 2026-11-20]

## The editor's own writes
- A held outside change to a file the editor then writes settles untaken, without touching the known stamp or bytes: disk now holds the editor's version, and the editor's own write is never taken as an outside one. [observed · accept 115 · 2026-11-20]
- Reading a prefab's set-aside level back writes no file; only saving the project writes the prefab file, and that write tells the watch. [observed · coder 115-07 · 2026-11-06]
