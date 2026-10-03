# 05 — The Errors window's text cannot be selected or copied

## Seen
"The editor gives errors in its own error window. I cant select and copy text there."

When Ship failed (bug 04), the only way to pass on the error was to find `Build/build.log` by hand, because
the text in the Errors window can't be selected.

## Expected
Text in the Errors window can be selected with the mouse and copied with Ctrl+C, to paste anywhere else on
the desktop. Copying an error gives its text exactly as shown, line breaks included. There is also a way to
copy all of the window's text at once.

## How to reproduce
1. Open `examples/tank_game` in the editor on Linux.
2. Cause an error that shows in the Errors window, for example Ship while bug 04 stands.
3. Try to drag over the error's text and press Ctrl+C: nothing is selected and nothing is copied.
