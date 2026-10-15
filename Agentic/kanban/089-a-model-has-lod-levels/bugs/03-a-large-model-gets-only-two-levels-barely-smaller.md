# A large model gets only two levels, barely smaller

It says there are only two levels? Going from 250k triangles to 207k. That's not very good. This is a large
model, it's a stable, so it's meters in all directions.

I think I need to be able to choose how many levels and the intensity for them when importing.

Find out first why the levels stop at 207k: a stable is many separate pieces, with seams between parts,
materials and UV islands. If the simplifier won't go further, a count alone won't help. The intensity has to
let it go further.
