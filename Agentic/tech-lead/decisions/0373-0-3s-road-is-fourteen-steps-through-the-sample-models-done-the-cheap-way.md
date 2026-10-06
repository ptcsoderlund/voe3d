# 0373 — 0.3's road is fourteen steps through the sample models, done the cheap way
date: 2026-10-06
by: tech-lead

## Decision
0.3 draws the Khronos glTF sample models (0372) the way a game engine would, not the way a reference
renderer would. Every one of these rules holds:

1. **The hardware line is 0318's low end.** A glTF feature is in 0.3 only if it runs on a GTX 1060 or
   RX 5700 class card with no ray tracing, no mesh shaders and no other feature those cards lack. It
   also has to cost a few texture reads and some arithmetic per pixel at most. Where the GPU does a
   job in fixed function (compressed texture formats, filtering, mip chains, cube maps, instancing,
   packed vertex formats), the engine uses that rather than shader or CPU work.
2. **Cheap approximations are allowed** wherever the model still looks the same at a glance next to
   the Khronos glTF Sample Viewer (github.khronos.org/glTF-Sample-Viewer-Release) with the same
   environment from github.com/KhronosGroup/glTF-Sample-Environments. The comparison is by eye, never
   pixel by pixel. 0316 holds: whatever costs time every frame is off until a scene or a model asks
   for it, and cheap when on.
3. **In 0.3:** `.gltf` files beside `.glb` (amends 0270), every core glTF feature, and these
   extensions: mesh_quantization, KHR_ and EXT_meshopt_compression, EXT_mesh_gpu_instancing, texture_transform,
   lights_punctual, materials_emissive_strength, materials_unlit, materials_ior, materials_specular,
   materials_clearcoat, materials_sheen, materials_transmission, materials_volume,
   materials_dispersion, materials_diffuse_transmission, materials_iridescence,
   materials_anisotropy, materials_variants and node_visibility. A model also gets lit by an
   environment image, and a scene gets a tone curve.
4. **Not in 0.3:** animation, skins, morph-target animation and animation_pointer (0.4). An animated
   model shows still in its rest pose, and morph targets at their default weights. Draco, KTX2/Basis
   and WebP (every sample that uses them has a plain variant or a PNG/JPEG fallback, except
   SheenWoodLeatherSofa, which is left out),
   pbrSpecularGlossiness (archived by Khronos), and the unratified volume_scatter and retroreflection.
   A file that requires an extension outside 0.3 is refused with its name in the Errors panel. An
   optional one is ignored, and the model draws without it.
5. **The file's own settings win over the engine's defaults for a model.** A texture uses the
   sampler (wrap and filter) the file gives it. Only a texture with no sampler keeps 0359's hard texels
   up close (amends 0359). A mesh with tangents in the file uses them, and one without gets the
   tangents glTF prescribes (amends 0278's derivative frame for models).
6. **Glass sees the scene drawn before it**, the way water does (0305). This is not screen-space
   lighting in 0328's sense: no light is gathered from the screen. Glass behind glass is not seen
   through the nearer glass. That is an accepted limit.
7. **The environment is a scene setting, off in a new scene.** An `.hdr` panorama from Assets lights
   the scene and can be its background. The tone curve is a scene setting too: Khronos PBR Neutral
   in a new scene, none in a scene saved before it existed, so old scenes keep their look.
8. **A model's own lights belong to the model.** They shine where it is placed and cast no shadows,
   by 0316. Spot lights come with them, and the editor gets them as a light type too.

The road is work orders 067–080, in that order. Each names its models by folder under
github.com/KhronosGroup/glTF-Sample-Assets/tree/main/Models. The models are downloaded by the sponsor
and never committed. 0.3 is done when 080 is accepted.

## Reasoning
The sponsor wants hardware-native rendering, not reference-correct rendering: "the performance way, not
the photorealistic correct way". 0318 already draws that line for the engine, so 0.3 reuses it rather
than inventing a second one. The order puts reading files first, then the light every comparison
needs (environment and tone curve), then the core surface, then the compression and instancing that
are the most hardware of all, then extensions from cheapest and most common to rarest.
- Reference-correct rendering (multi-scatter, ray-traced glass, true volumes): what the sponsor ruled out.
- Pixel-matched screenshot tests: cheap approximations would fail them, and Khronos's screenshots
  come from many renderers.
- Animation in 0.3: about 20 models need it, but it is its own road, kept for 0.4 as 0368 had it.

## Replaces
nothing. Amends 0270, 0278, 0359 and 0328 as the points above say.
