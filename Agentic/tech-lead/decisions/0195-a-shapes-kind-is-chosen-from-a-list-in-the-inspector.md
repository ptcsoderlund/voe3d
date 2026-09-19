# 0195 — A shape's kind is chosen from a list in the Inspector
date: 2026-09-19
by: tech-lead

## Decision
A shape's kind is editable: the Inspector shows it as a dropdown of the built-in kinds by name (Cube, Capsule,
Cylinder), and choosing one changes what that entity draws, live, and marks the project unsaved like any other
edit. The dropdown is a general "one of a named list" field, not a shape special case: the Inspector still names
no component (ADR-0134), and any described field whose values are a fixed named set shows the same way. A saved
scene keeps the kind as it does today. This amends 0191 where it says the intent puts `kind` back to the entity's
own.

## Reasoning
The sponsor found that Add → Entity → Shape can only ever give a cube, and that swapping a shape means deleting
and re-adding it, which is tedious when laying out a level. A general list field pays again for a light's type or
a material's alpha mode. Rejected: keeping kind read-only (delete and re-add); a typed number for kind (works, but
not for a non-programmer); a shape-only dropdown (breaks the Inspector naming no component).

## Replaces
Amends 0191 (kind no longer read-only).
