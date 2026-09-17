# 007 Plugins

Status: draft
Approved: -
Accepted: -

Optional parts of the engine — a foliage and landscape folder is the sponsor's example — are turned
on and off per project by someone who does not write code and has never seen CMake. The editor's
Preferences offers them the way Blender does: a list, a checkbox each. Everything is off by
default; a project takes only what it asks for. A programmer working without the editor is on
their own with CMake and Clang, and that is accepted.

## The sponsor's words, 2026-09-17

> We want to enable this to non programmers or non cmake programmers. Like how blender does with
> the preference and then checkbox for a plugin. But we want everything off and per project. In the
> editor, people might want to dev without editor. Then they are on their own with cmake and clang
> (maybe even other compilers).

## Answered

- **The shape, answered 2026-09-17:** ticking the box rebuilds the project. The sponsor's words:
  "rebuild straight away, C is too damn fast." Nothing is loaded at run time, so every performance
  property the engine has today is kept, and the editor writes the build file a programmer would
  otherwise write by hand.
- **Editor plugins are out of scope, 2026-09-17.** The sponsor asked whether the editor itself
  should be moddable. Skipped until the editor is a full product: an editor plugin needs stable
  places to hang — a panel, an Inspector row, a menu item — and those are still being laid down.
  Deferring is cheap precisely because the answer above is "rebuild": an editor plugin would be
  another folder the editor links and a rebuild of the editor, the same mechanism, not a second
  design. It becomes its own spec once the editor has settled.

## Open questions
- What a plugin is allowed to be — a folder of the engine's own, or something a third party writes.
- Whether a scene saved with a plugin's components opens in a project that has it switched off.
- Whether "per project" means the setting lives with the project rather than with the person,
  unlike the theme and font preferences.
- Whether the editor itself is subject to this, or only what a game links.

## Not yet discussed

Everything else. This file is the wish as it was said, written down so it survives a session; it
has not been through a spec interview and is not ready for approval.
