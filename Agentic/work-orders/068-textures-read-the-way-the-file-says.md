# 068 — Textures are read the way the file says

## What
A model's textures are drawn the way its file sets them up. Each texture wraps, mirrors or clamps as the
file asks, and is smooth or blocky up close as the file's filter asks. A texture whose file says nothing
keeps today's hard texels up close (0359, 0373). Textures of any size work, not only powers of two. A
material can take each texture from a different set of texture coordinates. A texture can be offset,
turned and scaled by the file (texture transform). Colours stored per vertex tint the surface. Colour
textures are read as colours and data textures (normals, roughness and metal) as data, so neither comes out
washed out or too dark.

## Why
Every later comparison depends on the textures landing where the file puts them, read the way it means.

## Models
From https://github.com/KhronosGroup/glTF-Sample-Assets/tree/main/Models: BoxTextured, BoxTexturedNonPowerOfTwo, SimpleTexture, TextureCoordinateTest, TextureSettingsTest, TextureLinearInterpolationTest, TextureEncodingTest, MultiUVTest, VertexColorTest, BoxVertexColors, TextureTransformTest.

## How to test
1. Place BoxTextured and BoxTexturedNonPowerOfTwo. Both show the glTF logo the right way round, as in
   their screenshots.
2. Place TextureCoordinateTest. Every square's label matches the corner it sits in.
3. Place TextureSettingsTest. Every tile repeats, mirrors or clamps as its label says.
4. Place TextureLinearInterpolationTest and fly close. The smooth and the blocky rows look as their labels
   say.
5. Place TextureEncodingTest. Each pair of squares matches in brightness, as in its screenshot.
6. Place MultiUVTest, VertexColorTest and BoxVertexColors. They match their screenshots.
7. Place TextureTransformTest. Every square shows a correct-looking arrow or check mark, none a cross.
8. Open each of these in the Khronos glTF Sample Viewer (https://github.khronos.org/glTF-Sample-Viewer-Release/) The textures sit and repeat the same way,
   apart from the lighting, which comes in 069.
9. Press Play: the same in the game window.
