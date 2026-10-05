# 0358 — The GPU is measured pass by pass, and Best Practices warnings are a gate
date: 2026-10-05
by: tech-lead

## Decision
Every GPU pass is timed every frame, and the editor and dev can show a frame breakdown, each pass by name with
its cost in milliseconds. Every pass, image, buffer and pipeline carries a debug name, so a RenderDoc or Nsight
capture reads in the engine's own words. The debug build runs the Khronos validation layer with Best Practices on,
together with the vendor checks for the card the engine took (0214). From the day this lands, a Best Practices
warning fails the checks unless it is on an allowlist kept in `render`. That list starts as today's warnings, each
with one line saying why it stands. It only shrinks: a card may remove an entry, and adding one is a decision.
If the debug build cannot load the layer, it says so on startup and the checks fail; the layer is never silently
absent. Performance work on the renderer is argued from this breakdown, not from guidelines alone.

## Reasoning
An outside review against the vendor guides (2026-10-05) found real gaps: wait-idles, coarse barriers, GENERAL
layouts, one allocation per resource. Without per-pass timing there is no way to rank them, and no way to check
0316's "every costly feature is cheap when on". Warnings that block a card give the agents a mechanical rule
to work against.
- Report warnings without failing: they get ignored, as the start log already is.
- Fail on every warning from day one: it stops all render work until the backlog is cleared.
- Fix the review's findings straight away: that is guessing at costs we can measure first.

## Replaces
Nothing.
