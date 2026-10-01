# 03 — A surface reads its normal and emission maps
folder: render
decisions: 0168, 0278, 0273

## Change
Shader and comments only; no record layout changes (0278 points 3 and 4).

- `render/shaders/draw.slang`: in the lit path, when `normal_texture` is not
  `VOE_RENDER_NO_TEXTURE`, build a tangent frame per pixel from `ddx`/`ddy` of the world
  position and the UV (tangent along +u, bitangent along −v, both orthogonalised against the
  interpolated normal, handedness from the UV derivatives' sign), sample the normal map, map each
  channel 0..1 to −1..1 and take the shading normal through that frame. After the sun and the
  fill, add `emissive × emissive texture` unshadowed. Unlit and the shadow pipeline are
  unchanged. Rewrite the header paragraphs that say the normal and emissive maps are not read.
- `render/include/render/device.h`: the shading record's texture-id paragraph ("the normal and
  the emissive ones are stored and not read") says both are read, and how, in a phrase each.
- `render/shaders/shaders.md`, `render/include/render/render.md`: only where they say the same.
- New `render/tests/surface_maps.c`, headless, skipping without a card, drawing one quad facing
  +Z lit by a sun from +Z, read back offscreen (copy the setup of `render/tests/unshaded.c`):
  a flat normal map (128, 128, 255) reads the same as no map within the file's tolerance; a map
  tilted towards +u reads darker than flat; a black base colour with emissive (1, 0, 0) and no
  emissive texture reads red, and with the sun pointing away still reads red. List it in
  `render/tests/tests.md`.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_render $(ninja -C build/debug -t
targets all | grep -oE "^voe_test_render_[A-Za-z0-9_]+") && ctest --test-dir build/debug -R
"^render/"` exits 0, `voe_test_render_surface_maps` among them.
