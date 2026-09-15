# 0009. Renderer first; editor, physics and audio are later, not never

**Rule:** Work proceeds to a working renderer before any other engine domain is
started. Editor, physics, audio and the asset pipeline are **later**, not
non-goals — they are wanted, and they are on hold until a renderer runs.

**Status:** Accepted · 2026-08-28
**Amends:** the non-goals list in `docs/prestudy/README.md`, which recorded
editor, physics and asset pipeline as deferred *non-goals*. Networking and
scripting remain non-goals and are unchanged.

**Why:** Principal's call. The first version already described in `STATUS.md` is
renderer-shaped — window, input, depth, model, textures, camera, one light,
maths, frame loop, statistics — and it is taken as the v1 cut unchanged.

**The consequence that matters — the editor's constraint arrives now.** The
register recorded editor as revisitable "only if it forces a
reflection/serialisation requirement into core." **That trigger has fired.** The
principal requires editor data to be text-based so that AI agents can take part
in developing games with it (ADR-0010). Text serialisation of components is a
requirement on the ECS itself, and it must be known while the module map is
drawn — even though the editor is years away.

This does not mean building editor features now. It means:

- Component data must be describable well enough to be written and read as text
  (**D-024**). Do not design a component type that cannot be.
- Engine state that must be editable later belongs in components, not hidden in
  a system's private variables (ADR-0007).

**Cost accepted:** Physics and audio may later want a say in the scene and
transform representation, and will arrive after ADR-0007 has fixed it. Accepted
knowingly — the alternative is designing for three domains that do not exist
against a renderer that does not run either.

**Also settled:** audio is *later*, closing the open question of whether it was
in scope at all.
