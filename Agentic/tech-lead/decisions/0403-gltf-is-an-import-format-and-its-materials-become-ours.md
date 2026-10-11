# 0403 — glTF is an import format, and its materials become ours
date: 2026-10-11
by: tech-lead

## Decision
A `.glb` is how models and animations come into a project; the look of a model is the engine's own
`.material` files. Importing a `.glb` into Assets writes one `.material` per glTF material, beside it,
filled with that material's plain values: colour, roughness, metal, the `lit` shader, or `unlit` for glTF's
unlit extension, and no maps. Each part of the model names its material by that path. Right-clicking a
`.glb` in Assets and choosing **Import textures** writes its textures as PNG files beside it: the colour,
normal and ORM pictures are put into the slots of the materials the import made (occlusion merged into the
ORM picture's red when the glTF keeps it in a texture of its own), and emission and every other map are
written as PNGs and attached to nothing. Importing the `.glb` again rewrites its materials from the file,
map paths included, and touches no PNG; textures are written only by Import textures, and maps lost to a
re-import are assigned again by hand. The generated materials are ordinary `.material` files, opened and
edited in the Inspector like any other. No shader is ever generated from a glTF material: the engine's
built-in shaders are the only shaders, and a look they cannot give is a new built-in shader added once, or
a custom shader once the shader editor exists (0189).

Still open, decided separately: where the part-to-material choice is stored (in a file of the model's,
shared by every placed copy, which would replace the per-entity Material 1–8 rows of 0399, or per entity
as now); whether cut-out, see-through, double-sided and emission become material fields; animations.

## Reasoning
- The human's call: glTF stays the way art arrives, but glTF's material model must not run the engine.
  A glTF material and the standard material are the same shape (base colour, metal-roughness, normal, ORM
  packed as glTF packs it, 0400), so a material written once at import covers it.
- A shader generated per glTF material, deduplicated by content: rejected; looks differ by values, not by
  shaders, and generated shaders grow into many variants, slow starts and a duplicate hunt.
- No materials at import, made by hand only: rejected; a model would arrive with no look at all.
- Textures written at import: rejected in favour of a separate step; often a person paints or reuses
  their own maps and the file's are not wanted.
- Re-import keeping edited materials, or keeping map paths whose PNGs exist: rejected; the human prefers
  the rule "the material comes from the `.glb`", and reassigning maps by hand is cheap.

## Replaces
Nothing. Bears on work order 088 and on 0399's model `materials` paths, both still standing until the open
questions are decided.
