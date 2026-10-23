// A prepared `.glb`'s cooked copy (0470 point 1): the bytes of
// `Cache/models/<path>.model`, written from a prepare and read back as one, so
// a load is a read and a put with no parse, bake or decode. The format alone:
// `game` reads and writes the files.
//
//     voe_3d_model_cooked_key key = { glb_stamp, parts_stamp }, kept;
//     voe_assets_bytes copy = voe_3d_model_cooked_write(arena, prepared, key);
//     ... // write copy.bytes, copy.count
//     prepared = voe_3d_model_cooked_read(arena, file, file_size, &kept);
//     if (prepared == NULL || kept.glb != key.glb || kept.parts != key.parts)
//             ... // prepare from the `.glb` and write the copy again
//
// THE LAYOUT. A head of VOE_3D_MODEL_COOKED_HEAD bytes: magic "VOEM", the
// version, the key's two stamps and the whole copy's size. Then sections, each
// starting at a multiple of 16 from the first byte: the materials, the
// primitives' materials, every picture as voe_assets_image_cooked_write lays
// it out, level 0's bake, the CPU shape, the sphere and flags, the own levels'
// bakes and the levels read. Native-endian and for this machine's cache only,
// like a cooked picture: the `.glb` is what travels.
//
// THE VERSION RISES WITH ANY CHANGE to the bake, to how the levels are made or
// to this layout. A copy whose magic or version is not this one's is stale and
// made again; nothing reads an older layout.
//
// A READ FILLS WHAT THE PUT READS, the rest empty. Of the model: every material
// whole, each primitive's material alone (no vertices, no indices), and every
// picture with its levels; no nodes, meshes, roots, names or encoded bytes. Of
// the prepare: all of it, but the levels read only when made or measured,
// which is when the put uses them; otherwise they are zero.
//
// MIPS ARE BY USE. A level is the area average of what its texels mean (0466):
// colour in linear light weighted by alpha, a normal made unit length again,
// data as stored. A chain made the wrong way darkens a colour or flattens a
// bump, so each picture is mipped by its first use in the model's materials:
// base colour and emission colour, normal normal, metal-roughness and
// occlusion data, one no material names colour. A picture used two ways keeps
// its first use's chain in both.
//
// CONSTRAINTS: the write pushes every picture's mipped copy and the copy itself
// onto `arena`, about twice the pictures' bytes beside the result. The read
// checks every count, range and index against what it indexes, so a damaged
// copy cannot read past an array; it does not check floats.
#pragma once

#include <3d/model_prepare.h>
#include <assets/image.h>
#include <base/arena.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// The platform stamps of the `.glb` and of the `.parts` beside it, 0 for none.
typedef struct {
	uint64_t glb, parts;
} voe_3d_model_cooked_key;

// The head's size in bytes: all voe_3d_model_cooked_key_of reads.
#define VOE_3D_MODEL_COOKED_HEAD 32

// `prepared` as a cooked copy on `arena`, keyed `key`. Asserts its pictures
// are decoded; each is given its full chain (voe_assets_image_mips) of the
// kind its first use says, and written with key 0. Cannot fail.
voe_assets_bytes voe_3d_model_cooked_write(voe_base_arena *arena,
					   const voe_3d_model_prepared *prepared,
					   voe_3d_model_cooked_key key);

// The key in a copy's head, from the head alone. False, `key` untouched, when
// `size` is short of a head or its magic or version is not this one's.
[[nodiscard]] bool voe_3d_model_cooked_key_of(const uint8_t *head, size_t size,
					      voe_3d_model_cooked_key *key);

// A copy read back as a prepare, it and its model pushed on `arena`, their
// arrays pointing into `bytes`, which must be 16-aligned (asserted) and
// outlive the put, and are never written. `*key` is the head's; the caller
// compares it. NULL with no line, nothing left on `arena`, on a wrong magic,
// version or size or a count, range or index out of bounds: a broken copy is
// made again.
[[nodiscard]] const voe_3d_model_prepared *
voe_3d_model_cooked_read(voe_base_arena *arena, const uint8_t *bytes,
			 size_t size, voe_3d_model_cooked_key *key);
