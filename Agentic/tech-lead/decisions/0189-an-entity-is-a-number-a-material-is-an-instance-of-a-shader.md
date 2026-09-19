# 0189 — An entity is a number, and a material is an instance of a shader
date: 2026-09-19
by: tech-lead

## Decision
This is how the product names things, in the editor, in work orders and in code.
- **An entity is only a number.** It has no place of its own. Where a thing is comes from its
  transform component, and everything else it is comes from its other components. "Add a cube"
  means a new entity with the components a cube needs, and its transform starts at the centre of
  the scene, the world origin. Dropping things at a chosen place comes later, with drag and drop.
- **A shader** is the program that says how a surface is drawn. **A material is an instance of a
  shader**: the shader's inputs filled in with values. A shader editor and a material editor are
  two separate editors. A visual (node-graph) shader editor is the long-term aim.
- **Until those two editors exist, a built-in shape has only a colour**, chosen in the
  Inspector. No material or shader is shown or chosen anywhere in the editor before then.

## Reasoning
The sponsor's vocabulary, fixed now so the Inspector, the files and the future editors do not
each invent their own. A colour alone is enough for the coin game (0186), and it keeps the
material and shader work from arriving before it is needed. Rejected: a material editor now,
which has no shader editor to be an instance of; a new entity placed in front of the camera,
which gives an entity a place it does not have.

## Replaces
nothing
