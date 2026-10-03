# 0342 — The Errors panel selects whole lines and copies them to the desktop's clipboard
date: 2026-10-03
by: planner

## Decision
For 052 bug 05.

1. **Selection is by whole lines.** In the Errors panel (editor `errors.h`) each kept line is a
   choice row. A press on a line and a drag select the run of lines from that line to the one under
   the pointer, drawn inverted as a chosen row is (ADR-0194). A new showing clears the selection.
2. **Ctrl+C copies the selected lines; Copy all copies every line.** The lines are joined by `\n`,
   with none after the last, exactly as shown: a line cut at 160 bytes is copied cut, and a tab
   shown as a space is copied as a space. Copy all is a button beside Close.
3. **`platform` gains a write-only clipboard:** `voe_platform_input_clipboard_set(window, text,
   size)`. On Wayland it uses the core `wl_data_device_manager`, so no new protocol XML is needed.
   It offers `text/plain;charset=utf-8`, `text/plain` and `UTF8_STRING`. The window keeps its own
   copy of the text and writes it to whoever pastes until another program takes the clipboard. The
   selection is set with the serial of the latest key or button event, as compositors require. With
   no manager, no seat or no input yet, the call does nothing, as the pointer's shape does (0227).
   There is no reading of the clipboard (rule 10). It is Linux only (0339).
4. **C is a key** (`VOE_PLATFORM_KEY_C`), added after R, for Ctrl+C.
5. **The copy reaches the clipboard in `frame_commands`.** The interface has no window (ADR-0141
   point 4). So Ctrl+C and Copy all only mark a wanted copy on the panel. After the draw,
   `frame_commands` takes the text and hands it to `platform` through a window pointer that `main.c`
   fills in. A capture has no window and copies nothing.

## Reasoning
Selecting single characters would need a selectable-text widget in `ui`: glyph hit tests, a
selection that spans labels, and highlight records. Nothing else needs that, and this would be the
engine's first. A compiler error is a few whole lines, and whole lines are what a person passes on.
So line selection covers the bug, and it is built from what already exists: choice rows and node
rectangles read back after the frame. That is the same drag the Scene list already does.

Reading the clipboard (paste) has no caller, so it is not built. Not even the text field takes a
paste (0165).

The interface stays free of the window. Marking a wanted copy and acting on it after the draw keeps
the pattern `frame_commands` already uses for Delete and Ctrl+D.

## Replaces
Nothing. It narrows `platform/input.h`'s "No clipboard" to "no paste". 0242 point 8's Errors panel
gains selection and Copy all.
