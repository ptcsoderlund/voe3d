# 0429 — A .glb's levels are set per model in the Inspector, and Re-import redoes the model
date: 2026-10-15
by: tech-lead

## Decision
For 089 bugs 02 and 03. The Inspector shows whatever was pressed last: an entity in a scene view or the scene
list, or a file in the Assets panel, the same way, with that thing's settings and tools. 0399 point 8 already
does this for a `.material`; a `.glb` is the second file to get it, and any later file type with settings joins
the same way. A selected `.glb` shows its **level settings**, kept per model in a tracked file beside the `.glb`
(no project default, no per-target profiles): how many made levels it has (0 to 3; 0 means it is always drawn
whole) and, for each, the triangles it keeps as a percentage of the full model, each below the one before
(defaults 50, 25, 12.5, as 0425 point 1). Changing a setting remakes that model's levels at once and touches
nothing else; the levels cache is stale when the settings change, as when the file does (0426). Under them the
Inspector shows what each level actually reached (triangles and percentage), so a level the simplifier could
not bring as low as asked says so; when a level is shown on screen stays 0425 point 3's rule, so a heavier
level simply appears only when the model is smaller. A `.glb` with its own Blender levels shows that it brings
its own and no settings. **Re-import** — on the `.glb`'s right-click menu and as a button in its Inspector —
reads the file again under the splash: materials and parts written afresh as 0403 and 0420 say (edits to those
materials lost, PNGs untouched), and levels remade with the model's current settings, which Re-import keeps.

## Reasoning
The human's calls: settings per model only, count plus a percentage per level, edited in a contextual
Inspector shared by the scene view and the Assets panel. One project is one game, so a phone game sets its
models lighter where it wants them lighter; a background model gets heavy levels. Rejected: a project-wide
default (the human wants it per model); per-target profiles (only desktop ships today, see ideas.md); one
strength slider (no per-step control); a switch-sooner bias (the error rule already holds a heavy level back
until it is small); a settings dialog on Re-import (changing levels would always cost material edits).
Models imported before 089 need no Re-import for levels: their levels are made when they are loaded (0426).

## Replaces
Amends 0425 point 1 (the fixed halving becomes the default) and 0399 point 8 (the Inspector's file case
becomes general).
