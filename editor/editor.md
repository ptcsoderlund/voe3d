# editor

The program a person opens to author a scene. Today it opens a window and draws
two named regions side by side — `Scene` on the left, a quarter of the width, and
`Inspector` beside it. The left one lists the authored entities of a scene built
in code and a click on one selects it; the right one says which. Nothing is
draggable and there is no viewport.

It is a leaf and it stays one, exactly as `dev` is: it names whatever it needs
and nothing names it (ADR-0121). No engine folder gains anything for the
editor's sake — a gap in one of them is a card in that folder, never a
reach-around from here.

## Why the layout is a tree of data and not the order of the calls

`src/dock.h` holds a tree: a node either divides the space it was given in two,
by an axis and a fraction, or names the one panel that fills it. The walk turns
that tree into `ui` rows, columns and panels. An interface laid out by call order
would look identical on screen and cost less to write — the difference is what it
takes to *change* it. With call order, a splitter has to rewrite the function
that draws the frame, which is the same function every panel's contents are in;
with a tree, a splitter writes one `fraction`. The tree is here first so the
thing that comes next is an edit to one number rather than a rewrite of
everything around it. ADR-0142.

Nothing writes a fraction today. `voe_editor_dock_default()` builds the tree and
no code path changes it afterwards, which is the whole of what a drag would have
to touch.

## Building and running it

    cmake --preset editor
    cmake --build --preset editor --target voe_editor
    ./build/editor/editor/voe_editor

The `editor` preset is `debug` plus `-DVOE_BASE_DESCRIPTIONS=1`, which is what
puts `base`'s field descriptions in the binary — the inspector expands a
component by walking them, so with them off it will have nothing to show. `debug`
and `release` keep them off, and the program says at startup which kind of build
it is. It builds and runs under the `debug` preset too; it just cannot expand
anything.

## The files

- `src/main.c` — opens the window and the device through `voe_app_new`, makes the
  arena, the world, the font, the interface context and the default tree, and
  runs the loop until the window closes. The `while` is this file's and the parts
  in it are `app`'s (ADR-0135). **The one division that turns the mouse's pixels
  into the surface's millimetres is here and nowhere else** (ADR-0141 point 4):
  the interface is handed a pointer already in millimetres, because the day a
  panel is a quad standing in the world that conversion is a ray against the quad
  and only a call site can know which of the two it wants. The view and the light
  handed to `voe_app_draw_open` are zeroed — the editor draws no world yet. It
  registers the two components, builds the scene, and runs both owning systems
  every frame whether anything submitted or not.
- `src/dock.h`, `src/dock.c` — the tree, the walk, and `voe_editor_panel_draw`,
  which is where the two panels' contents are.
  A panel is a value in an enumeration: not a string, not registered anywhere,
  and there is no table to add a row to. **No function pointer lives in this
  folder** — the panel draw is one function with a `switch`, the way `app` keeps
  callbacks out of the frame loop. A split's two children are given *fixed* sizes
  in millimetres, computed from the length the split was handed; the millimetre
  of seam between them is where a splitter goes when there is one, and nothing
  draws or hit-tests it today.
- `src/interface.h`, `src/interface.c` — the screen-filling surface. Pixels per
  millimetre from the window's height over a 135 mm surface, the records `ui`
  emitted submitted into the open frame, and one draw command per root. It
  decides nothing about what is on a panel, and its header says why the one
  question it does ask — what was clicked — has to be asked from in there.
- `src/scene.h`, `src/scene.c` — the four entities the editor opens on, built in
  code until there is a loader, and the selection. Its header says why three of
  the four are authored and the fourth deliberately is not, why the selection is
  the editor's and not the dock tree's, and why the rows the Scene panel drew
  have to outlive the call that drew them.

## Roots

`src/dock.h`'s `voe_editor_dock_root` is a surface with a tree on it: its size in
millimetres and its pointer in the same millimetres. The editor holds an array of
one and the loop walks every entry, so root zero is the window and a detached
panel would be a further root.

That does not make a second OS window cheap, and the header says so where somebody
would otherwise assume it: `platform` opens one window, `render` opens one device
onto one surface and `app` brackets one frame for it. A second root that is its
own window is a card in each of those three before it is a line here. What the
array buys today is that the walk takes a root rather than reading a global.

## What is deliberately absent

No splitter drag, no tab bar, no dragging a panel between regions, no closing
one, no scrolling, no clipping and no saved layout (ADR-0142 point 6). No
viewport, no camera, no light and no 3D. No inspector beyond the name of what is
selected — the real one is card 059 — and no create, no delete, no save and no
load. No pixel snapping anywhere — the
interface is world geometry and is not snapped to a pixel grid (ADR-0141 point 3).
