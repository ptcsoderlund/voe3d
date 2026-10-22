# 0443 — An import error speaks to the artist
date: 2026-10-18
by: tech-lead

## Decision
Any message the editor shows when a file brought into a project (a `.glb`, a picture, a sound) can't be
imported, loaded or have its LODs made speaks to the artist who made the file. It names the file, and the
part, material or picture when there is one. It says what is wrong in the artist's terms, with the
numbers (the limits below only show the form; they are not real limits): "the part `bark` has 5,368,447 vertices, more than the 4,194,304 the engine takes", "the picture
`twig_diff` is 16384 pixels wide, the most is 8192", "the part `needles` has no UV map". Where it can, it
says what to do in Blender ("reduce it with Decimate"). A limit the engine keeps on purpose is reported
as that limit, never as broken data. One cause gives one line, not the same line once for every
pass that meets it. Wording about the format itself ("an accessor count out of range", "the data is not
what it claims to be") may go to the log for developers, never as the only thing the artist sees.

## Reasoning
The human, importing a 4-million-triangle fir tree, got "a count or an index that is not a whole number
inside its range" five times and could not tell what to change. Whoever imports a file can fix it in
Blender if they are told what is wrong, and can't if they aren't.
Alternatives: keep the format's wording (correct but useless to an artist); a generic "could not import"
with a details button (hides the one fact the artist needs).

## Replaces
nothing
