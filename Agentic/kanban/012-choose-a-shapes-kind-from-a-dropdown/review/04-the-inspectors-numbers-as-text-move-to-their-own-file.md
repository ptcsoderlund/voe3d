# 04 — The Inspector's numbers as text move to their own file
folder: editor
decisions: 0168

## Change
A move and nothing else: `editor/src/inspector.c` is 1184 lines and this feature changes both of its halves, so
the part that turns a field's bytes into text and numbers goes first.

`editor/src/inspector_value.h` (new): the header of a file that owns what a field's bytes say — nothing in it
draws, nothing in it writes, and every function in it is about one field's kind and one field's bytes. Say that
it is `inspector.c`'s and `inspector_edit.c`'s shared arithmetic, that the strings it formats go in the frame's
arena for the reason `inspector.h` already gives, and that the three shown angles are shown and never stored.
Declare the functions moved below, in the order they move.

`editor/src/inspector_value.c` (new): moved from `inspector.c`, unchanged but for losing `static` on what the
header now declares — `text`, `chars`, `heading`, `real32_at`, `whole_signed`, `whole_unsigned`, `is_signed`,
`is_unsigned`, `dragged`, `shown_angles`, `angle_of`, `world_axis`, `axis_name`, `value_text`, `lanes` and
`is_vector`, with the section comments they sit under (`the text`, `reading`, `the angles`, `what it says`) and
the includes those functions need.

`editor/src/inspector.c`: those functions gone, `#include "inspector_value.h"` added, everything else exactly as
it was.

`editor/src/src.md`: two entries for the new pair, in the list's order beside `inspector.c`, each in the voice
the other entries use.

## Done when
`checks.sh` for `editor` exits 0, `cmake --build --preset debug` builds the whole tree, and `wc -l
editor/src/inspector.c editor/src/inspector_value.c` shows each under 900 lines.
