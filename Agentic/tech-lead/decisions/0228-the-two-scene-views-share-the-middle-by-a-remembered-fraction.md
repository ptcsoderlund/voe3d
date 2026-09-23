# 0228 — The two scene views share the middle by a remembered share
date: 2026-09-23
by: tech-lead

## Decision
The border between the two scene views can be dragged like the borders of 021 (0226): the pointer shows it,
each view keeps a minimum room, and a double-click puts it back to an even split. The views divide the
middle by a **share**, not a length: at 70/30 they stay 70/30 whatever the window's size. The share is kept
in the editor's own settings beside the panel sizes, so every project opens with it as last left.

## Reasoning
A share, because the two views are equals and neither is a tool panel that needs a steady width; the side
panels keep lengths (0226) because a list or an inspector is read at a size. Rejected: the left view keeps
its width like a side panel (one view would swallow a growing window); dragging to an edge closes a view
(not asked for, more work).

## Replaces
nothing
