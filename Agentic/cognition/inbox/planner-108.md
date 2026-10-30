## Learned
- `ui/src/widgets.c` is 881 lines and holds the label (measure and draw), so a heading text role needs a card that first splits the label out into its own file.
- Nothing in `.landscape` or the Foliage section stops two kinds of one landscape naming the same `.glb`, so a model path does not name a kind uniquely.
- The editor can add a foliage kind (an Assets drop, no undo step) but has no way to remove one, so 0502's question of folds shifting when a kind is removed does not arise in 108.
- 0496 point 3 and `editor/src/settings_layout.h` both cite 0491 for the dock layout, but no file 0491 exists in `Agentic/tech-lead/decisions/`.
- Oxanium covers ‹ and › but has no down-pointing arrow or triangle (U+2193, U+25B8, U+25BE are absent), so a fold arrow cannot come from the glyph sheet as it stands.
- `editor` is a program with no `tests/` folder (`voe_executable` builds none), so its cards are proved by the build and a grep, and by the human's walk.

## Assumed
- The Inspector's Enabled row, Duplicate and Delete row, a part's line, Add component and the open material's section are not sections and do not fold.
- A kind's drop target stays its title row, not the whole box.
