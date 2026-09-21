# 0204 — Undo is a line of scene texts, and a step is what an edit settles into
date: 2026-09-21
by: planner

## Decision
For spec 014.

- **A step is the whole scene, as the text a save writes.** `authoring/scene_write.h` already turns a world
  into those bytes and `authoring/scene_read.h` puts them back, and the same world is the same bytes every
  time. The editor keeps a line of those texts and undoes by reading one of them back into the project's
  world. Nothing records what changed: no command objects, no inverse per kind of edit, and no folder outside
  `editor` gains anything for undo's sake.
- **Restoring destroys every authored entity through `ecs`'s structural queue, applies it, and reads the text
  into the same world** (0190) — the same tables, the same registrations, not a new project. Entity handles
  do not survive it, so the selection is re-found by the identity id it had before the step and cleared when
  that id is no longer in the scene. Selecting is not an edit and is never recorded.
- **A step is bounded by rest.** An edit reaching the project marks the history; the world is written and
  compared only on a later frame at which the pointer's primary button is up, no field or number box holds
  the keyboard (`voe_ui_typing`), the colour picker and the dropdown are closed and the browser is hidden.
  One drag, one typed commit and one visit to the colour picker are therefore one step each. The known cost:
  Tab from one field to the next never lets the keyboard go, so a row of numbers filled that way undoes as
  one step.
- **The line is a fixed ring of texts** — 64 states of 32 KB, in the history's own arena, the oldest dropped
  when it is full — and a scene text that does not fit empties the line rather than recording half of it. A
  project's world holds at most 32 authored entities (`VOE_EDITOR_SCENE_ROWS`), so 32 KB is a scene over
  again.
- **The line belongs to the project being worked on.** New and Open empty it and record the fresh project as
  its first state; Save does not touch it; an undo leaves the project marked unsaved.
- **A step is taken at the top of a frame**, before the structural queue is applied and the systems run, so
  the rows put back are given their meshes before anything draws them.

## Reasoning
The feature asks for nine kinds of change to be undone — a number, a name, a colour, a shape's kind, an add, a
duplicate, a delete, a component added and one removed — and an undone delete has to come back with its id,
its name and every component's values. The text carries all of that already, is written by code that is
tested, and costs one path instead of nine; a tenth kind of edit tomorrow costs none. The restore is one call
per authored entity plus one read, on a scene of at most 32 entities, on a frame a person asked for it, and
the write is once per settled edit and never per frame.

Rejected: a command record per kind of edit, which is nine inverse paths to keep in step with every new
component and every new control, and which has to copy a deleted entity's rows by hand — the one case the
text does for free. Rejected: keeping a second world as a copy, which doubles every table and has no copy
call to make it with. Rejected: a diff between texts, which buys memory this program does not need and adds
the one thing a text line does not have.

## Replaces
Nothing. It reads 0190 for the structural queue, ADR-0149/0150/0151 for the scene text and 0192 for what
holds the keyboard.
