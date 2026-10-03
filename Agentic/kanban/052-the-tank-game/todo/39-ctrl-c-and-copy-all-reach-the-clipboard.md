# 39 — Ctrl+C and Copy all put the Errors panel's text on the clipboard
folder: editor/src
after: 36, 37, 38
decisions: 0168, 0342

## Change
0342 point 5. Read `editor/src/shortcuts.h`, `editor/src/shortcuts.c`,
`editor/src/frame_commands.h`, `editor/src/frame_commands.c`, `editor/src/errors.h`,
`editor/src/src.md`, the part of `editor/src/main.c` that fills `voe_editor_frame_commands`, and
`voe_platform_input_clipboard_set` in `platform/include/platform/input.h`.

- `shortcuts.h` and `shortcuts.c`: a new `copy` flag. It is Ctrl+C (`VOE_PLATFORM_KEY_C` pressed,
  Control down, Shift up), silenced while flying, while the browser shows and while typing. The
  header's list of which key edge means which command names it.
- `frame_commands.h`: the struct gains `voe_platform_window *window`, which is NULL on a capture.
  The header says Ctrl+C marks a copy of the Errors panel's selection, and that after the draw the
  wanted copy, Ctrl+C's or Copy all's, is handed to the clipboard.
- `frame_commands.c`:
  - The read: on `copy`, while the session's Errors panel shows and the browser does not, call
    `voe_editor_errors_copy_selection`.
  - `voe_editor_frame_commands_after_draw`: take the wanted copy with `voe_editor_errors_copy_take`
    into a local buffer of `VOE_EDITOR_ERRORS_TEXT_BYTES`. When it is more than 0 bytes and there is
    a window, call `voe_platform_input_clipboard_set`.
- `main.c`: fill `.window` with the loop's window, or NULL when capturing.
- `src.md`: the entries for `shortcuts.c` and `frame_commands.c` name Ctrl+C and the clipboard.

## Done when
`grep -q voe_platform_input_clipboard_set editor/src/frame_commands.c && grep -q VOE_PLATFORM_KEY_C editor/src/shortcuts.c`
exits 0, and the folder builds.

Human, on Linux:
1. Open `examples/tank_game` in the editor.
2. Make a build fail so the Errors panel shows. For example, put a syntax error in a file under
   `Code/` and press Refresh.
3. Press on one line and drag down over three lines. They show selected.
4. Press Ctrl+C and paste into a text editor. The three lines arrive exactly as shown, line breaks
   included.
5. Press Copy all and paste. Every line of the panel arrives.
6. Undo the syntax error.
