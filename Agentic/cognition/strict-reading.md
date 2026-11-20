# Strict reading

What a strict read of a sectioned file refuses that the lenient read lets pass, and what the agents' check does and does not catch.

## What strict adds
- The lenient scene read is unchanged: an unknown key is only a warning and ranges are not looked at. [observed · coder 115-02 · 2026-11-06] see 0509
- Only scene fields with a 0498 range are refused for being out of range; material and landscape values are refused only where their readers already refused them. [observed · accept 115 · 2026-11-20] see 0509
- In a material only cutout is refused at both ends; emission strength and its channels only below 0; roughness, metal and repeat not at all. [observed · coder 116-11 · 2026-11-20]
- For `project.voe3d` the lenient read already refused an unknown section and a window width or height out of range; strict adds only refusing an unknown `[project]` key. [observed · coder 115-03 · 2026-11-06]
- In a landscape, a key the lenient read stops before counts as a key no section takes: `layer2` or `kind2` after a gap, a look key or paint row of a kind or layer past the count, `[Paint]` rows with no layers. [observed · accept 115 · 2026-11-20]
- A section the format does not have is refused even when empty. [observed · accept 115 · 2026-11-20]
- A `.parts` file is not read strictly: an old `strength` in `[Levels]` is not wrong. [observed · accept 115 · 2026-11-20] see 0437

## Refusals
- A refused unknown section names the section's own line. [observed · accept 115 · 2026-11-20]
- A range refusal shows the value as stored (a float printed short), not as the file spelled it. [observed · accept 115 · 2026-11-20]

## The check
- The agents' check reads `.prefab` text only as scene text, without the prefab reader's one-tree shape check: a prefab can pass the check and still be refused as a prefab. [observed · accept 115 · 2026-11-20]
- "`Code/` with no library built" means `Code/` lists a `.c` file; a `Code/` with no `.c` registers no components, so nothing goes unchecked. [assumed · coder 116-17 · 2026-11-20]
- A built library that will not load is one line with its reason and "the project's own components were not checked"; it is not counted as a refused file. [assumed · coder 116-17 · 2026-11-20]
