# 0401 — An ORM map alone gives roughness and metal
date: 2026-10-11
by: tech-lead

## Decision
In the standard material, a filled ORM slot is the whole truth for roughness and metal: the surface's
roughness is the map's green and its metal the map's blue, exactly as stored, and the Roughness and Metal
values are not read at all. They are not multiplied, offset or blended in. They are kept in the
`.material` file so that emptying the slot brings them back. In the Inspector the two sliders are greyed
out and cannot be dragged while an ORM map is assigned, so nothing on screen seems to do something it
does not. An empty slot reads roughness and metal from the sliders as before. The same rule holds for a
glTF model: where its material has a metallic-roughness texture, the texture's green and blue are the
values and `roughnessFactor` and `metallicFactor` are ignored; with no texture the factors are the values.
Changing a surface's look beyond what its maps carry, such as a wet sheen in rain, is a custom shader's job
once the shader editor exists (0189), not a knob on the standard material.

## Reasoning
- The human's call: a map painted for a surface should show as painted. Under glTF's multiply, the
  default Roughness of 0.5 halved every mapped roughness, so the dirt ground looked wet.
- glTF's factor × texture: only ever makes a surface shinier and squeezes the map's contrast; it suits a
  transfer format, not a control.
- A min/max remap per channel: a real control, but a knob nobody asked for; a custom shader covers it.
- glTF models under the same rule: one standard material reads one way. A glTF file with a texture and a
  factor below 1 will look rougher than in a glTF viewer; exporters write 1 there, so this is rare.

## Replaces
Amends 0400: "Green times Roughness and blue times Metal give the surface's roughness and metal" becomes
"green and blue alone". The rest of 0400 stands.
