# Ideas

Far-future thoughts. Pruned by the secretary when one becomes a decision or a work order.

- **Plugins** (was spec 007, a draft never interviewed; the sponsor's words on 2026-09-17: "We want to enable
  this to non programmers or non cmake programmers. Like how blender does with the preference and then checkbox
  for a plugin. But we want everything off and per project. In the editor, people might want to dev without
  editor. Then they are on their own with cmake and clang (maybe even other compilers)."). Optional parts of the
  engine — a foliage and landscape folder is the example — switched on per project from the editor's Preferences,
  a list with a checkbox each, everything off by default; a programmer without the editor is on their own with
  CMake and Clang. Answered on 2026-09-17: ticking the box rebuilds the project ("rebuild straight away, C is too
  damn fast") — nothing is loaded at run time, and the editor writes the build file a programmer would otherwise
  write by hand. Also answered: the editor itself is not moddable until it is a full product; an editor plugin
  would be another folder the editor links and a rebuild, the same mechanism, once panels, Inspector rows and menu
  items are stable places to hang. Still open: what a plugin may be (a folder of the engine's own, or a third
  party's); whether a scene saved with a plugin's components opens in a project that has it off; whether "per
  project" lives with the project rather than the person; whether the editor is subject to it or only what a game
  links.
- **A screenshot key in the editor** while you are using it — asked when 003 was approved and left out; worth
  asking again now that `--capture` works.
- **A list of recent projects** in the editor, and **Save As never**: moving or copying a project is done outside
  the editor; copying a scene will be a clone in a future asset browser (from 004's out-of-scope list).
- **Key repeat, dead keys and input methods** for typing in the editor (left out of 004 and 008).
- **Caps Lock and the `€` on AltGr+5**: left out of 008 on purpose; a later feature adds the legacy keysym blocks
  together with a font that can draw them.
- **Visual logic** as an opt-in editor plugin (node graphs, Blueprint-like), off by default, that produces
  what hand-written C would (0187). After the coin game (0186).
- **A visual shader editor** (node graph) and **a material editor**, two separate editors; a material is
  an instance of a shader (0189). They replace a shape's lone colour.
- **Drag and drop into the scene**: place a new thing where it is dropped rather than at the origin (0189).
