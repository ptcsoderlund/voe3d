# 0270 — Imported meshes come with an Assets panel
date: 2026-09-27
by: tech-lead

## Decision
Milestone 3 of 0268 (imported static meshes) ships with an Assets panel in the editor. The panel
shows the project's `Assets/` folder, with its subfolders, as the project's files. A `.glb` comes
into a project in one of two ways: Import in the panel copies it into the folder the panel shows, or
someone copies it into `Assets/` outside the editor and the panel picks it up. Dragging a model from
the panel into a scene view places a new thing there that draws that model. A model re-exported
from Blender over the same file updates everywhere it is placed, without a restart. The panel is
where later project files appear as they arrive: prefabs (milestone 5), sounds and whatever else a
project keeps. Only `.glb` is imported in 0.2. Other files in `Assets/` are listed, and nothing
more is done with them until a feature says so.

## Reasoning
The sponsor will import dozens of models for the tank game. Typing file paths into the Inspector
would make level building slow, and an asset browser was already on the ideas list as the home of
prefabs. Alternatives: a path field on a mesh component only (cheap, but miserable for dozens of
models); a full asset database with ids and import settings (more than 0.2 needs, and a format of
our own under 0236).

## Replaces
nothing. It amends 0268 milestone 3, and it takes the asset browser off the ideas list.
