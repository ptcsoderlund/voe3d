# 0263 — engine_assets is the human's folder, and the README opens with the name and the logo
date: 2026-09-26
by: tech-lead

## Decision
Root `engine_assets/` holds the engine's own branding and the human's input material: the logo
and app images in `engine_assets/Engine images/`, reference files and examples beside them. It is
the human's. Agents may read it and link to it, and never add, change, rename, move or delete
anything in it. It is not a code folder: the folder checks skip it and the build does not compile
it. When a feature needs one of its files inside the product, the card copies that file into the
code folder that uses it and leaves the original where it is. The engine's full name is
**Voluntary Overtime Engine 3D**; the short name in text is **voe3d**, written lowercase. The logo
image keeps its own capitals. `README.md` starts with that full name as its only `#` heading, and
the next thing on the page is `engine_assets/Engine images/Primary.png`, linked in place.

## Reasoning
The human wants the name and the logo to be the first thing anyone sees, and wants one place for
material they supply that the agents do not rearrange. Alternatives: pointing the README at
`dev/src/logo.png` (the same picture, but it belongs to the dev program); a light/dark wordmark
pair (it drops the yellow bar and "Voluntary Overtime"); letting agents write to `engine_assets/`
too (that loses a folder holding only what the human put there).

## Replaces
nothing
