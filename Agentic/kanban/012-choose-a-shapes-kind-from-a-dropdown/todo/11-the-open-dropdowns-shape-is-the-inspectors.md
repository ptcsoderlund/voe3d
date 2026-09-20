# 11 — The open dropdown's shape is the Inspector's
folder: editor
decisions: 0168, 0195, 0198, 0199

## Change
Three files, and nothing runs differently after this card: it is the move that lets the Inspector's own panel
name the open list, which is what cards 12 to 14 make it draw.

`editor/src/scene.h`: `#define VOE_EDITOR_DROPDOWN_ROWS 16` and `typedef struct { ... } voe_editor_dropdown;`
leave this file, fields and comments as they stand. What stays is the `voe_editor_dropdown dropdown;` field on
`voe_editor_scene` and the three calls that open, close and ask about it — this file already includes
`inspector.h`, so both still resolve. `<base/describe.h>` goes with them: after the move nothing else in this
file names a described type, and `inspector.h` includes it.

Its prose paragraph THE OPEN DROPDOWN'S TARGET IS HERE TOO keeps every sentence about policy — who opens it, who
closes it, that closing chooses nothing, and that only one of it and the colour picker is ever open — and gains
that its shape is `inspector.h`'s, because the panel that draws the list and reads its rows is the Inspector's;
what is here is the one that is open, kept across frames.

`editor/src/inspector.h`: the constant and the struct arrive verbatim, placed after
`VOE_EDITOR_INSPECTOR_SECTIONS` and before `voe_editor_inspector_control`. The paragraph A NAMED FIELD IS A
DROPDOWN gains one sentence: what a fired button opens the list on is the `voe_editor_dropdown` below, whose
shape is this file's because this panel is what draws the list and reads what was picked from it, and scene.h
holds the one that is open and says who may open and close it.

`editor/src/src.md`: the `scene.h` and `inspector.h` entries say where the open dropdown's shape now lives.

## Done when
`checks.sh --folder editor` exits 0 and `cmake --build --preset debug` builds the whole tree. The editor behaves
exactly as before — the list still draws from `interface.c` and still stays where it was opened, which is
bug 01 and is card 12's and card 14's to take away.
