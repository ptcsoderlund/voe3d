# 019 — one directional light, with material shading

status: review
claimed-by: claude-code (kanban-coder)
blocked-by: 018

The card that makes it stop looking like a programming exercise.

## Goal

A model lit by one sun, with its material actually affecting how it looks.

## Scope

- **One directional light.** Direction, colour, intensity. No point lights, no
  spotlights — *many lights* is on the *later* list.
- **No shadows.** The principal's decision, and shadows are their own large card.
- **Material shading using what card 018 already parsed**: base colour, metalness,
  roughness, occlusion. Metalness–roughness, matching glTF, because that is what
  the importer produces and inventing a second model would mean converting.
- Normals from the model. **Normal matrices are not model matrices** — non-uniform
  scale is where that becomes visible, and it looks like broken lighting rather
  than broken maths.

## Where this card is likely to go wrong

- **Colour space.** Base colour textures are sRGB; normal, occlusion, roughness and
  metalness maps are **not**. Getting this wrong makes everything slightly too dark
  or too flat and it is almost impossible to spot without a reference. State the
  choice per texture kind explicitly.
- **Nothing here is tone mapped yet**, so bright values clip. That is expected, it
  is on the *later* list, and card 013's offscreen target is what will make it
  possible. Do not sneak it in.

## Verify

- Rotate the light and the shading follows. Rotate the model and the shading stays
  attached to the surface, not to the screen.
- A rough material and a smooth one look different. A metal and a non-metal look
  different.
- A non-uniformly scaled model is still lit correctly — the normal matrix test.
- `check.cmake` zero. Windows is the principal's.

## Notes (coder, 2026-09-04)

Done. Everything the card asks for is in, and what could not be **run** on the
machine in front of the coder is listed under *Verified* below — read that first,
because it includes the shader.

### What was made

- **`scene` — the `light` module**, four files like the two beside it.
  `voe_scene_light` is a direction, a colour and an intensity;
  `voe_scene_light_system` owns the one intent that turns it and is the only
  place a direction is normalized, so every reader downstream gets a unit
  vector without asking. `scene/tests/light.c` is that claim.
- **`render`** — `voe_render_light` beside `voe_render_view`, handed to
  `voe_render_frame_begin` once a frame and written into the same per-slot
  uniform buffer as the camera (one block, `struct voe_render_frame_block`,
  internal). `voe_render_view` gained the eye position; `voe_render_object`
  gained a normal matrix. `voe_render_texture_create` gained a
  `voe_render_texture_kind`.
- **`render/shaders/draw.slang`** — the glTF metalness-roughness BRDF written
  out: GGX distribution, Smith height-correlated visibility with the specular
  denominator folded in, Schlick Fresnel, Lambert diffuse scaled by what the
  Fresnel term did not reflect. Base colour, metalness, roughness and occlusion,
  factors and maps both. Every decision in it is a paragraph in its header.
- **`3d`** — `normal_matrix`, a module of its own so that the card's normal
  matrix claim has somewhere to be tested; the draw system reads the world's one
  light and computes one inverse per drawn object per frame.
- **`3d/src/import.c`** — pictures are uploaded per colour space now, which is
  the one structural consequence of getting the colour spaces right.
- **`dev`** — one sun, circling the scene once every twenty seconds as a light
  intent, and the turning cube is squashed (1.4, 0.6, 1.0) so that the normal
  matrix is something a person can see rather than only a test.

### Colour space, stated per texture kind as the card asked

- **Base colour and emissive pictures go up as `VOE_RENDER_TEXTURE_COLOUR`** —
  `R8G8B8A8_SRGB`, so the hardware decodes them to linear on every read.
- **Metallic-roughness, occlusion and normal pictures go up as
  `VOE_RENDER_TEXTURE_DATA`** — `R8G8B8A8_UNORM`, read exactly as written.
  Decoding one of these would turn a roughness byte of 128 into 0.21 and make
  every surface in the scene shiny.
- **Which kind a picture is comes out of the material slot that referenced it**,
  because nothing in a PNG says. So `upload_images` now walks the materials
  before it uploads anything, and a picture referenced both ways round is
  uploaded twice — `voe_3d_import.texture_count` is uploads and not pictures,
  and `dev` prints "textures" now.
- **The other half: the swapchain and the target are `B8G8R8A8_SRGB`**, which is
  what encodes the linear frame on the way to the window. `render/src/texture.c`
  said card 019 had to do both or neither, and it does both. Consequences worth
  knowing: the clear colour in `frame.c` is now a *linear* colour (the same slate
  blue as before, in linear form), and `render/tests/matrix.c` expects the sRGB
  encoding of its three matrix elements rather than 255 times them.
- A surface that offers no sRGB format at all takes what it can get and says on
  stderr that the frame will be too dark. Not refused: it is a machine nobody
  here has seen and a wrong picture beats no program.

### Decisions the card left open, and where each is written down

- **Occlusion multiplies the direct light, which is not where glTF puts it.** In
  the specification an occlusion map attenuates *indirect* light and this engine
  has none, so reading it that way would mean uploading a channel and throwing it
  away. Applied to the direct result instead, and `draw.slang`'s header says so
  and says which card moves it.
- **A surface with no vertex normal is drawn unlit** — its base colour with no
  light on it — rather than black or NaN. Card 018 deferred this decision
  explicitly. glTF says a client should compute flat face normals for such a
  file; that means splitting shared vertices at import and it is a card of its
  own, not a line here.
- **A world that is drawn needs exactly one light, and none asserts.** Same
  treatment as the camera. A world with no sun draws black, and black is the one
  result that reads as a broken renderer rather than a missing line.
- **Emission and normal maps are still stored and not read** (rule 10). Neither
  is in this card's scope list: a normal map needs tangents the importer does not
  produce, and emission without tone mapping is a colour that clips. Emission is
  a one-line addition the day a card asks for it.
- **Nothing is tone mapped and bright values clip**, as the card requires. No
  exposure, no ambient, no image-based light, no shadows.

### Verified

On **Linux (WSL, clang 21)**, which is the machine in front of the coder:

- Every folder's sources and every test compile with the engine's own flag set —
  `-std=c23 -Wall -Wextra -Wpedantic -Werror` — including `dev/src/main.c` and
  the four tests that cannot be linked here.
- **The 19 tests that need no graphics card pass**, `scene/light` and
  `3d/normal_matrix` among them. `3d/normal_matrix` is the card's normal matrix
  claim: a normal stays perpendicular to a non-uniformly scaled surface, the
  world matrix's answer does not, and the three cases where the two agree are
  each checked so that the wrong matrix cannot pass.
- `clang --analyze` clean over every `src/` and `tests/` file in the tree, which
  is step 7's check run by hand.
- Step 5's include-hygiene rule re-checked by hand: no folder includes a folder
  it does not depend on, and only `tests/` includes `testing/`.
- The BRDF's arithmetic was checked on the CPU by porting the fragment shader's
  maths into a scratch program: a white rough dielectric facing a white light of
  π comes back at 0.97 and not above 1, a grazing light contributes nothing,
  roughness is monotonic, and a smooth surface's aligned highlight is the large
  number a directional light's delta lobe should be. That is not the shader
  running, but it is the formulas being wrong caught before the principal looks.

**NOT VERIFIED HERE, AND THIS IS THE LIST TO READ BEFORE MOVING THE CARD ON:**

- `cmake -P check.cmake` **fails at step 1 on this machine** with
  `slangc could not be run` — there is no `slangc`, no `ninja` and no Wayland
  development package on it. Nothing about the code; the step never got as far as
  the tree.
- **`draw.slang` has not been compiled by anything.** No `slangc` here, so the
  whole of the shading exists only as source. Its layout is asserted against the
  C structs at compile time (`descriptors.c` checks five sizes and five offsets),
  which is the half that can be checked without slangc, and the half that cannot
  is whether Slang accepts the file at all.
- **Nothing was drawn.** The four GPU tests (`render/offscreen`, `render/matrix`,
  `render/pools`, `3d/import`) and the `dev` window all need a device this
  machine cannot open. `render/offscreen` is the one to watch: it now drives its
  own sun and is the automated half of "a lit cube still has its colours in the
  right places".
- So the card's four *Verify* lines — the light rotating, the shading staying on
  the surface, rough against smooth and metal against non-metal, and the squashed
  model — are all a look at the `dev` window on Windows, and that is the
  principal's. `dev` was arranged specifically for them: the sun laps the scene
  on a different period from the camera, one cube is squashed and turning, the
  cubes are smooth-ish plastic (roughness 0.8, metalness 0) and the figure's
  roughness and metalness come out of its ORM map.

### Suggestions, not done

- **Emission is a one-line addition** (`+ shading.emissive * emissive map`) and it
  is the obvious next thing a person will miss on a model that has it. Left out
  because the card's scope list does not name it and because it clips without
  tone mapping.
- **Flat face normals for a file that has none**, at import, per glTF. Needs
  vertices split per face; it is the honest fix for the unlit fallback above.
- **A cached normal matrix** in the transform table if one inverse per object per
  frame ever measures slow. It does not yet and there is no number attached.
