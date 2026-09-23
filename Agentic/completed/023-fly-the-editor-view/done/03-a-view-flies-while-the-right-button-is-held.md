# 03 — A view flies while the right button is held
folder: editor
decisions: 0168, 0222, 0233

## Change
The views gain the fly of 0233 as one call, beside the middle-button drag. Nothing calls it yet (card 04).

- `editor/src/view.h`
  - `voe_editor_fly_keys`: seven bools, `forward`, `back`, `left`, `right`, `up`, `down`, `fast`,
    each the level of the key the caller maps to it.
  - `voe_editor_views` gains `uint32_t flying` (the flying view, `VOE_EDITOR_VIEW_NONE` when none)
    and `bool right_was_down`.
  - `bool voe_editor_views_fly(voe_editor_views *views, voe_math_float2 pointer, bool right,
    voe_math_float2 turn, voe_editor_fly_keys keys, float seconds)`: the right button's down edge
    over a view (`voe_editor_views_under`) while no middle drag holds one makes it the flying view
    until `right` goes up; while flying, `turn` (motion units, +x right, +y down) turns and `keys`
    move that view by `seconds`. Returns whether a view is flying after this call.
  - Header: a paragraph saying a view is moved two ways, the middle drag and the right-button fly,
    one view at a time, neither written anywhere (not the world, not undo, not settings); the
    comment on `eye` says the fly moves it with the focus and keeps the orbit's sum true.
- `editor/src/view.c`
  - `voe_editor_views_create`: `flying` starts as `VOE_EDITOR_VIEW_NONE`.
  - `voe_editor_views_drag`: a middle press does not capture while a view flies.
  - `voe_editor_views_fly`, with its rates as named `#define`s beside the orbit's (0233: 0.004 rad
    per unit, 4 m/s, ×3). Turning: yaw falls as `turn.x` grows (positive yaw turns to −X), pitch falls
    as `turn.y` grows, clamped to `PITCH_LIMIT`; the eye stays and the focus becomes eye plus the
    existing forward helper times `distance`. Moving: forward is that helper's direction (with pitch),
    right is level from yaw, up is world +Y; the held ones summed, normalised when not zero, times
    speed and `seconds`, added to eye and focus. Then the eye is recomputed the way the orbit does.
  - File header: "the orbit owns the eye" still holds; the fly turns about the eye by moving the focus.
- `editor/src/src.md` — the `view.h` and `view.c` entries name the right-button fly.

Read `view.h` and `view.c`, and `src.md`; no other file.

## Done when
The Checks line of `CLAUDE.md` with `{folder}` = `editor` exits 0. With `XDG_CONFIG_HOME` at an empty
scratch folder, `voe_editor <scratch>/p --capture <scratch>/a.png --size 1280x720` exits 0 and draws
the two views as before (0177).
