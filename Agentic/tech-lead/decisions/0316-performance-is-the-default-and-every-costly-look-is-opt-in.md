# 0316 — Performance is the default, and every costly look is opt-in
date: 2026-10-02
by: tech-lead

## Decision
VOE3D makes games that as many people as possible can play, not only those with high-end cards; the editor
and the engine are built for performance first. A clean scene in the editor is feature-empty: the developer
turns every costly look on themselves. Any feature that costs GPU or CPU time every frame beyond the plain
lit look (shadows, bounce light, later effects) is off on a new scene, a new light and a new thing, and is
turned on in the editor, saved with what it belongs to. Turned off, it costs nothing: no pass, no image, no
update. A new scene's light, and the editor's preview light, light without shadows and without bounce. A
mesh keeps casting shadows by default, since that costs nothing until a light casts. Scenes saved before
this keep the shadows they had; the bounce was never saved, so it is off in every scene until turned on.
Every costly feature is also built to be cheap when on.

## Reasoning
The sponsor's standing call (2026-10-02, "we said performance as default, with opt-in"; "a clean scene in
editor should be feature empty, devs have to turn stuff on themselves"), never written down until now, so
046 shipped bounce light always on.
- Costly looks on by default, opt-out: a new project is slow before its maker has chosen anything.
- A global quality preset instead of per-feature switches: hides what costs what; can come later on top.

## Replaces
Nothing. Amends 0307 (the bounce is opt-in) and 0289 (the new scene's light, and so the preview light,
casts no shadows).
