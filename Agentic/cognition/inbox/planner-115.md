# planner 115

## Assumed
- When the editor holds no unsaved change to a file that changed on disk, it takes the file in
  quietly, without a question. 0506 settles this.
- Out-of-range values: only scene fields with a 0498 range are refused when out of range.
  Material and landscape values are refused only where their readers already refuse them;
  roughness and metal get no range of their own.
- `strength` in a `.parts` `[Levels]` section is not wrong (0437 point 3). The parts reader is used
  as it is.
- A changed `[Levels]` in `.parts` takes effect at the next Make LODs or project open; a watch never
  makes cooked copies (0470).
- Escape does not answer the load-from-disk-or-keep-mine question.
- A change to `project.voe3d`'s `scene` key does not switch the scene that is open.
- Re-importing a model tells the watch which `.parts` and `.material` it rewrote, so a re-import is
  never taken as an outside change.

## Learned
- Decision 0508 already exists, under `implemented/`, and is not listed in `decisions.md`.
- `editor/src/assets_panel.c` is 805 lines. Its only file write is the import's copy into the shown
  folder.
- `voe_assets_landscape_resample` at a landscape's own cell count is an exact copy, and is the only
  way to copy a whole landscape.
- `voe_3d_models_load_landscape` needs the device, so a landscape taken in from disk loads at the
  next models update, not inside an undo take.
- The editor has no tests folder. Editor cards are proved by grep and the `--capture` run.
