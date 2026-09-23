# 0220 — The editor's panel sizes live in the global editor settings file
date: 2026-09-23
by: tech-lead

## Decision
The sizes the person gives the editor's panels — the Scene list on the left, the Inspector on the right and the
top bar — are kept in **one global editor settings file** for the person, not in the project, so every project
opens with the panels as they were last left. It is written when a resize ends. A missing or unreadable file
means the default sizes.

## Reasoning
The sponsor's call (2026-09-23): the layout is how a person likes their editor, not part of a game. Rejected:
per project, which makes every new project start from the defaults again; not remembered, which undoes the
point of resizing.

## Replaces
nothing
