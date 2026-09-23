# 06 — Every draggable border rests in the border colour and lights when reached
folder: editor
decisions: 0168, 0194, 0196, 0226, 0229, 0231, 0232

## Change
Bug 02: every dock seam is filled with `palette->border` at rest and drawn as card 05's two stripes
while hovered or dragged. The seam keeps its size and place; `voe_editor_dock_arrange`, the band and
`VOE_EDITOR_RESIZE_REACH` are untouched. The top bar's lower edge stays as it is.

- `editor/src/resize.h`, `editor/src/resize.c`
  - `voe_editor_resize_result` gains `uint32_t reached`: the split being dragged, else the split whose
    band the pointer is in with a drag allowed and the button up (as the cursor rule),
    else UINT32_MAX (also for the bar's edge).
  - `voe_editor_resize_frame` fills it on every path.
  - Header: the "own hit test" paragraph no longer says `ui` draws nothing in a seam; it says a seam is
    drawn by the walk as a fill, not a widget, so no widget answers for it, and that `reached` is what
    the walk lights (0231, 0232).
- `editor/src/dock.h`
  - `voe_editor_dock_root` gains `uint32_t lit`: the split whose seam is drawn lit this frame, UINT32_MAX
    or any non-split index for none; the caller's to fill, like `pointer`.
  - The header's paragraph ending "drawn as a dark and a light stripe ... (0230)" says instead: every
    seam is drawn in the border colour, and the lit one as two stripes (0231, 0232).
  - `voe_editor_dock_walk`'s `palette` paragraph: read for every seam's colours, not only the views'.
- `editor/src/dock.c`
  - `views_seam` becomes the seam of any split, named for it (e.g. `seam_draw`), taking a `bool lit`:
    lit, the present two stripes; not lit, one `voe_ui_swatch` of `palette->border.xyz`, `SEAM` along
    and filling across, in the same box.
  - `walk_node` takes the lit index (from `root->lit`, passed down by `voe_editor_dock_walk`) and draws
    that seam for every split where it now draws the views' seam or leaves a gap; each child still gets
    exactly the arrangement's size. `views_split` stays for the share calls only.
  - File header: the "THE SEAM IS A GAP AND NOT A DRAWN DIVIDER" paragraph becomes: a seam is a millimetre
    neither child fills, drawn in the border colour so it reads like every other border; the reached
    one as the inverse pair, which shows over any picture in any theme (0194, 0196, 0231, 0232); the
    arrangement's `seam` is still what resize.h hit-tests.
- `editor/src/main.c` — right after `voe_editor_resize_frame`, every frame, `roots[0].lit =
  resized.reached;`. The comment above that call no longer says a seam is a gap `ui` draws nothing in.
- `editor/src/src.md` — the `dock.h` entry: seams drawn in the border colour, the reached one lit (not
  "the views' in two stripes"); the `resize.h` entry names the border it reports reached.

`interface.c` needs no change: it copies the root.

Read `resize.h`, the heads of `voe_editor_resize_frame` and `border_under` in `resize.c`, `dock.h`, `dock.c`'s
file header and the heads of `views_seam`, `walk_node` and `voe_editor_dock_walk`, and `main.c` around the
`voe_editor_resize_frame` call; no other file.

## Done when
The coder: `bash ~/.claude/skills/checks/scripts/checks.sh --all` exits 0, and with `XDG_CONFIG_HOME` at an
empty scratch folder, `voe_editor <scratch>/p --capture <scratch>/a.png --size 1280x720` writes its picture
with every seam a thin line in the border colour and none striped.

The human, at a running `voe_editor`, in Near black, Near white and a monochrome theme, with a view
showing light and then dark content: the border between the views and both side-panel borders look like
the editor's other borders at rest; each turns to a dark and a light stripe while the pointer is over it
and for the whole of a drag, even when the pointer leaves it mid-drag; each goes back when the pointer has
left and the drag has ended; the pointer still changes shape as close to each border as before and
`feature.md`'s steps still hold.
