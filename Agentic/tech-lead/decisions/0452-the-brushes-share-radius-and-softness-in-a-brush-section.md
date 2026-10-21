# 0452 — The brushes share Radius and Softness in a Brush section
date: 2026-10-21
by: planner

## Decision
1. The Inspector gets a Brush section holding what every brush shares: Radius (m) and Softness, as
   sliders with the ranges sculpt.h gives them today. One radius and one softness serve all brushes,
   as before; the ring drawn in the view does not change.
2. It is drawn on a thing that wears a landscape while any brush is chosen (a sculpt kind, PAINT,
   SCATTER or ERASE), and not otherwise.
3. It is drawn after the Foliage section and before the LOD section, so choosing or unchoosing a
   brush moves no button above it: the button just pressed stays under the pointer.
4. The Sculpt section keeps Strength alone (paint strokes with it too); Foliage keeps Density, Size,
   Turn and Lean. Its nodes and figures live in the sculpt state as the other sections' do; it is
   forgotten with them and read after Foliage.

## Reasoning
090 bug 03: the sponsor had to go to Sculpt to size a foliage brush, and Strength sat beside Radius
doing nothing to foliage. Shown only while a brush is in use, as the bug asks; placed below the
sections whose buttons choose the brush because a section that appears above a button moves it away
from the pointer that just pressed it.

## Replaces
Amends 0379 point 3 and 0442 point 3: Radius and Softness leave the Sculpt section.
