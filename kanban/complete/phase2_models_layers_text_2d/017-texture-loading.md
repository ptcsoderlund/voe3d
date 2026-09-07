# 017 — texture loading: PNG and JPEG

status: complete
claimed-by: claude-code (kanban-coder)
blocked-by: 014

We write the readers ourselves (ADR-0023). Nothing is fetched.

## Goal

A texture from a file on the cube.

## Scope

- **PNG and JPEG readers, in `assets`.** Both written here.
- **UV coordinates arrive now**, which means **the cube becomes 24 vertices** —
  each corner needs different coordinates per face. Card 014 said this was coming.
- Sampler, image, image view, descriptor set. **Descriptor sets are per frame slot**
  (ADR-0050) the moment anything writes to them per frame.
- Mip generation, or an explicit decision not to yet, stated either way.
- GPU textures are referenced from components by **generational id, and that id is
  the same index the shader uses** (ADR-0018).

## Both readers are more work than they look

- **PNG** is the smaller job: zlib inflate, then unfiltering per scanline. Interlaced
  PNG is rare — refusing it with a clear failure is a legitimate answer, said out
  loud rather than silently.
- **JPEG** is the bigger one: Huffman, dequantise, inverse DCT, upsample, colour
  convert. Baseline only. **Progressive JPEG is a different decoder** — refuse it
  explicitly rather than producing a corrupt image.
- **`assets` parses with an explicit stack and a nesting limit** and never with
  recursive descent on file input (ADR-0043). Binding here.
- **Every dimension and length in the file is hostile until checked.** These are the
  two most-attacked file formats in existence. A malformed header must be a
  recoverable failure, never an allocation of whatever the file asked for.

## Verify

- A known image on the cube, right way up. **Upside-down is the classic outcome** —
  image rows run top-down and texture coordinates conventionally run bottom-up.
  Decide where the flip lives, do it once, and say where.
- A truncated file, a wrong magic number and an absurd declared size are all
  recoverable failures, each with a test.
- `check.cmake` zero. Windows is the principal's.

## Notes — implementation

New folder `assets`, plus texture support in `render`, `dev` and `base`.

### Decisions the card asked to be made

**The flip lives nowhere, deliberately.** PNG row zero is the top, Vulkan's
texture coordinate (0,0) is the top-left texel, and glTF's UVs put (0,0) at the
top-left of the image. All three already agree, so rows are uploaded in decode
order and the picture comes out the right way up. The flip everyone reaches for
is OpenGL's, whose texture origin is at the bottom; adding one here is what makes
the picture upside down. Written down in `assets/include/assets/image.h` and
again in `cube.slang`, because it is the thing a future card will be tempted to
"fix".

**Mipmaps are generated**, by blitting each level into the next. The cube is
flown around, and a minified texture without mipmaps shimmers in a way that gets
blamed on the camera. The card allowed either answer; this is the one with the
better picture. `VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT` is queried
first and a card that cannot filter linearly gets a single level rather than a
broken chain.

**Decoders take a byte buffer, not a path.** `platform` owns files and has no
file API — `platform/window.h` says so outright — so opening one in `assets`
would be a reach into another folder. `dev` gets its picture through `#embed` at
build time, exactly as `render` gets its shaders, which also keeps "nothing is
read from disk at run time" true. The card that gives `platform` a file API is
the card that makes this take a path, and nothing in `assets` changes when it
does.

**Textures are RGBA8 (`UNORM`) whatever the file held.** One layout means one
upload path and no branch downstream. UNORM and not SRGB because there is no
lighting yet and an SRGB view would make the cube visibly pale against the same
file in any image viewer; card 019 brings the light and is the card that changes
this and the swapchain together.

### ADR-0018, made concrete

`voe_render_texture` is `{ index, generation }`. The index **is** the subscript
the fragment shader uses into its descriptor array — there is no lookup table in
between, so nothing can fall out of step. The generation never reaches the GPU
and is what makes a stale id safe: slot 0 is a one-pixel white default that is
never handed out, unclaimed slots point at it so every array element is always
valid, and `voe_render_device_set_texture` refuses an id whose generation no
longer matches. `render/tests/offscreen.c` proves that refusal.

### What was run

`cmake -P check.cmake` on **Linux** — **all 15 steps ok**, 14 tests, analyser
clean over 40 files (clang 21.1.8, cmake 4.2.3, slangc 2026.16.1). Tests went
from 11 to 14 and analysed files from 33 to 40.

Both decoders were also run under **AddressSanitizer and UndefinedBehaviorSanitizer**
during development — clean, no leaks. That is not part of `check.cmake` and was
not added to it; it is worth knowing the hostile-input tests were run that way.

`dev` launched against WSLg: `texture 64x64 in slot 1`, window opened, no crash.

### Two bugs the tests caught, both the same shape

Worth recording because the second one repeated the first in a different format.

- **A truncated PNG decoded happily.** The chunk walk never required `IEND`, so
  cutting the last twelve bytes off left every chunk this decoder reads intact
  and the file passed. `IEND` is now required.
- **A truncated JPEG did the same**, for the same reason, and additionally could
  not tell "the bytes ran out" from "a marker ended the scan" — both stopped the
  bit reader identically. Now `truncated` is separate from `overrun`, and `EOI`
  is required.

The lesson is in both files: the end marker is the only thing in either format
that says *that was all of it*, and a picture arriving whole is not evidence the
file did.

One more, in `inflate.c`: the first version refused incomplete Huffman codes,
which refuses every fixed-Huffman DEFLATE block — the fixed **distance** code is
30 symbols in a five-bit space and is itself incomplete. Incomplete codes are now
accepted; the safety comes from `huffman_decode` only ever returning a symbol
that was assigned, which is argued in that file's header.

### Scope beyond `assets`, and why

The card names all of it, but it is worth listing because it crosses folders:

- `base` — one new error code, `VOE_BASE_ERROR_MALFORMED`. Rule 13 says codes are
  added when a card needs one; none of the three existing codes means "this data
  is not what it claims to be", and the distinction from `UNSUPPORTED` is what a
  caller can do about it.
- `render` — `src/texture.c`, four new loader entry points, the descriptor array,
  the push constant gaining a texture index and the range gaining the fragment
  stage, and the cube becoming 24 vertices.
- `dev` — decodes and uploads the picture; `DEPENDS` gained `assets`.
- Root `CMakeLists.txt` — `add_subdirectory(assets)`. Without it the folder
  configured standalone and was silently absent from the root build, which is
  how the first green run reported the same test count as before.

### The per-corner vertex colour is gone

Card 014 gave each corner a colour so a person could tell the faces apart; the
texture does that better, and keeping it as a tint would mean the cube never
showed the colours the file actually contains — which is the one thing this card
has to be checkable by. `cube.slang`'s own header had said the card that adds
texture coordinates rewrites this data.

That changed what `render/tests/offscreen.c` could assert, since it read those
colours to prove back-face culling and the Y flip. It now uses a texture of its
own — red top-left, blue bottom-left, green down the right — and compares
**centroids** rather than pixels at chosen coordinates, because the camera sits
above and behind the cube so no fixed coordinate is stable. Red above blue is the
right way up; green right of red is the near face. Both invert in the mirrored
case, and they invert for two different reasons, so a mistake in one moves only
one of them. This test now covers the card's "right way up" requirement
automatically rather than by eye.

### Not verified

- **Windows — not touched.** Per the new one-platform rule, a bug report rather
  than a held card.
- **The picture itself has not been looked at by a person.** The orientation is
  checked programmatically by `offscreen.c`, and `dev` reports the texture
  uploaded, but only a software renderer (llvmpipe) was available here and no one
  has seen the cube. The embedded image is an "F" with four coloured corners —
  red top-left, green top-right, blue bottom-left, yellow bottom-right — chosen
  because it reads wrong under a flip and under a mirror.
- **Mipmap generation has not been seen working**, only exercised. llvmpipe
  reports linear-filter support so the blit chain ran, but whether the levels
  look right at a distance wants a real GPU and a person.
- JPEG is tested against files from an encoder written alongside it. The header
  of `assets/tests/jpeg.c` says what that is and is not worth.

No `DEVIATION:` or `BLOCKED:` markers were left in the code.
