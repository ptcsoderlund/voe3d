# 0354 — Everything placed in the scene is clicked by its mesh or a marker
date: 2026-10-04
by: tech-lead

## Decision
In the editor's scene views, every entity with a transform can be selected by a click. An entity that draws a
mesh (shape, model, water) is clicked on its mesh as today. Every other entity with a transform shows a small
line marker at its place, drawn in the editor only and never in the game: lights keep or get a marker of their
own shape (the directional light's from 0274), the camera keeps its marker (0223), and everything else (a light
blocker, an emitter, a sound, a bare transform or parent, a project's own component) gets one plain marker
shared by all. A light blocker also always shows its box as faint lines, brighter when selected (0347's
selected lines). Clicking inside a blocker's box does not select it; only its marker does. When a click falls
on a marker and on a mesh at the same point, the marker wins, whichever is nearer. Markers are always shown;
there is no toggle to hide them.

## Reasoning
The sponsor's call (2026-10-04). Line markers are what the editor already has for the camera and the light, so
there is one look and no image assets. The marker wins because it is small and the click on it was meant.
Rejected: camera-facing picture icons per type, which need an icon set and a new way to draw; selecting a
blocker by clicking its volume, which would hide everything inside a room from clicks; nearest-wins, which
makes a marker inside a house reachable only from the Scene list; a hide toggle now, which can come when
markers get in the way.

## Replaces
nothing. Extends 0202, 0223, 0274, 0347.
