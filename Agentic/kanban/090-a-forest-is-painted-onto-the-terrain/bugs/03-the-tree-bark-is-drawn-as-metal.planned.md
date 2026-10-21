# 03 — The tree bark is drawn as metal

## Seen
"The fill color is aggressive on the trees but not on their shadows." The trunks of `tree asset.glb` look
pink and shiny, not like wood, under a light with a pink fill.

Found: every texel of `tree asset-bark-orm.png`, written by Import textures, has blue 255, so the bark is drawn
fully metal (0401: a filled ORM slot gives metal from blue). The glTF's `bark` material has
`metallicFactor: 0` and a `metallicRoughnessTexture` that is `tree_bark_03_rough_4k.png`, a grey roughness
map whose blue is its grey. `assets/src/model_material.c` copies the packed picture's green and blue as they
are and never multiplies them by `roughnessFactor` and `metallicFactor`. glTF defines metal as the texture's
blue times `metallicFactor`, and roughness as its green times `roughnessFactor`. 0401 says glTF models ignore
their factors when the texture is there, which is what lets this happen.

## Expected
Decision 0449. An imported glTF part is as metal and as rough as glTF defines: its metallic-roughness
texture's blue times `metallicFactor`, green times `roughnessFactor`, written into the ORM picture by Import
textures, which writes that picture again when run again. The bark, `metallicFactor: 0`, is not metal.

## How to reproduce
1. Open the lumberhill project; `Assets/Lanscapestuff/tree asset.glb` is imported with Import textures.
2. Look at `tree asset-bark-orm.png`: blue is 255 everywhere.
3. Look at the trees painted on `Hill.landscape`: the trunks shine like metal and take the light's fill
   colour.

## How to test
1. Run Import textures again on `tree asset.glb`: `tree asset-bark-orm.png` has blue 0 everywhere and its
   green is the roughness map's.
2. The trunks look like matte wood in sun and in shade, with no metal shine.
3. A glTF whose metallic-roughness texture has blue 255 and `metallicFactor: 1` is still fully metal.
