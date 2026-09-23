# 0218 — A scene has exactly one camera, never none and never two
date: 2026-09-23
by: tech-lead

## Decision
Every scene has **exactly one camera**, on an entity of its own, and it is what the game sees through. A new
scene is made with it, a scene opened without one gets one, it cannot be deleted, its camera component cannot
be removed, and Add component never offers a second camera. It is moved and aimed like any entity. The engine
has **no camera framework**: no blending, no switching, no priorities. Anything more, such as a third-person
follow, is the game's own code moving that one camera. Filming something else in the game (a security camera
shown on a screen) will be a separate thing that renders to a texture, not a second camera (see `ideas.md`).

## Reasoning
The sponsor's call (2026-09-23): a simple ECS where people build everything themselves, so the engine gives
one camera and no rules about several. Never none means Play (0186, milestone 2) always has something to show.
Rejected: several cameras with one ticked as the game's, which needs rules for which one wins; no camera until
one is added, which leaves Play with nothing to show.

## Replaces
nothing
