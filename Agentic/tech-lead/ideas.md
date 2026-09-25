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
- **A runtime-GUI editor** for what the cooker outputs, itself drawn in the editor themes (0194). A game's GUI
  draws with a theme; per-element property overrides and a theme override are how it gets colour on purpose.
  After the coin game, whose GUI is written in code (0194). The editor then sets the file format, from what
  that game actually needed.
- **Rotate and scale gizmos**, and a toggle for the move gizmo to follow the entity's own axes rather than the
  world's (left out of 015 on purpose, 2026-09-19). The sponsor would like the rotate gizmo drawn on top of the
  move gizmo, both at once, in world space (2026-09-23); typing rotation in the Inspector does until then.
- **A developer picks the graphics card from code**, and a finished game lets the player pick one in its
  settings — both later, once 0201's automatic choice is in and proven.
- **An asset browser** where a project stores its assets and prefabs (sponsor, 2026-09-23). Not on 0186's road
  until the coin game needs it; duplicating an entity covers twenty coins. Prefabs and a scene clone (above)
  would land here when it comes.
- **A view recorder**: something in the game that films from its own place and renders to a texture, for a
  security camera on a screen or anything else that must film in game (sponsor, 2026-09-23). Not a second camera
  (0218); builds on render to a texture (003).
- **A UI scale** that grows panels, rows and everything with it, beside 0219's text size (sponsor, 2026-09-23).
  Per-element overrides of a theme setting are the other half (0194).
- **A game window setting in the project**: size, and windowed or fullscreen, for the game Play starts and
  the shipped game (sponsor, 2026-09-24). 0234 fixes 1280×720 until then.
- **One shared engine build per editor** that every project links, so only the first Play on a machine
  pays the engine compile (0235).
- **More than one light in a scene**: a frame draws at most one today (sponsor, 2026-09-24).
  0238 makes none allowed.
- **A developer's own shading**: custom materials or shaders for a game that lights things its own way
  (sponsor, 2026-09-24). Until then, no light means unlit (0238).
- **A `character_controller` component and system in the engine**, with physics kept separate from
  simple collision, and collision perhaps on the GPU through compute (sponsor thinking aloud,
  2026-09-24). A project's `player_system` would drive the controllers whose entity also has its
  `keyboard_input` (0239). To settle before milestone 4.
- **Draw every script as real letters, not the missing-glyph box**: Latin Extended (ł, ő, ș…),
  Greek, Cyrillic, then Chinese/Japanese/Korean, Arabic and Hebrew. That needs fallback fonts
  beside Oxanium (0185), glyphs beyond U+00FF and, for some scripts, shaping and right-to-left
  text. The sponsor wants it (2026-09-25). Wait until a game needs it. Until then, 0247 only
  promises that a character never fails.
