# NNN — <what the card delivers, in a sentence>

claimed-by: -
blocked-by: -
status: todo
decision: <ADR title in words, and its number> — one line on what it concluded

## Goal
What exists when this card is done, and in which folder.

## Scope
What to build. Named files, fields, functions where the decision fixed them.
Call sites in downstream folders this change will break, listed.

## What must not change
The neighbours and rules this card leaves alone.

## Verify
What to run, on which platform, and what it must say. `cmake -P check.cmake` green.

## Done looks like
The observable result, checkable by the human at review.

## Notes
Coder's notes: what ran, what it said, `DEVIATION:` / `BLOCKED:` markers, suggestions.

<!-- Ceiling 150 lines (ADR-0114). A card states, it does not argue: for *why*, name
     the decision above. Argument that needs to be here means the ADR is missing it. -->
