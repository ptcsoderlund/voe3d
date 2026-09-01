# 017 — texture loading: PNG and JPEG

status: todo
claimed-by: -
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
