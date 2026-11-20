# 0512 — The key-table row is `voe_assets_documented_key`
date: 2026-11-20
by: planner

## Decision
The row type 0511 point 1 puts in `assets/sectioned.h` is named `voe_assets_documented_key`. The
existing `voe_assets_sectioned_key`, the parsed `key=value` line the sectioned reader hands back, keeps
its name. Everything else in 0511 stands.

## Reasoning
0511 gave the row the name the line type already had. Renaming the line type would touch `assets`,
`authoring` and `theme` for no gain to 116; a new name touches only the cards that use the row. The
name says what the row is: documentation of a key, not a parsed one.

## Replaces
The row type's name in 0511 point 1.
