# 0509 — A file taken in from disk: own writes told, one step per kind, read strictly
date: 2026-11-06
by: planner

## Decision
For 115, carrying out 0506 points 1 and 2:

1. **One watch, told of the editor's own writes.** The editor stamps every sectioned file of the open
   project (the kinds `editor/src/assets_walk.h` walks, and `project.voe3d`) once a second, on one task
   of the standing workers (0504). Every editor write of such a file tells the watch its path, and the
   watch takes the new stamp as known: nothing the editor writes is a change. Under that, a change
   whose content says what the editor already holds is settled without a step. A new file is noted and
   a gone one let go; neither is taken in.
2. **One undo step per kind.**
   - The open scene or prefab: the scene text state, as for every edit.
   - A material, a closed prefab, `.parts` and `project.voe3d`, which the editor writes at once: a file
     step holding the bytes before (the watch's last known copy) and after. Stepping writes that side
     to disk and takes it in.
   - A landscape, which is saved with the project: a landscape step holding whole copies before and
     after. Stepping puts one into the store and leaves the landscape unsaved, as a stroke does.
   - Steps are malloced, as strokes are, and freed with their state.
3. **Strict readers beside the lenient ones.** A file being taken in is read by a strict reader
   (`_read_strict`, `voe_authoring_scene_check`). It refuses:
   - everything the lenient reader refuses;
   - a section or key the reader does not take, except a former name (0353), a retired map key, and
     a section of a component type this program never registered, which is kept;
   - in scene text, a ranged field (0498) outside its range.

   Opening a project keeps the lenient readers. `.parts` is read as it is, already strict; 0437
   point 3's `strength` stands. A refusal names the file, line, key and why in the notice. The editor
   keeps its last good state, and the stamp is settled so the refusal is said once.
4. **What waits.**
   - Nothing is taken in while Play runs, while a splash wait is shown, or while the question is open.
     The watch holds its changes and looks again once all are settled.
   - A change to the level's scene while a prefab is open waits for Back.
   - A changed `.parts` `[Levels]` applies at the next Make LODs or open (0470).

## Reasoning
The scout found that the editor cannot tell its own writes from outside ones, that readers vary in how
strict they are, and that unsaved is held per owner: the scene and an open prefab share one flag, the
set-aside level has its own, and each landscape keeps its store `edited` flag. Materials and the project
file are never unsaved. So:

- A step over a file the editor writes at once also writes, as material steps and the window size
  already do.
- A step over a file saved with the project changes only what the editor holds.

Rejected:
- Guessing own writes by time: an outside write in the same second would be lost, and then erased by
  the next save.
- Recognising own writes by content alone: a closed prefab and a landscape have nothing cheap to
  compare against.
- Making the existing readers strict: a project holding a key from another editor version would stop
  opening.
- A whole-grid stroke for a landscape: a stroke is a rectangle of heights or paint, not layers, kinds
  and copies.
- Bytes for a landscape's before: there is no cheap copy of its file, and the store already holds it.

## Replaces
Nothing.
