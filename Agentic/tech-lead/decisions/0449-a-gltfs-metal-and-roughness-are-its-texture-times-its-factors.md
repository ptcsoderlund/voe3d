# 0449 — A glTF's metal and roughness are its texture times its factors
date: 2026-10-21
by: tech-lead

## Decision
For 090 bug 03, amending 0401's glTF sentence. Where a glTF material has a metallic-roughness texture,
its roughness is the texture's green times `roughnessFactor` and its metal the texture's blue times
`metallicFactor`, as the glTF specification defines them; with no texture the factors alone are the
values, as before. This holds everywhere a glTF material is read: the ORM picture Import textures writes
carries the products, so a `.material` pointing at it needs nothing more, and running Import textures
again writes that picture again. 0401's rule for the standard material is unchanged: a filled ORM slot is
the whole truth, and the Roughness and Metal values in the `.material` are not read.

## Reasoning
- Blender and every engine that reads glTF multiply. Blender writes a roughness-only map as the
  metallic-roughness texture with `metallicFactor: 0`, so ignoring the factor made `tree asset.glb`'s bark
  fully metal: the map's blue is its grey, near 255.
- Rejected: treating only a factor of 0 as "not metal" (any other factor, such as 0.5, would still be read
  wrong); multiplying in the shader at draw time (the `.material` keeps its own values under 0401, and the
  ORM picture is the one place a glTF's look is carried into ours).

## Replaces
Nothing. Amends 0401: the sentence "`roughnessFactor` and `metallicFactor` are ignored" no longer holds for
glTF; the standard material's ORM rule stands.
