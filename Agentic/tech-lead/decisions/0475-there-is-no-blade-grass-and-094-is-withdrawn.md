# 0475 — There is no blade grass, and 094 is withdrawn
date: 2026-10-23
by: tech-lead

## Decision
The engine has no grass blades: a foliage kind's look is always a model. Grass on the hill, if the sponsor
wants it, is a model kind (a tuft from Blender or downloaded) that the grass material grows as 0473 says,
so 0376's "grass grows from the painted grass layer" holds through 095. 094 (grass grows on the grassy
parts) is withdrawn untested: its board is removed, its branch is discarded and none of its grass code is
merged. Its card 09 (the model loader reads what `load_from` says) fixed a suite finding outside grass and
is kept: the sponsor carries that commit onto `dev`, and if it does not apply there it comes back as a
`/bugreport`. 0472 is replaced. 0473 is amended: point 1 reads "a kind's look is a model", point 2 loses
"blades may now cast" and "never for blades", so every kind may have Blend with ground (0467's "never
grass" no longer applies), and point 4 loses "blades too". 0474 is amended: 095 still comes next, built on
090's foliage instead of rewriting 094.

## Reasoning
The sponsor's call (2026-10-23): "no blade grass at all". One kind of look keeps foliage one thing to
learn and build, which is what 0473 is after.
- Accept 094 as the blade machinery and build 095 on it: the tech-lead's pick, turned down because the
  sponsor does not want blades in the engine.
- Drop 094 and fold blades into 095: rebuilds nearly the same code and doubles 095.

## Replaces
0472. Amends 0473 points 1, 2 and 4, 0474 and 0467.
