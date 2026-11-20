# 05 — The disk watch: what changed in the project, once a second
folder: editor
after: 01, 03, 04
decisions: 0168, 0386, 0490, 0504, 0505, 0506, 0509

## Change
Make new files `editor/src/disk_watch.h` and `editor/src/disk_watch.c`. Nothing uses them until
cards 07 to 14. First read the headers of `map_watch.h` (the same once-a-second stamp pattern) and
`assets_walk.h` (which files count as a project's sectioned files, and what is skipped).

- `voe_editor_disk_change` holds:
  - a project-relative path and the stamp seen;
  - the file's bytes and size, malloced, its own;
  - for a `.landscape`: the strict-read landscape, or NULL; when the read refused it, the worker's
    first error text.
- `voe_editor_disk_watch`: its memory is its own (rule 11's exception, as `strokes.h` explains).
  - `_create` and `_destroy`. Destroy finishes a look in flight first.
  - `_look(watch, folder, now)`. At most once a second, when no look is in flight and no changes
    are held, it starts one task on `voe_platform_burst_workers(false)`. The task:
    - stamps every file of the kinds assets_walk walks, plus `project.voe3d`;
    - reads the bytes of each file whose stamp differs from the known one;
    - strict-reads each changed `.landscape` there (`voe_assets_landscape_read_strict`), keeping
      its first error (`base/report.h`, 0490).

    A NULL folder means nothing is looked at. A folder unlike the last one forgets everything known.
    The first look in a folder notes every file and reports none.
  - `_changes(watch, &count)`: once the look's burst is done, returns the changes it found and now
    holds.
    - A new path is noted, not reported as a change.
    - A gone path is let go.
    - A path `_wrote` was told about since the look began is not a change.

    The changes are held until each one is settled, and no look starts meanwhile.
  - `_settle(watch, index, bool taken)`: the change has been dealt with. When `taken`, its bytes
    become the path's known copy (never for a landscape). Once every change is settled, all are
    freed.
  - `_wrote(watch, path)`: the editor wrote `path`, full or project-relative. A path outside the
    folder is ignored. The file is stamped now and that stamp becomes known; unless it is a
    `.landscape`, its bytes are re-read as the known copy. It is never a change.
  - `_known(watch, path, &size)`: the bytes last known for `path`, or NULL when there are none.
    This is a file step's before (0509 point 2).
- Header points:
  - what is watched;
  - why the once-a-second look runs as a burst (0386, 0504);
  - why a landscape is read on the worker;
  - why own writes are told rather than guessed (0509 point 1);
  - what is held, and when looks pause.
- `editor/src/src.md`: an entry for the two files.

## Meaning
- Scout: the editor cannot tell its own writes from outside ones.
- Scout: the editor sees no outside change made while a prefab is open or Play runs.

## Done when
- `grep -q voe_editor_disk_watch_wrote editor/src/disk_watch.h` and
  `grep -q voe_editor_disk_watch_settle editor/src/disk_watch.c` both exit 0.
- `d=$(mktemp -d) && XDG_CONFIG_HOME=$d build/debug/editor/voe_editor --capture $d/a.png && test -s $d/a.png`
  exits 0.
