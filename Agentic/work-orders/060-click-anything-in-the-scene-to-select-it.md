# 060 — Click anything in the scene to select it

## What
In a scene view I can click anything that has a place in the scene and it becomes selected, just like clicking
a shape does today. Things with a mesh I click on the mesh. Things without one (a light blocker, a point light,
a particle emitter, a sound, an empty entity that only has a transform, a parent with no mesh, a game's own
things like a spawner) each show a small line marker where they are, and I click the marker. Lights and the
camera have markers of their own shape; everything else has one plain marker. A light blocker also always shows
its box as faint lines, brighter when it is selected, but clicking inside the box does not select it, only the
marker does. If a marker sits in front of or inside a mesh, clicking the marker selects the marker's thing.
Markers are only in the editor, never in the game (0354).

## Why
Today a blocking volume and other things without a mesh can only be selected from the Scene list.

## How to test
1. Open the tank game. Every light blocker shows its box as faint lines and a small marker; every point light,
   emitter and sound shows a marker. Nothing without a transform shows a marker.
2. Click a blocker's marker. The blocker is selected: the Inspector shows it and its box lines get brighter.
   The move gizmo appears on it and moving it moves the box.
3. Click inside a blocker's box, on the floor of the house. The floor is selected, not the blocker.
4. Click a point light's marker, then an emitter's, then a sound's. Each is selected in turn.
5. Add entity, give it only a transform, and move it somewhere empty. It shows the plain marker; click away,
   then click the marker: it is selected again.
6. Place a marker inside a model (for example a light inside the house) where it shows through. Clicking the
   marker selects the light, clicking the house beside it selects the house.
7. Press Play. No markers or blocker lines are seen in the game.
