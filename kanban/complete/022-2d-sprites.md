# 022 — 2D and sprites

status: completed
claimed-by: claude-opus-5 (kanban-coder)
blocked-by: -

**2026-09-06: this card is claimable. `blocked-by` is empty for the first time.**
The four decisions it was holding are taken — ADR-0080, ADR-0081 and ADR-0082 —
and the previous version's *"Do not claim this yet"* section is gone because
there is nothing left in it. A coder stopped at this card and refused to guess at
spherical-vs-cylindrical billboarding and at a fringe strategy. **That was the
right call and it is why this card is now short.**

**Rewritten by the tech lead under the standing card-writing grant.** Not a
spin-off: it keeps its number.

## Goal

A 2D sprite: a textured plane in the 3D world, drawn with the right alpha mode,
in either layer, picking one frame out of a sheet.

## The decision that made this card small

**The engine does not billboard** (ADR-0080). The principal's words: *"None, that
is up to the developer. [...] We just do planes with 2d sprites in 3d world,
which means Z axis decides overlap."*

So there is **no billboard mode, no facing flag, no look-at helper, and no enum**
in this card. Do not add one. A sprite is a quad with an ordinary transform. If a
program wants it to face the camera it rotates it itself, in game code, from the
camera it already owns — `voe_scene_camera` is an ECS component with `eye`, `yaw`
and `pitch`, and `voe_scene_camera_forward` is public precisely so a caller does
not do its own trigonometry. Spherical facing is a rotation built from that
forward vector; cylindrical is the same thing using yaw and ignoring pitch. Both
belong to `dev`, as demonstrations, not to the engine.

**Overlap is depth**, and depth already works. Opaque and cutout write and test
it; blended tests it, does not write it, and is sorted back-to-front per object
by card 021a's sort. Nothing here changes any of that.

## What is decided, so none of it is a choice

Summarised so this card is implementable without reading the planning root.

- **A sprite sheet is created `VOE_RENDER_SAMPLING_SHARP`** — nearest, no mip
  chain, `CLAMP_TO_EDGE` — and **uploaded as authored, straight, not
  premultiplied** (ADR-0082). Do **not** build an edge-bleed pass and do **not**
  add a premultiplied-storage flag. Fringing is an artifact of *filtering* and
  nothing that is a picture is filtered in this engine; there is no path from a
  sheet's texels to a colour that was not in the sheet. The clamp matters for a
  real reason: a sheet addressed `REPEAT` can fetch across its own border into a
  different sprite.
- **The sheet is `VOE_RENDER_TEXTURE_COLOUR`** and `base_colour_distance_field`
  stays **false**. A sprite sheet is a picture. The distance-field path is the
  glyph sheet's and nothing else's.
- **One draw call per sprite, and that is deliberate** (ADR-0081). No batching, no
  instancing, no indirect draw — they stay on the *later* list and they are one
  decision that is not taken here. **The ceiling is roughly a thousand blended
  objects**, at which point per-object submission costs 1–3 ms of CPU a frame. The
  sort is not the problem and will not be: at the same count it costs 0.02–0.2 ms.
  Write the ceiling in `sprite.md`; do not try to raise it.
- **Both layers are ordinary** (ADR-0074). A character in the world is
  `VOE_3D_LAYER_WORLD`; a crosshair or a health bar is `VOE_3D_LAYER_OVERLAY` and
  stops being covered by walls. **Neither is the default and neither is the odd
  one out** — if this card finds itself treating one as the normal case, that is a
  finding worth reporting. An overlay sprite still has a real position in metres
  and is seen through the same camera; it is not screen space.
- **Unlit is a material property** (ADR-0071) and is what a sprite usually wants.
  No new shader and no new pipeline in this card.
- **The alpha mode is the material's**, named as glTF names it (ADR-0061):
  opaque, cutout with a cutoff, blended. A sprite picks one; it does not invent a
  fourth. Cutout edges crawl and that is an accepted, stated limitation
  (ADR-0078), not a defect to fix here.

## How a sprite picks a frame out of a sheet — the shape, and why

**This is scope, not a decision, for the reason card 024's depth clear was**:
ADR-0060 already licenses `render`'s public surface growing on demand, and the
material component already gained a field this way (`base_colour_distance_field`,
card 025). It is written out here because the card that leaves it implicit is the
card its coder gets caught short by, and that has happened twice already (D-111).

**A UV rect — offset and scale, four floats — on the shading record and on the
material.** The quad's own UVs are baked 0..1 and every sprite in the engine
shares **one unit quad geometry**.

Why this shape and not the two obvious alternatives:

- **Not baked per-sprite UVs in the geometry.** `voe_render_geometry_create` waits
  for the GPU to go idle and appends to a pool with no destroy, so geometry is
  built once and cannot change (D-092). A sprite whose UVs are in its mesh can
  never change frame, which is most of what a sprite sheet is for.
- **Not a texture array.** It forces every frame in a sheet to be the same size,
  and an atlas is what artists actually ship.
- **The UV rect costs nothing per frame, because the mechanism already exists.**
  An object's record carries a world matrix and a **shading index, submitted every
  frame** — `render/device.h` puts it plainly: *"the geometry stays where it is
  and the shading slides across it."* So a sheet of N frames is N shading records
  over one geometry and one texture, and changing frame is pointing an object at a
  different record. Nothing is re-uploaded and nothing stalls.

Note the pool: `voe_render_limits.shadings` defaults to 64 in `dev`. A sheet with
more frames than that needs the program to ask for more, which is the program's
call. Say so in `sprite.md` rather than raising the default.

## Scope

- **`render`** — the UV rect in `voe_render_shading_values` and in the shading
  record the shader reads, and the base colour's UV transformed by it in the
  fragment stage. Four floats, one multiply-add. Nothing else.
- **`3d`** — the same four floats on `voe_3d_material`, passed through
  `voe_3d_material_upload`. Default is offset (0,0) scale (1,1), so **every
  existing material and every existing call site is unchanged** and the cube, the
  model and the glyph sheet all keep behaving exactly as they do.
- **`sprite`** — the module planned beside `3d` in the module map and not yet
  built. A unit quad, and helpers to build a sheet's shading records from a grid
  (columns, rows, frame count). It depends on `render` and `3d` and on nothing
  sideways or upward. **Keep it thin**: if it starts wanting a component and a
  system of its own, stop and report rather than growing one.
- **`dev`** — a scene showing, at minimum: a sprite in the world that a cube can
  pass in front of; a sprite in the overlay that the same cube cannot cover; one
  frame picked out of a sheet; and **both facing rotations written in game code**,
  side by side, so the difference between them is visible and it is visible that
  the engine did not do it.

## Not in scope, and each has a reason

- **Billboarding of any kind in engine code** — ADR-0080. It is `dev`'s, as a
  demonstration.
- **Batching, instancing, the indirect draw** — ADR-0081. One decision, taken on a
  card with real content behind it.
- **Sprite animation over time** — no decision exists and none is needed yet. The
  UV rect and per-frame shading index are what an animation card will use, which
  is the point of choosing them, but nothing here advances a frame.
- **Any orthographic projection.** None exists in the engine and this card does
  not add one. The principal has raised whether projection becomes the program's
  choice; it is D-113 and it is **not** a sprite question.
- **A fixed pixel-to-world scale.** Still a known unknown, and now a smaller one:
  nothing in this engine is natively a number of pixels, and if projection ever
  becomes the program's choice the developer arranges this themselves.
- **A second sampler, a filter, or a mip chain for the sheet** — ADR-0078,
  ADR-0079. If the sheet looks aliased at distance, that is the engine working as
  decided.

## Where this card is likely to go wrong

- **Adding a billboard mode because it feels like the sprite module's job.** It is
  the single thing this card most needs not to do, and it will feel like an
  omission. It is not.
- **Growing the UV rect into a full texture transform** — rotation, shear, a
  second UV set. Four floats. If something genuinely needs more, report it.
- **Putting the rect on the object record instead of the shading record.** The
  object record is per-object per-frame and is the hot path; the shading record is
  where per-material data lives and is already indexed per frame for free.
- **Treating the overlay as the sprite layer.** Both placements are ordinary, and
  a `dev` scene that only shows one has not shown the decision.
- **Defaulting the rect to something other than the whole texture.** Every
  existing material must be untouched by this card.

## Verify

- The cube, the glTF model and the text all render exactly as before — a
  screenshot comparison, not an assertion that nothing was touched.
- A sprite in the world is occluded by geometry in front of it, and the same
  sprite in the overlay is not.
- Two sprites in the overlay still occlude each other correctly.
- A blended sprite behind another blended sprite composites in the right order as
  the camera orbits past the point where their depth order swaps.
- One sheet, several frames, several sprites: each shows a different frame, from
  one geometry and one texture.
- A cutout sprite's edge is hard, and crawls when the camera moves. **That is the
  expected result, not a bug** — record it as seen.
- Both facing rotations in `dev`: the cylindrical one stays upright as the camera
  rises, the spherical one leans back to face it.
- Tests where they are cheap and mean something: the UV rect maths, and the
  default rect being identity.

## Report, do not decide

If this card cannot be implemented as written, say so instead of guessing — the
last two coders to hit an underspecified card here were right to stop, and both
times the card was at fault rather than the coder.

Worth reporting whichever way it goes:

- **Whether `sprite` earned being its own module**, or whether what got built is
  thin enough that it should have been part of `3d`. An honest answer either way.
- **Whether one unit quad for every sprite held up**, or whether something wanted
  its own geometry.
- **What the per-object draw cost actually measured at**, if the `dev` scene gets
  to a few hundred sprites. ADR-0081 argues 1–3 ms per thousand *in advance*, and
  a real number against that prediction is worth more than the prediction.
- **Anything that made the no-billboarding decision awkward in practice.** It is a
  cheap decision to reverse and the first honest report of friction is what would
  reverse it.

---

## Notes — what was built and what was verified

Verified on **Linux (Fedora, KWin/Wayland, NVIDIA RTX 4070 Laptop, Clang 22,
CMake 4.3.0)**. `cmake -P check.cmake` exits **zero**: all 21 steps ok, 33 tests
passed, analyser clean over 91 files. Nothing was checked on Windows.

### `render` — the rectangle in the record

`voe_math_float4 base_colour_uv_rect` at the end of `voe_render_shading_values`,
xy the offset and zw the scale. It lands at offset 80, which is already a
sixteen-byte boundary, so nothing in front of it moved and no padding was added;
the record grew from 80 bytes to 96. `render/src/descriptors.c` has the new size
and an offset assert for the rect beside the existing ones, and `draw.slang`'s
struct matches. The fragment stage reads the base colour at
`uv * rect.zw + rect.xy` and every other map at the vertex's own coordinates, as
the card's scope says. One multiply-add, no new branch — the shader still has
exactly three.

### `3d` — the same four floats on the material

`base_colour_uv_offset` and `base_colour_uv_scale`, two `voe_math_float2` rather
than one `float4`, because a material is the thing a person writes by hand and
two named fields cannot be packed the wrong way round. `voe_3d_material_upload`
packs them into the record.

**The default needed a decision the card did not spell out, and it is worth
knowing which way it went.** Every material in this engine is built by naming the
fields it cares about and letting C zero the rest, so the rect arrives zeroed —
and a scale of nothing is not a harmless zero, it reads one texel across the
whole surface. `voe_3d_material_upload` therefore reads a zero scale component as
one, component by component, and **writes the answer back into the material** so
that the component and the record it just made cannot disagree. That write-back
is also what makes the rule testable: `3d/tests/material.c` uploads a silent
material, a fully framed one and a half-framed one and checks what came back.

### `sprite` — new folder

`voe_sprite_quad_create` (the one unit quad) and `voe_sprite_sheet` /
`voe_sprite_sheet_frame` / `voe_sprite_sheet_upload` (a grid of cells, one
material per frame). `cmake/voe.cmake` has its row, the root `CMakeLists.txt`
adds it, and it configures standalone. Its dependency row is `3d render math
base`, as the card specified — see the report below, because that departs from
the module map.

**The quad is eight vertices and not four.** The engine culls back faces and a
sprite is a flat thing in a world a camera walks around, so it is two squares in
the same place wound opposite ways, the way `dev/src/quad.c` already is and for
the reason its header gives. The back face shows the picture mirrored, like a
cardboard cutout. Exactly one face survives culling from any side, so a
see-through sprite never blends over itself.

### `dev` — the exhibit

`dev/src/sprites.h` and `dev/src/sprites.c`: a sheet built in code (four cells
across, two down, thirty-two texels each), one quad, nine records and twelve
entities. `main.c` gained one call at startup and one per frame, in the same gap
between the camera and transform systems the heads-up line uses.

The sheet's cells are discs that grow and change colour, each with an opaque
white square in its top-left corner — the square is what makes a wrong rectangle
obvious. **The last cell is a triangle, not a disc, and that was a correction
made after looking at the first build:** a disc is the same shape from every
angle, so the two sprites whose whole purpose is that one of them turns
differently were two circles that stayed circles. The triangle has an
unmistakable up.

**The disc rims ramp alpha over two texels and the texels outside are empty** —
no edge is bled outward. That is ADR-0082's claim made visible, and it holds: at
about three and a half screen pixels per texel the rim comes out as discrete
texel-sized steps and there is **no dark halo** anywhere. If a halo ever appears
there, something started filtering.

### What was looked at, and what a person still has to look at

Screenshots were taken around the orbit on the machine above.

- **The cube, the models, the text and the see-through quads render as before.**
  Compared against shots of the same scene: unchanged.
- **One sheet, several frames, several sprites** — the row of four is four
  entities off one geometry, one texture and four rectangles, and they are four
  different discs at four different sizes. ✔
- **A world sprite occluded by geometry in front of it** — the disc behind the
  still cube is cut off exactly at the cube's edge from one side of the lap and
  comes out whole from the other. ✔
- **The same cube cannot cover the overlay ones** — the two overlay sprites stand
  inside that cube and are drawn over its solid face all the way round. ✔
- **Two overlay sprites still occlude each other** — from the +Z side the nearer
  one's disc is over the further one's, the right way round. ✔
- **A cutout sprite's edge is hard** — it is, and it is directly under the
  blended disc of the same frame so the two are comparable. **Its crawling was
  not confirmed**: that is motion and a still cannot show it. Recorded as
  expected, not as seen.
- **Blended sprites composite in the right order as the order swaps** — the
  see-through pair straddle the origin in Z at one X, the same arrangement card
  021a's quads use. Looked right from both sides; the swap itself happens between
  captures and was not caught mid-swap.
- **Both facing rotations** — both are built and both track the camera's yaw:
  from well off to the side the two triangles face the camera square-on while the
  unbillboarded row of discs beside them foreshortens into ellipses, which is the
  clearest possible statement that the engine did not do it. **The pitch half was
  not confirmed.** The orbit camera's pitch is a fixed ~14°, which foreshortens by
  three per cent and is invisible; seeing the cylindrical one stay upright while
  the spherical one leans back needs the flying camera — **Tab, then Space to
  climb** — and this machine has no way to drive a key from a script. That is one
  minute at the keyboard and it is the one thing on the Verify list a person still
  has to do.

### Markers left

None. No `DEVIATION:` and no `BLOCKED:` in the code.

## Report, as the card asked

### Whether `sprite` earned being its own module

**Honestly: barely, and it depends on the next card.** What got built is one
geometry upload, one division and a loop — under sixty lines of code across two
files. On its own that is thin enough to have been two functions in `3d`.

Two things argue the other way, and I think they win. It has no component and no
system, so it is a leaf that costs nothing to leave alone; and cards 023 (`ui`)
and whatever `text`'s neighbours become will want a place to put exactly this
kind of thing. If the next sprite card — animation — adds a component, the folder
is obviously right. If nothing else ever lands in it, it should be folded into
`3d`.

### The dependency row departs from the module map, deliberately

`CLAUDE.md`'s tree says *"text · sprite · ui — planned, not yet created — each on
render, low level"*. What was built names `3d`, because its whole job is the
material a sheet's frames wear and a material is `voe_3d_material`. The card said
so explicitly (*"It depends on `render` and `3d`"*), so this is not a surprise,
but **the tree in `CLAUDE.md` now says something untrue about a folder that
exists** and the reason is written into `cmake/voe.cmake` rather than there.

**There was a render-only shape and it was not taken.** `sprite` could have
stayed a leaf beside `text` by handing back a frame's offset and scale for the
caller to put on its own material — `math` and `render` only, no `3d`. That keeps
the map's claim true and makes the folder thinner still; what it costs is that
every caller writes the loop that `voe_sprite_sheet_upload` writes once. The card
chose, and I built what it chose. Flagging it because the map is the thing that
is now wrong, and because whether `sprite` is low-level or above `3d` is
architecture rather than my call.

**`CLAUDE.md` was not edited.** Its folder tree also still says `text` is "not yet
created", so it was already behind the code before this card. That is a document
the tech lead owns, not something to fix in passing.

### Whether one unit quad for every sprite held up

**Yes, without strain.** Twelve sprites, one geometry id, and nothing wanted its
own. The only decision it forced was single- versus double-sided, and that is a
property of the quad rather than of a sprite, so one quad still answers it for
everybody.

Where it would stop holding: a sprite whose picture is not square. A cell that is
64×32 on a 1:1 quad comes out stretched, and the fix today is a non-uniform
scale on the transform — which works and is what `dev`'s heads-up panel already
does. If that becomes common, the aspect wants to come from the sheet rather
than from every call site.

### What the per-object draw cost actually measured at

**About two milliseconds of CPU per thousand blended sprites, which is inside
ADR-0081's predicted 1–3 ms.** Measured by temporarily adding a thousand more
blended sprites to `dev` on the machine above, reading the timing block, and
taking the code back out — nothing of that measurement is in the diff.

| | frames/s | draw (CPU) | gpu (card's own clock) |
|---|---|---|---|
| the twelve sprites the exhibit ships with | ~1500 | 0.60–0.85 ms | 0.03 ms |
| plus a thousand blended sprites | ~370 | 2.64–2.76 ms | 0.08 ms |
| plus a thousand **opaque** sprites | ~426 | 2.34 ms | 0.07 ms |

Two things fall out of that beyond the headline number.

**It is entirely CPU-side submission, exactly as ADR-0081 argued.** The card's own
clock barely moved — 0.03 ms to 0.08 ms for a thousand more draws. The GPU is not
what a thousand sprites costs.

**The sort is not the problem, and now there is a number rather than a
prediction.** The same thousand sprites drawn opaque — no sort, no blended pass —
cost 2.34 ms against 2.68 ms blended. So the sort *and* the blended path together
are about 0.33 ms at a thousand objects, of which the sort is some part. ADR-0081
predicted 0.02–0.2 ms for the sort alone; that is consistent, and it is certainly
not off by an order of magnitude. D-070 can be answered with a measurement now
instead of an argument.

### Anything that made the no-billboarding decision awkward in practice

**Nothing, and that is worth saying plainly, because it is the decision the card
warned would feel like an omission.** `voe_dev_sprites_face` is about twenty
lines including the comment, and the difference between cylindrical and spherical
is one term. `scene` already hands out the camera's `yaw` and `pitch`, so no
trigonometry is done at the call site and no forward vector is decomposed.

The one place it showed at all: an engine-side helper would have been the natural
home for "which frame is a sprite standing on the ground". There is no ground in
this engine, so it did not come up.

### One thing the card did not ask about but that bit

**`render`'s shading record no longer has a safe zero, and that is new.** Every
other field's zero is the answer somebody would have wanted — opaque, lit, no
texture, `SMOOTH` — and `device.h` says so three separate times. A zeroed UV
scale is not: it reads one texel across the whole surface. This is not
theoretical. `render/tests/offscreen.c` builds a `voe_render_shading_values` by
hand and it failed the moment the field existed, in exactly that way: the cube
came out one flat colour and four of its checks went red. It now names the rect
explicitly with a comment saying why the line is not optional.

The card put the default in `3d` (*"Default is offset (0,0) scale (1,1)"* is in
the `3d` bullet) and gave `render` *"four floats, one multiply-add. Nothing
else"*, so that is what was built and the trap is documented at the field. **The
alternative is one `if` inside `voe_render_shading_create`** — a startup call,
so no per-frame cost — which would keep "a zeroed record is a sane record" true
for the whole engine and cost `render` about four lines. There are only two
places in the tree that build a record by hand, so it is cheap either way; which
one is right is a convention question and therefore not mine.

## What was not touched

The uncommitted work in the tree when this card was claimed — card 027's changes
to `render/src/texture.c`, `render/src/device_internal.h`, `text/src/font.c` and
their `.md` files — is still uncommitted and is now mixed in the same working
tree as this card's diff. Nothing of card 027's was changed, but the two cards'
changes are not separable by `git status` alone.
