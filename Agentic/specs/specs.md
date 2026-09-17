# specs

What the sponsor has asked for, one folder per feature. Not how it is built. Work before 001 was
done as kanban cards, kept in `history/cards/`.

- `001-scrolling-editor-panels/` — the editor's columns clip, scroll with the wheel or a scrollbar, and the inspector's rows wrap.
- `002-fields-of-any-shape/` — a component field may be an array of any kind with up to seven dimensions, and scene text saves and loads it nested.
- `003-render-to-a-texture/` — a frame can be drawn into a texture, used in the scene or saved as a PNG, with no display needed.
- `004-open-and-save/` — the editor opens and saves a project from a top bar and its own file browser, starts on an untitled cube and light, and reopens the last project.
- `005-editor-font/` — the editor's panels default to Pixel Operator, and Preferences switches them to Oxanium and back, remembered.
- `006-themes/` — interface looks come from a theme file of an accent, two sliders, a mode and a font; nearest theme wins, live in the editor.
- `007-plugins/` — optional parts of the engine are switched on per project from the editor's Preferences, off by default, without the person seeing CMake. Draft: the wish as it was said, not yet interviewed.

Order of work is not the order of the numbers: 003 was put ahead of 002 by the sponsor on
2026-09-15, because it makes every later feature cheaper to verify.

006 is built ahead of 005, by the sponsor's choice on 2026-09-15: 005's font choice sits on 006's
themes and Preferences.
