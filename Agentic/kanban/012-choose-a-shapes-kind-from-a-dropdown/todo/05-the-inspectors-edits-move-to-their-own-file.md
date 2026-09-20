# 05 — The Inspector's edits move to their own file
folder: editor
decisions: 0168

## Change
The second half of card 04's split, again a move and nothing else: what `inspector.c` does after the frame has
ended leaves it, so that what it draws and what it reads are two files this feature can change one at a time.

`editor/src/inspector_edit.h` (new): the header of the file that turns what the pointer did to this frame's
controls into replace intents and into scene.h's calls. Its prose is the paragraphs already in `inspector.h`
about an edit being a replace intent and never a write, about a component with no replace intent being shown and
not edited, and about Duplicate, Delete, Remove and Add component being read after the frame — moved down here,
not copied. It includes `inspector.h` for the struct and declares, moved out of it word for word:
`voe_editor_inspector_edits_read`, `voe_editor_inspector_buttons_read` and
`voe_editor_inspector_colour_submit`.

`editor/src/inspector_edit.c` (new): moved from `inspector.c`, unchanged — `submit`, `apply`, `typed`,
`voe_editor_inspector_edits_read`, `action_of`, `counted`, `voe_editor_inspector_buttons_read` and
`voe_editor_inspector_colour_submit`, with the `the edit` section comment and the includes they need
(`inspector_value.h` among them).

`editor/src/inspector.c`: those functions gone. It keeps `voe_editor_inspector_frame_begin`,
`voe_editor_inspector_draw` and everything they call, so `dock.c` and its include are untouched.

`editor/src/inspector.h`: the three declarations and the paragraphs named above gone; the struct, the four
`#define`s and `frame_begin` and `draw` stay. `editor/src/interface.c`: `#include "inspector_edit.h"` beside its
`#include "inspector.h"`, nothing else.

`editor/src/src.md`: two entries for the new pair, and `inspector.c`'s and `inspector.h`'s entries no longer
claim the edit half.

## Done when
`checks.sh` for `editor` exits 0, `cmake --build --preset debug` builds the whole tree, and `wc -l
editor/src/inspector.c editor/src/inspector_edit.c` shows each under 700 lines.
