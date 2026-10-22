# 0453 — The Assets folder is the engine's, and Import textures picks a largest size
date: 2026-10-20
by: tech-lead

## Decision
A project's `Assets/` holds imported copies, never an author's source files. The `.blend` files, full-size
pictures and anything else an author edits live outside the project. The editor may rewrite, resize or
replace anything in `Assets/` without asking. Import textures asks for a largest size (512, 1024, 2048,
4096, default 2048) and writes every picture it makes no larger than that on its longest side. It keeps the
aspect ratio and never enlarges a picture. This covers colour, normal, emission and the merged ORM picture.
Running Import textures again with a different size replaces the pictures, up to the size the `.glb`
carries. Anything bigger than the `.glb` carries is re-exported from the source.

## Reasoning
The sponsor's words: "No one should ever store their source assets in the game project. We should be able to
do what we want with what's in the assets folder." Exported pictures arrive at whatever size they were baked
at, often 4K, and the sponsor does not want 4K everywhere. Large pictures also cost load time and video
memory. Alternatives:
- A per-texture max size applied when cooking, with the PNG kept full size. Rejected: it keeps a full-size
  copy that the editor treats as untouchable, which this decision says `Assets/` is not.
- Leaving pictures at their exported size. Rejected: that is the 4K problem.

## Replaces
Amends 0403 and 0420: Import textures writes every picture at the chosen largest size, not at the size the
`.glb` holds. Amends 0270: an imported file in `Assets/` is the engine's copy, not the author's original.
