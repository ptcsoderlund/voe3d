# 0306 — The editor is laid out for a PC monitor
date: 2026-10-01
by: tech-lead

## Decision
The editor is laid out for a PC monitor at desktop viewing distance, not for touch. Its text at the
text-size slider's 100 % is 80 % of the size it had before 054, and its spacing (padding, row and
button heights, gaps between widgets) is 65 % of what it was. The new sizes are the editor's 100 %:
the slider still reads 100 % by default, and a remembered text scale keeps its number and so shrinks
with the base. A game's interface keeps its sizes, and the default panel widths and minimums stay as
0226 and 0229 set them. A game's interface and the panels are sized again later, with a GUI editor
in the editor and with docking.

## Reasoning
The editor read as a touch panel: big buttons and wide gaps. Spacing shrinks more than text because
the empty space is what makes it look made for touch. Desktop toolkits typically use text about 20 %
below phone sizes.
Alternatives: text only at 85 % (buttons stay big); a Comfortable/Compact switch (more work, and
nobody needs Comfortable now); narrower default panels (left for docking).

## Replaces
nothing
