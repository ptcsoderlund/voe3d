# 0379 — A landscape is a text grid of heights worn as a model, and undone by the stroke
date: 2026-10-07
by: planner

## Decision
For 082, carrying out 0376's sculpting and 0377 point 1's landscape asset:
1. **The file.** `<name>.landscape` in `Assets/`, sectioned text (0236): `[Landscape]` with `size=` metres a
   side (16–8192, 256 when Created) and `cells=` per side (a multiple of 4 up to 512; Create writes 512),
   then `[Heights]` with `row0=` … `row<cells>=`, each `cells + 1` whole millimetres separated by spaces;
   row r lies at z = −size/2 + r·size/cells, value c at x = −size/2 + c·size/cells, about the thing's
   origin. Everything is in the one file, so a rename, move, duplicate or delete is 081's unchanged.
2. **Worn as a model.** A placed landscape is a thing whose model row (0277) names the `.landscape`; the
   model store loads it as 16 parts, one per chunk of a 4 × 4 split, all wearing one lit opaque material
   (linear base colour 0.24, 0.30, 0.16, roughness 0.9, metallic 0) until 085 paints it. So it is drawn,
   lit, shadowed, casts, fades, saves, undoes, cooks and is placed by an Assets drag as any model is. Its
   CPU shape is empty: the pick, the bounds and a drop's point meet its heights instead; it has no outline.
   It is never re-read by stamp. A bouncing light fits its probe volume to it, coarse at 1 km, until 083.
3. **Sculpting.** The Inspector of a thing wearing a landscape has a Sculpt section: Raise, Lower, Smooth,
   Flatten (pressing the chosen one again chooses none), Radius 1–200 m (20), Strength 0.05–1 (0.5) and
   Softness 0–1 (0.5). While one is chosen and such a thing is selected, its gizmo is hidden and a left
   press in a view over its ground sculpts instead of picking, grabbing or dropping; Escape chooses none.
   The weight is 1 inside radius·(1 − softness) and smoothsteps to 0 at the radius. Per second, at most
   0.1 s a stamp: raise and lower add ± strength·radius·0.5·weight metres; smooth and flatten move a height
   toward its 8 neighbours' mean, or toward the height under the press, by min(1, 10·strength·weight·s).
   A held drag stamps along the segment from the last hit at most radius/4 apart, the frame's seconds
   shared among them. The circle is two rings on the ground, at the radius and at the full-weight radius,
   coloured from the gizmo's rest colour to the outline's by strength.
4. **Live without a stall.** While a stroke is held its touched chunks are built each frame and drawn as
   transient geometry; between frames with no stroke held they are uploaded again as static ones.
5. **One stroke, one step.** A stroke is a state on 0204's undo line whose scene text is the one before
   it, carrying the touched rectangle's heights before and after; stepping over it writes one or the
   other into the store. It marks the project unsaved.
6. **Saved with the project.** Save writes every landscape edited since it was loaded or saved; a New or
   Open reads an edited one from its file again; a rename or move renames the store's entry. Size is set in
   a Landscape panel, opened by clicking a `.landscape` row and after Create → Landscape; it writes the file
   at once, heights kept and stretched, and is no undo step.
7. **The game parses no text (0236).** Play and Ship cook every `.landscape` under `Assets/` as saved into
   the game tree's `landscapes.c`, whole millimetres, and the game loads that table into the store before
   the scene's models. So Play shows the hill as last saved.

## Reasoning
Wearing it as a model reuses the draw, shadows, fade, save, undo, cook and drop as pictures did (0298); a
component of its own would repeat each. One text file with no picture beside it keeps 0378's following
whole and the sponsor's files diffable; 2–3 MB at 512 cells is read in milliseconds. A static re-upload
waits for the card to go idle, so a drag that did it each frame would stutter; transients do not wait.
A whole-scene undo state cannot hold a megabyte of heights, so the stroke rides beside the text.
- Heights as a 16-bit picture beside it: needs a 16-bit codec and a sibling that renames apart.
- A landscape component and system of its own: every draw, shadow and pick path learns a second kind.
- Displacing a flat grid in a shader: a new pipeline in `render` and its shadow twin, for no gain yet.
- Cooking unsaved heights into Play: needs the store in every game-tree call; prefabs cook as saved too.

## Replaces
nothing. Carries out 0376 and 0377 point 1 for the landscape.
