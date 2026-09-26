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
- **GPU particles that collide with the world**: compute-shader particles colliding one way
  against an uploaded copy of the collider components or the depth buffer (0249 rule 4).
- **Dynamic bodies**: things pushed by forces, stacking and tumbling, physics layer 3 of 0249;
  solved in islands so it can go to threads later. Not needed for the coin game.
- **Draw every script as real letters, not the missing-glyph box**: Latin Extended (ł, ő, ș…),
  Greek, Cyrillic, then Chinese/Japanese/Korean, Arabic and Hebrew. That needs fallback fonts
  beside Oxanium (0185), glyphs beyond U+00FF and, for some scripts, shaping and right-to-left
  text. The sponsor wants it (2026-09-25). Wait until a game needs it. Until then, 0247 only
  promises that a character never fails.
- **World streaming for open worlds**: the world in cells loaded and unloaded around the player,
  through ADR-0118's asynchronous tier, which today keeps physics shapes synchronous; streaming
  collision near the player needs that revisited. Builds on 0250's double positions (sponsor,
  2026-09-25).
- **Automatic LOD at run time with cached baking** (sponsor, 2026-09-25): "like Nanite but with
  good performance, more like traditional auto LOD". A mesh gets simplified levels generated
  automatically (quadric-error simplification in the meshoptimizer style), baked on first use
  and kept in the project's `Cache/` (0235), picked per object by its size on screen, blended
  so levels do not pop. No per-cluster virtual geometry. Needs imported meshes first; today a
  project draws only the built-in shapes. **LOD streaming** (sponsor, 2026-09-25): only the
  levels near the one on screen (current ±1) sit in VRAM, the rest on disk, loaded ahead through
  ADR-0118's asynchronous tier. Recommended shape: the lowest level of every model always stays
  loaded, so a teleport shows something coarse rather than nothing; loading is prioritised by
  where the camera is heading; and one VRAM budget replaces a per-game mode: a low-poly game fits
  entirely and never streams, a high-detail game streams. A game can ask to preload around a
  place before teleporting there.
- **Dynamic lighting with the same philosophy** (sponsor, 2026-09-25): no baked lightmaps and
  no editor bake step; lighting computed at run time and cached. Starts from 0252's sun shadows.
  The sponsor's shape for the bounce: **a lit surface becomes a light** — surfaces the sun hits
  store what they received (times their colour; dark surfaces send less) in a surface cache and
  light what they see, a slice of surfaces re-lit per frame so the whole cycle takes ~50 ms. This
  is radiosity, the shape of Enlighten and of Lumen's surface cache. The open question is who
  sees whom: traced at run time (hardware ray tracing or a distance field of the world), or a
  per-surface visibility baked automatically and kept in `Cache/` like the LOD idea, with moving
  things lit from probes. Neither needs ray-tracing cards: cached visibility and distance fields
  run on any card. The sponsor also proposed **light-blocking volumes** a developer places
  (static, so never re-checked): light does not pass them either way. Recommended as a hand
  touch-up for the leaks the automatic visibility misses (thin walls, coarse probe grids), like
  Unity's probe adjustment volumes, and as rooms and portals (a room lit only by what is inside
  and what comes through its doors and windows), not as the main mechanism. One bounce first, more by feeding the cache back into itself. After the
  coin game, unless a game needs it sooner.
- **Fill light for shadows** (2026-09-26): 028 has no ambient (0258), so shadowed sides are flat
  dark. A cheap stopgap was offered: a two-colour sky/ground fill term, a few lines of shading,
  replaced later by the bounce. The sponsor chose to wait; revisit if the coin game's shadows read
  too black.
- **A sky** (sponsor, 2026-09-26): a sky that is drawn, i.e. a panorama or sky dome and later
  volumetric clouds, which could also feed the fill light and the bounce. Wanted sooner or later;
  too big for now.
- Explicit system ordering (Bevy-style `.before`/`.after`, system sets) for when a slot's systems
  run on several threads; until then list order within a slot is enough (0256).
