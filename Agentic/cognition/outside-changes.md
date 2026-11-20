# Outside changes

What the editor does when a file of the open project changes on disk: how it notices, how it takes the change in, and what it never takes for one.

## Taking in
- A file with no unsaved changes is taken in silently as one undo step: the tech-lead's choice; the sponsor spoke only of the unsaved case. [assumed · tech-lead · 2026-11-06] see 0506
- Escape does not answer the load-from-disk-or-keep-mine question. [assumed · planner 115 · 2026-11-06]
- A change to `project.voe3d`'s `scene` key does not switch the scene that is open. [assumed · planner 115 · 2026-11-06]
- A changed `[Levels]` in a `.parts` file takes effect at the next Make LODs or project open. [assumed · planner 115 · 2026-11-06] see 0470
- A landscape taken in from disk shows at the next models update, not inside the undo step's take: loading it needs the device. [observed · planner 115 · 2026-11-06]
- A whole landscape is copied only by resampling it at its own cell count, which is an exact copy. [observed · planner 115 · 2026-11-06]

## Noticing
- What the watch counts as the project's files is a copy of the assets walk's extension and skipped-folder lists; a new file kind must be added to both. [assumed · coder 115-05 · 2026-11-06]
- A new `.landscape` is noted without being read; a changed file that cannot be read is not yet a change and is looked at again a second later. [assumed · coder 115-05 · 2026-11-06]
- The watch lives as long as the open project: made when startup gives the editor one, ended with it on both ways out. [observed · coder 115-07 · 2026-11-06]
- Moving to no project folder forgets everything the watch knew. [assumed · coder 115-05 · 2026-11-06]

## Once seen
- A settled change is known whether it was taken or not, so after "keep mine" or a refusal the same change is not found again each second; only taking it moves the bytes the editor compares against. [assumed · coder 115-05 · 2026-11-06]

## The editor's own writes
- A held outside change to a file the editor then writes is marked written and settles without touching the known stamp or bytes, so the editor's own write is never taken as an outside one. [assumed · coder 115-05 · 2026-11-06]
- Reading a prefab's set-aside level back writes no file; only saving the project writes the prefab file, and that write tells the watch. [observed · coder 115-07 · 2026-11-06]
