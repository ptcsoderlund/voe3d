# 0505 — The editor is made for people and agents alike
date: 2026-11-06
by: tech-lead

## Decision
The editor is a product its users ship games with, and the engine neither forbids nor assumes AI tools: in a
game project an agent may read and write every sectioned-text file (`project.voe3d`, scenes, prefabs,
materials, landscapes, parts) as freely as the user lets it. The boundary is the user's, never ours. Two things
follow and are owed by the product. (1) The editor notices when any sectioned-text file of the open project
changes on disk and takes the change in, so a person in the editor and an agent in the folder work on one
project at once without either erasing the other. (2) The strict meaning of those files — which sections exist,
which keys each takes, their types, ranges and units, so that a light never gets a `texture=` — is written down
where an agent working in a project folder finds it, and stays true to the editor that wrote the project.
0376 and 0251 still hold for how this repository's own agents work: the hill and the coin game's levels are
the sponsor's hand, and agents building VOE3D build tools. They never spoke for the editor's users.

## Reasoning
The sponsor (2026-11-06): "I cannot and should not dictate that users of the editor cant or wont use ai
tools … it is up to the user of the editor to set boundries to ai agents not us", and "I want this when
making games in the future." 0236 already made every project format text so a person or an AI can edit it
without the editor; this finishes that thought. Today Save writes over the scene with no check, and only
`.glb` models and texture maps are watched, so an agent's edit to a light is lost on the next Save.
- Agents edit only at the user's request: a rule the editor cannot enforce and should not try to.
- Leave it to users, document nothing: the keys are strict and live only in engine header comments, so
  agents would guess and write files the editor refuses.

## Replaces
Nothing. Narrows how 0376 and 0251 are read: they bind this repository's workflow, not the product.
