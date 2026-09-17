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

## Open questions

- **The one that decides the shape:** ticking the box rebuilds the project (the person never sees
  CMake, but waits; every performance property today is kept), or the plugin is a separate file
  loaded at startup (instant and Blender-like, but gives up "nothing is read from disk at run
  time" and puts a pointer jump in front of calls that are inlined now).
- What a plugin is allowed to be — a folder of the engine's own, or something a third party writes.
- Whether a scene saved with a plugin's components opens in a project that has it switched off.
- Whether "per project" means the setting lives with the project rather than with the person,
  unlike the theme and font preferences.
- Whether the editor itself is subject to this, or only what a game links.

## Not yet discussed

Everything else. This file is the wish as it was said, written down so it survives a session; it
has not been through a spec interview and is not ready for approval.
