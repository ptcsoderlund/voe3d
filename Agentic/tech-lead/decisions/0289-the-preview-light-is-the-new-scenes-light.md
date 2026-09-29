# 0289 — The preview light is the new scene's light
date: 2026-09-29
by: tech-lead

## Decision
The editor's preview light (0287) is the light the editor puts in a new untitled scene: the same
direction (down and from the front-right), colour, strength, fill and shadows. The untitled scene's
light and the preview light share one set of values, so changing one changes the other. A light
added through Add component keeps its current defaults: white, strength 1, fill 0, and it shines
along its entity's -Z (0273). The shared values belong to the editor alone.

## Reasoning
Taken literally, 0287's "a newly added directional light" shines sideways, so a lightless prefab
would draw its top faces black. That is worse than drawing it flat, and worst of all in the tank
game's top-down view. The untitled scene's light already looks right. Godot does the same: its
preview sun tilts down, and a light the user adds points straight ahead.
Rejected alternatives:
- literal 0287: the preview draws upward faces black and shadows stretch sideways.
- every new light shines down by default: this changes Add component across `scene` and `ecs`,
  well beyond a bug fix.

## Replaces
Nothing. It amends 0287: "a newly added directional light" now means the new scene's light.
