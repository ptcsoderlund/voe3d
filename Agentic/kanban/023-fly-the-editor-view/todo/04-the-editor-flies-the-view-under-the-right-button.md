# 04 — The editor flies the view under the right button
folder: editor
decisions: 0168, 0192, 0233
read: feature.md

## Change
The loop reads the right button and the mouse's motion, flies the view (card 03), locks the pointer
while it does, and lets no key it reads reach a field or a shortcut.

- `editor/src/shortcuts.h`, `editor/src/shortcuts.c` — `voe_editor_shortcuts_guards` gains
  `bool flying`. While it is true every command flag is false and `at_rest` is false; `escape` and
  `escape_free` are unchanged. The guards' comment names where the caller reads it.
- `editor/src/main.c`
  - In the window reads: the right button and `voe_platform_input_motion`, zero with no window.
  - Build `roots[0].pointer` before the shortcuts read (it needs only the pointer, the wheel, the left
    button, Shift and the millimetres), then call `voe_editor_views_fly` with its `.at`, the right
    button and `!browser.showing`, the motion, W/S/A/D/E/Q/Shift from `keyboard.down`, and
    `(float)opened.tick.step`.
  - While flying: `flying` into the shortcuts' guards; `roots[0].keyboard` gets no text and no
    Backspace, Enter or Tab; `roots[0].pointer` has `over` and `down` false; `left` is false for the
    gizmo and pick; resize is not allowed to start (its enabling argument false).
  - When flying changes from last frame, `voe_platform_input_lock_pointer(window, flying)` (window
    not NULL). Keep the previous value in a local beside the loop's others.
  - The header's paragraph on the keyboard read says a flying view keeps every key it reads from the
    interface and the shortcuts; the input paragraph names the right button as the views'.
- `editor/src/src.md` — `shortcuts.h` entry names the flying guard; `main.c` entry: flies a view.

Read the headers of `main.c`, `shortcuts.h`, `shortcuts.c`, `view.h`, and `main.c`'s loop.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --all` exits 0. The human then runs `## How to test`
of `feature.md` steps 1–10 in `voe_editor` on Linux: each step as written, including the pointer
hidden while flying and shown where it was on release.
