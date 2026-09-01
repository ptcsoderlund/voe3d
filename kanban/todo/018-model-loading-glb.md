# 018 — model loading: glTF binary

status: todo
claimed-by: -
blocked-by: 017

Written by us (ADR-0023). **Import only — glTF is never a scene format**
(ADR-0010).

## Goal

A `.glb` file on screen, with its own geometry and its own textures.

## Scope

- **`.glb` only.** Binary container, no `.gltf` text form. This is a real
  simplification and the reason to take it: no external file resolution, no
  base64 data URIs, no relative-path guessing. Textures come from chunks inside the
  file, decoded by card 017's readers.
- **Geometry**: positions, normals, texture coordinates, indices.
- **Materials: parse them**, per the principal. Base colour, metalness, roughness,
  occlusion, emission, and their texture references. **Stored, not applied** —
  applying them is card 019.
- **No animations.** The principal's decision. Skins, joints and keyframes are
  later, not never.

## Two things that are already decided and must not be re-decided

- **No coordinate conversion, ever.** The engine is right-handed, +Y up, −Z
  forward — *identical to glTF* — precisely so the importer does nothing here.
  ADR-0033 does not merely permit this, it forbids adding one.
- **Matrix layout is transposed, coordinates are not.** ADR-0035, exactly. Getting
  these two confused is the bug that makes a model look mirrored *and* correct
  depending on what you compare it against.

## On material channel packing

The principal's direction is baked channels — **ORM**, and possibly **ORME** with
emission in alpha. Useful fact: **glTF already packs ORM that way** — occlusion in
R, roughness in G, metalness in B of a shared texture is the standard arrangement,
so this is not a divergence. **Emission is where it differs:** glTF's emission is a
full RGB texture, not a single strength channel, so ORME is a real deviation and a
decision that has not been taken. Parse what glTF gives you; do not invent the
packing here.

## Verify

- A known model, correct orientation, correct handedness, textures on the right
  faces. **A mirrored model is the failure to look for**, and it is invisible on
  anything symmetrical — use an asymmetrical test model.
- Malformed chunk headers, a truncated file and unsupported features are all
  recoverable failures with tests. Unsupported must say *what* it did not support.
- `check.cmake` zero. Windows is the principal's.
