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
- **A visual shader editor** (node graph); a material is an instance of a shader (0189). The material
  editor is work order 085.
- **A runtime-GUI editor** for what the cooker outputs, itself drawn in the editor themes (0194). A game's GUI
  draws with a theme; per-element property overrides and a theme override are how it gets colour on purpose.
  After the coin game, whose GUI is written in code (0194). The editor then sets the file format, from what
  that game actually needed.
- **A scale gizmo**, and a toggle for the move and rotate gizmos to follow the entity's own axes rather than
  the world's (left out of 015 and 035). Typing scale in the Inspector does until then.
- **A developer picks the graphics card from code**, and a finished game lets the player pick one in its
  settings — both later, once 0201's automatic choice is in and proven.
- **A view recorder**: something in the game that films from its own place and renders to a texture, for a
  security camera on a screen or anything else that must film in game (sponsor, 2026-09-23). Not a second camera
  (0218); builds on render to a texture (003).
- **A UI scale** that grows panels, rows and everything with it, beside 0219's text size (sponsor, 2026-09-23).
  Per-element overrides of a theme setting are the other half (0194).
- **One shared engine build per editor** that every project links, so only the first Play on a machine
  pays the engine compile (0235).
- **A developer's own shading**: custom materials or shaders for a game that lights things its own way
  (sponsor, 2026-09-24). Until then, no light means black in the game and the preview light in the editor (0287).
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
- **LOD streaming** (sponsor, 2026-09-25; the levels themselves are work order 089): only the
  levels near the one on screen (current ±1) sit in VRAM, the rest on disk, loaded ahead through
  ADR-0118's asynchronous tier. Recommended shape: the lowest level of every model always stays
  loaded, so a teleport shows something coarse rather than nothing; loading is prioritised by
  where the camera is heading; and one VRAM budget replaces a per-game mode: a low-poly game fits
  entirely and never streams, a high-detail game streams. A game can ask to preload around a
  place before teleporting there.
- Explicit system ordering (Bevy-style `.before`/`.after`, system sets) for when a slot's systems
  run on several threads; until then list order within a slot is enough (0256).
- **Entity groups in the Scene list** (sponsor, 2026-09-26): a filter box at the top of the list, and
  whether selecting a group selects what is in it (move the whole group with the gizmo). The groups
  themselves became 0300: a bare entity parents others and its row folds.
- Split a multi-object `.glb` into a tree of things on import (a body with a head child), so moving
  parts need not be exported one file each. Set aside for 0.2 by 0280; worth it once parenting (037)
  exists and exporting parts one by one becomes a chore.
- **Duplicate copies the whole tree** (sponsor, 2026-10-03): the children come along with the copy,
  for kit-bashing. Set aside: prefabs (038) cover reuse for now. If it comes back, the sketch was:
  the camera and sun are left out of the copy, links inside the tree point at the copy's own parts,
  and a duplicated child stays under its parent. The whole copy is one undo step.
- Splash customisation: box position, minimum time, fade, a splash per scene (0346 leaves these for later).
- **Renderer work parked from the Vulkan review** (2026-10-05, see 0358): rank by 062's frame breakdown before
  building any. Runtime `vkDeviceWaitIdle`s go (timeline semaphores, a deletion queue, a staging ring,
  update-after-bind textures) with the first work order that loads content during play, i.e. streaming. A
  suballocator, written by us (ADR-0023), not VMA, when allocations near the 4096 floor. Block-compressed textures
  (BC7/BC1, encoded in the cook) when a texture budget needs it (0359). Async compute and a transfer queue,
  more dynamic state, subgroups and fp16, cached readback: only if the breakdown shows the gain.
  Precise in-frame barriers and leaving GENERAL layouts are planner's work once they can be measured.
- Revisit 058 (several directional lights) once a real game needs a sun and moon or a lit cave; it was accepted on theory, without a concrete use to judge it by.
- **F with nothing selected frames the whole scene** (sponsor, 2026-10-05): left out of 065, which frames only a selection.
- **Wind in the foliage** (2026-10-07, from 0376's road): grass and leaves sway in a wind that has a
  direction and strength. Left out of 090/091 to keep them testable in one sitting.
- **Backlit leaves** (2026-10-07, was work order 077): thin leaves glow on their shaded side when the sun
  is behind them. Cheap, and what makes trees read as trees from the tower.
- **The rest of glTF** (2026-10-07, was work orders 067, 073–080): `.gltf` with side files, every
  primitive mode, a model's own and spot lights, specular, clearcoat, sheen, glass, iridescence,
  anisotropy, variants. Each waits until a game needs it. The loghouse's lamps may want spot lights.
- **A project's own shaders, made in the editor** (2026-10-07, from 0377): Create → Shader in the
  Assets panel. 0.3's materials use the engine's shaders; this waits for a work order of its own.
- **Editing text in a field, all the way** (2026-10-07, asked after testing 081: "I would like to be able
  to move a cursor instead of having to retype the whole string"; "When we do it we do it all the way").
  `ui`'s one shared text box (0192) has no cursor: it opens all selected and typing replaces it. Done in one
  go, never in part: a cursor moved by ←/→, Home/End and a click; Ctrl+←/→ by word; Backspace/Delete at the
  cursor; selection by Shift+keys, drag and double-click; copy, cut and paste through the system clipboard
  (Wayland's is a piece of `platform` work). Every field gets it at once: Rename, Inspector names, typed
  numbers, hex. Waits until after 093; amends 0192.
