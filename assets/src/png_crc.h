// PNG's CRC-32, shared by the reader and the writer. Internal to assets.
//
// IT EXISTS SO THAT THERE IS ONE CRC IN THIS FOLDER AND NOT TWO. The table and
// the loop live in src/png.c, where the reader already had them; src/png_write.c
// needs the same polynomial over the same bytes to stamp the chunks it emits,
// and a second copy is a second thing to get wrong in a way no round trip would
// ever notice — the writer and the reader would simply agree with each other and
// with nobody else.
//
// ONE CALL OVER ONE RUN OF BYTES, AND THERE IS NO SEED TO CHAIN FROM. Both
// callers have the bytes they want covered laid out contiguously — the reader
// has the whole chunk in the file, the writer has just copied the type and the
// data into the buffer it is filling — so neither has anything to continue from,
// and rule 10 says a running value is written when something needs one. The
// inversion either end of the loop is inside here, which is why this returns a
// finished CRC and not a state.
//
// DEVIATION: rule 10 (implement on demand), the signature is
// voe_assets_png_crc(bytes, count) and not the uint32_t seed parameter spec
// 003's task 3 spelled out, because nothing in the tree has a running value to
// pass and rule 10 asks for the over-specified surface to be reported rather
// than written. The seed comes back the day something writes a chunk in pieces —
// an IDAT split across chunks would — and src/png.c is the only file that
// changes when it does.
//
// A CHUNK'S CRC COVERS ITS TYPE AND ITS DATA, AND NOT ITS LENGTH. That is the
// format's rule, and it is the one mistake that makes every file this engine
// writes unreadable everywhere else while still decoding here.
#pragma once

#include <stddef.h>
#include <stdint.h>

// CRC-32 as PNG defines it, over `count` bytes at `bytes`.
uint32_t voe_assets_png_crc(const uint8_t *bytes, size_t count);
