# 0188 — Play runs the scene on screen, unsaved changes included
date: 2026-09-19
by: tech-lead

## Decision
Play cooks the scene as it is in the editor at that moment, unsaved changes included. It does not
save the project, does not ask to save, and does not change whether the project is marked
unsaved. The files on disk change only when the person saves.

## Reasoning
Play is for trying something quickly, and the person expects to see what they are looking at.
Rejected: save automatically on Play, which quietly overwrites the file; Play only what was
saved, which makes trying an edit take two steps and surprises anyone who forgot to save.

## Replaces
nothing
